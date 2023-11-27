#!/usr/bin/env python3
import errno

import boto3
import sys
from botocore.config import Config
import hashlib

# config
my_config = Config(
    signature_version = 'v4',
)

host_url=f"http://127.0.0.1:8080"
# THE UPLOAD HAPPENS HERE
s3 = boto3.resource('s3', endpoint_url=host_url, use_ssl=False)
s3_bucket="test"
s3_file="/home/max/Desktop/IMAGE-ASTRONOMY-glimpse/test1/ssc2014-02a1.tif"
# Create the new bucket
s3.create_bucket(Bucket=s3_bucket)
s3.Bucket(s3_bucket).upload_file(s3_file, "ssc2014-02a1.tif")

s3 = boto3.client('s3', endpoint_url=host_url, use_ssl=False, config=my_config)
response=s3.get_object(Bucket=s3_bucket, Key="ssc2014-02a1.tif")

# Read the content of the object from the response
file_content = response['Body'].read()
# Save the object content to a local file
with open("/home/max/Desktop/restored.tif", 'wb') as file:
    file.write(file_content)

digest_orig = None
digest_rest = None

algo = hashlib.sha512()
with open("/home/max/Desktop/IMAGE-ASTRONOMY-glimpse/test1/ssc2014-02a1.tif", "rb") as f:
    for byte_block in iter(lambda: f.read(4096), b""):
        algo.update(byte_block)
    digest_orig = algo.hexdigest()

algo = hashlib.sha512()
with open("/home/max/Desktop/restored.tif", "rb") as f:
    for byte_block in iter(lambda: f.read(4096), b""):
        algo.update(byte_block)
    digest_rest = algo.hexdigest()

print("sha512(input): " + digest_orig)
print("sha512(output): " + digest_rest)

if(digest_orig != digest_rest):
    print("Checksums do not match!", file=sys.stderr)
    sys.exit(errno.EBADMSG)
