#
# This file contains concurrent read and write tests for testing the UH Cluster.
#
import pytest
import boto3
from boto3.s3.transfer import TransferConfig
import concurrent.futures
import queue
from s3_util import unused_bucket_name, unused_object_key, has_bucket

WORKER_THREADS = 10
GB = 1024 ** 3
MP_THRESHOLD = 1 * GB
MP_CONCURRENCY = 5
transfer_config = TransferConfig(multipart_threshold=MP_THRESHOLD, use_threads=True, max_concurrency=MP_CONCURRENCY)

@pytest.fixture(scope="function")
def bucket(s3):
    name = unused_bucket_name(s3)
    s3.create_bucket(Bucket=name)
    assert has_bucket(s3, name)

    yield name

    response = s3.list_objects_v2(Bucket=name)
    if 'Contents' in response:
        objects = [{'Key': obj['Key']} for obj in response['Contents']]
        s3.delete_objects(Bucket=name, Delete={'Objects': objects})

    response = s3.list_objects_v2(Bucket=name)
    assert 'Contents' not in response, "Bucket is not empty"

    s3.delete_bucket(Bucket=name)
    assert not has_bucket(s3, name)

def multi_chunk_upload(s3, bucket, file):
    key = unused_object_key(s3, bucket)
    s3.upload_file(file, Bucket=bucket, Key=key, Config=transfer_config)
    return key, file

def get_object(s3, bucket, key):
    resp = s3.get_object(Bucket=bucket, Key=key)
    return resp["Body"].read()

def read_file(path):
    with open(path, 'rb') as f:
        return f.read()

def verify(s3, bucket, key, file):
    received_file = get_object(s3, bucket, key)
    original_file = read_file(file)
    if received_file == original_file:
        assert True
    else:
        print("*********************")
        print("Original File Size:", len(original_file), "\nContents:\n", original_file)
        print("*********************")
        print("Received_file:", len(received_file), "\nContents:\n", received_file)
        print("invalid contents retrieved")
        assert False

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

def test_basic_multiparts(s3, files, bucket):
    file = files[0]

    manager = worker_manager()
    manager.post_task(lambda: multi_chunk_upload(s3, bucket, file))

    keys = manager.run_tasks()[0]
    verify(s3, bucket, keys[0], keys[1])

def test_multithreaded_multiparts_singlethreaded_download(s3, files, bucket):
    manager = worker_manager(WORKER_THREADS)
    for file in files:
        manager.post_task(lambda: multi_chunk_upload(s3, bucket, file))

    keys = manager.run_tasks()

    for key, file in keys:
        verify(s3, bucket, key, file)

    assert True

def test_multithreaded_multiparts_multithreaded_download(s3, files, bucket):
    manager = worker_manager(WORKER_THREADS)
    for file in files:
        manager.post_task(lambda: multi_chunk_upload(s3, bucket, file))

    keys = manager.run_tasks()

    for key, file in keys:
        manager.post_task(lambda: verify(s3, bucket, key, file))

    manager.run_tasks()

    assert True
