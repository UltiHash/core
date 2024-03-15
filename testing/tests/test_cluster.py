#
# This file contains concurrent read and write tests for testing the UH Cluster.
#
import pytest
import botocore
import concurrent.futures
import queue
from s3_util import unused_bucket_name, unused_object_key

@pytest.fixture(scope="function")
def bucket(s3):
    name = unused_bucket_name(s3)
    s3.create_bucket(Bucket=name)
    yield name

    # response = s3.list_objects_v2(Bucket=name)
    # if 'Contents' in response:
    #     objects = [{'Key': obj['Key']} for obj in response['Contents']]
    #     print(objects)
    #     s3.delete_objects(Bucket=name, Delete={'Objects': objects})
    #
    # s3.delete_bucket(Bucket=name)

def multi_chunk_upload(s3, bucket, file):
    key = unused_object_key(s3, bucket)
    s3.upload_file(file, Bucket=bucket, Key=key)
    return key, file

def get_object(s3, bucket, key):
    resp = s3.get_object(Bucket=bucket, Key=key)
    return resp["Body"].read()

def read_file(path):
    with open(path, 'rb') as f:
        return f.read()

def verify(s3, bucket, key, file):
    received_file = get_object(s3, bucket, key)
    if received_file == read_file(file):
        assert True
    else:
        print("invalid contents retrieved")
        assert False

def worker_manager(workers, func, s3, files, bucket):
    with concurrent.futures.ThreadPoolExecutor(max_workers=workers) as executor:
        futures = [executor.submit(func, s3, bucket, file) for file in files]

        keys = []
        for future in concurrent.futures.as_completed(futures):
            key = future.result()
            keys.append(key)

    return keys

class worker_manager:
    def __init__(self, workers=1):
        self.task_queue = queue.Queue()
        self.executor = concurrent.futures.ThreadPoolExecutor(max_workers=workers)

    def __del__(self):
        self.executor.shutdown()

    def post_task(self, task):
        self.task_queue.put(task)

    def run_tasks(self):
        futures = []
        values = []

        while not self.task_queue.empty():
            task = self.task_queue.get()
            future = self.executor.submit(task)
            futures.append(future)

        for future in concurrent.futures.as_completed(futures):
            values.append(future.result())

        return values

def test_single_upload_onsingle_thread(s3, files, bucket):
    file = files[0]
    manager = worker_manager()
    manager.post_task(lambda: multi_chunk_upload(s3, bucket, file))

    keys = manager.run_tasks()

    received_file = get_object(s3, bucket, keys[0][0])

    if received_file == read_file(file):
        s3.delete_bucket(Bucket=bucket)
        assert True
    else:
        print("invalid contents retrieved")
        s3.delete_bucket(Bucket=bucket)
        assert False

def test_multithreaded_upload_singlethreaded_download(s3, files, bucket):
    manager = worker_manager(10)
    for file in files:
        manager.post_task(lambda: multi_chunk_upload(s3, bucket, file))

    keys = manager.run_tasks()

    for key, file in keys:
        verify(s3, bucket, key, file)

    assert True

def test_multithreaded_upload_multithreaded_download(s3, files, bucket):
    manager = worker_manager(10)
    for file in files:
        manager.post_task(lambda: multi_chunk_upload(s3, bucket, file))

    keys = manager.run_tasks()

    for key, file in keys:
        manager.post_task(lambda: verify(s3, bucket, key, file))

    manager.run_tasks()

    assert True
