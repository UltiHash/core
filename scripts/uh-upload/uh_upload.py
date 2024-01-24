#!/usr/bin/env python3

import argparse
import concurrent
import boto3
import os
import pathlib
import sys
import time


def parse_args():
    parser = argparse.ArgumentParser(
        prog='UH upload',
        description='Uploading files to UH cluster')

    parser.add_argument('path', help='directory or file to upload',
        type=pathlib.Path, nargs='+')
    parser.add_argument('-u', '--url', help='override default S3 endpoint',
        nargs=1, default='http://localhost:8080', dest='url')
    parser.add_argument('-v', '--verbose', help='write additional information to stdout',
        action='store_true', dest='verbose')
    parser.add_argument('-B', '--bucket', help='upload all files to the given bucket',
        action='store')
    parser.add_argument('-j', '--jobs', help='number of concurrent jobs',
        action='store', default=8, type=int)

    return parser.parse_args()

class uploader:
    def __init__(self, config):
        self.threads = concurrent.futures.ThreadPoolExecutor(max_workers=config.jobs)
        self.s3 = boto3.client('s3', endpoint_url=config.url)
        self.config = config

    def upload(self, bucket, path):
        stats = dict()

        with open(path, 'rb') as f:
            resp = s3.put_object(Bucket=bucket, Key=path.name, Body=f)

            headers = resp['ResponseMetadata']['HTTPHeaders']
            stats['uploaded_bytes'] = float(headers['uh-original-size'])
            stats['stored_bytes'] = float(headers['uh-effective-size'])

        return stats

    def push(self, path):
        results = []
        path = path.resolve()

        if self.config.bucket is not None:
            bucket_name = self.config.bucket
        else:
            bucket_name = path.name

        print(f"uploading {path} to bucket {bucket_name}")

        self.s3.create_bucket(Bucket=bucket_name)
        for (root, dirs, files) in os.walk(path):
            for file in files:
                fullpath = pathlib.Path(root) / file

                if self.config.verbose:
                    print(fullpath)

                results += [(fullpath, self.threads.submit(self.upload, bucket_name, fullpath))]

        return results

if __name__ == "__main__":
    config = parse_args()

    s3 = boto3.client('s3', endpoint_url=config.url)

    start_time = time.monotonic_ns()

    up = uploader(config)
    results = []

    for path in config.path:
        results += up.push(path)

    uploaded_bytes = 0
    stored_bytes = 0
    for f in results:
        try:
            info = f[1].result()
            uploaded_bytes += info['uploaded_bytes']
            stored_bytes += info['stored_bytes']
        except Exception as e:
            print("Error uploading %s: %s" % (f[0], str(e)), file=sys.stderr)

    end_time = time.monotonic_ns()

    elapsed_s = (end_time - start_time) / 1000000000
    uploaded_mb = uploaded_bytes / (1024 * 1024)

    print("elapsed time: %.02f s" % elapsed_s)
    print("uploaded bytes: %d" % uploaded_bytes)
    print("stored bytes: %d" % stored_bytes)
    print("upload bandwidth: %.02f MB/s" % (uploaded_mb / elapsed_s))
    print("storage reduction: %.02f %%" % (100 * (1 - stored_bytes / uploaded_bytes)))
