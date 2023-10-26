#
# This test case tests for additional problems in our implementation of the
# S3 API.
#

import pytest
import string
import util

# ########################################################################
#
# Supporting functions related to the test domain (SUPPORT)
#
# ########################################################################

#
# Valid characters in bucket names according to
# https://docs.aws.amazon.com/AmazonS3/latest/userguide/bucketnamingrules.html
#
BUCKET_NAME_CHARACTERS=string.ascii_lowercase + string.digits + ".-"

#
# Valid characters in object keys according to
# https://docs.aws.amazon.com/AmazonS3/latest/userguide/object-keys.html
#
OBJECT_NAME_CHARACTERS=string.ascii_letters + string.digits + "!-_.*'()"

def has_bucket(s3, bucket):
    buckets = s3.list_buckets()
    for b in buckets['Buckets']:
        if b['Name'] == bucket:
            return True

    return False

def has_object(s3, bucket, key):
    objs = s3.list_objects_v2(Bucket=bucket)

    for o in objs['Contents']:
        if o['Key'] == key:
            return True

    return False

def unused_bucket_name(s3, length=8):
    while True:
        name = util.get_random_name(BUCKET_NAME_CHARACTERS, length)
        if not has_bucket(s3, name):
            return name

def unused_object_key(s3, bucket, length=8):
    while True:
        name = util.get_random_name(OBJECT_NAME_CHARACTERS, length)
        if not has_object(s3, bucket, name):
            return name

# ########################################################################
#
# Test cases follow (TEST_CASES)
#
# ########################################################################

def test_create_bucket(s3):
    name = unused_bucket_name(s3)
    response = s3.create_bucket(Bucket=name)

    assert has_bucket(s3, name)


def test_put_object(s3):
    bucket = unused_bucket_name(s3)
    s3.create_bucket(Bucket=bucket)

    name = unused_object_key(s3, bucket)
    s3.put_object(Bucket=bucket, Key=name)

    assert has_object(s3, bucket, name)


#def test_create_illegal_bucket_name(s3):
# https://docs.aws.amazon.com/AmazonS3/latest/userguide/bucketnamingrules.html
#assert False
