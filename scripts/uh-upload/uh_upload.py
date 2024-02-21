#!/usr/bin/env python3

import argparse
import concurrent
import boto3
import os
import pathlib
import sys
import time

AWS_KEY_ID="key-id"
AWS_KEY_SECRET="secret"

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
        self.s3 = boto3.client('s3', endpoint_url=config.url,
            aws_access_key_id=AWS_KEY_ID, aws_secret_access_key=AWS_KEY_SECRET)
        self.config = config

    def upload(self, bucket, path):
        stats = dict()

        with open(path, 'rb') as f:
            resp = self.s3.put_object(Bucket=bucket, Key=path.name, Body=f)

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

class progress_bar(object):
    def __init__(self, start, files):
        self.files = files
        self.done = 0
        self.uploaded = 0
        self.stored = 0
        self.start = start

    def update(self, uploaded, stored):
        self.done += 1
        self.uploaded += uploaded
        self.stored += stored

    def print(self):
        elapsed_s = (time.monotonic_ns() - self.start) / 1000000000
        uploaded_mb = self.uploaded / (1024 * 1024)
        reduction = 0 if self.uploaded == 0 else (1 - self.stored / self.uploaded)

        print(f"\rUploaded {uploaded_mb: .02f} MB of data @ {uploaded_mb / elapsed_s: .02f} MB/s, " +
              f"storage reduction {100 * reduction: .02f} %    { 100 * self.done / self.files: .02f} % ",
              end='')


if __name__ == "__main__":
    config = parse_args()

    start_time = time.monotonic_ns()

    up = uploader(config)
    results = []

    for path in config.path:
        results += up.push(path)

    prg = progress_bar(start_time, len(results))

    while len(results) > 0:
        results_remain = []

        for f in results:
            if f[1].done():
                try:
                    info = f[1].result()
                    prg.update(info['uploaded_bytes'], info['stored_bytes'])
                except Exception as e:
                    print(f"Error uploading {f[0]}: {str(e)}", file=sys.stderr)
            else:
                results_remain += [ f ]

        time.sleep(0.1)

        results = results_remain
        prg.print()

    print()
