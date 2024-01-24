#!/usr/bin/env python3

import argparse
import boto3
import os
import pathlib
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

    return parser.parse_args()

def upload_path(s3, path, config, stats):
    path = path.resolve()

    if config.bucket is not None:
        bucket_name = config.bucket
    else:
        bucket_name = path.name
    print("uploading " + str(path) + " to bucket " + bucket_name)

    s3.create_bucket(Bucket=bucket_name)
    for (root, dirs, files) in os.walk(path):
        for file in files:
            fullpath = pathlib.Path(root) / file

            if config.verbose:
                print(fullpath)

            with open(fullpath, 'rb') as f:
                resp = s3.put_object(Bucket=bucket_name, Key=file, Body=f)
                headers = resp['ResponseMetadata']['HTTPHeaders']
                stats['uploaded_bytes'] += float(headers['uh-original-size-mb']) * (1024 * 1024)
                stats['stored_bytes'] += float(headers['uh-effective-size-mb']) * (1024 * 1024)

if __name__ == "__main__":
    config = parse_args()

    s3 = boto3.client('s3', endpoint_url=config.url)

    stats = {
        'uploaded_bytes': 0,
        'stored_bytes': 0
    }

    start_time = time.monotonic_ns()

    for path in config.path:
        upload_path(s3, path, config, stats)

    end_time = time.monotonic_ns()

    elapsed_s = (end_time - start_time) / 1000000000
    uploaded_mb = stats['uploaded_bytes'] / (1024 * 1024)

    print("elapsed time: %.02f s" % elapsed_s)
    print("uploaded bytes: %d" % stats['uploaded_bytes'])
    print("stored bytes: %d" % stats['stored_bytes'])
    print("upload bandwidth: %.02f MB/s" % (uploaded_mb / elapsed_s))
