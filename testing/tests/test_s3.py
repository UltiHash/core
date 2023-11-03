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
BUCKET_NAME_CHARACTERS=string.ascii_lowercase + string.digits

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
    with pytest.raises(Exception):
        s3.create_bucket(Bucket=name)

def test_put_object(s3):
    bucket = unused_bucket_name(s3)
    s3.create_bucket(Bucket=bucket)

    name = unused_object_key(s3, bucket)
    s3.put_object(Bucket=bucket, Key=name)

    assert has_object(s3, bucket, name)

def test_delete_bucket(s3):
    name = unused_bucket_name(s3)
    response = s3.create_bucket(Bucket=name)

    assert has_bucket(s3, name)

    s3.delete_bucket(Bucket=name)
    assert not has_bucket(s3, name)

def test_delete_object(s3):
    bucket = unused_bucket_name(s3)
    response = s3.create_bucket(Bucket=bucket)

    name = unused_object_key(s3, bucket)

    s3.put_object(Bucket=bucket, Key=name)
    assert has_object(s3, bucket, name)

    s3.delete_object(Bucket=bucket, Key=name)
    assert not has_object(s3, bucket, name)

def test_object_content(s3):
    bucket = unused_bucket_name(s3)
    s3.create_bucket(Bucket=bucket)

    name = unused_object_key(s3, bucket)
    body = util.get_random_name(string.printable, 64 * 1024)
    s3.put_object(Bucket=bucket, Key=name, body=body)

    resp = s3.get_object(Bucket=bucket, Key=name)
    assert body == resp['Body']


def test_create_illegal_bucket_name(s3):
    # See https://docs.aws.amazon.com/AmazonS3/latest/userguide/bucketnamingrules.html
    with pytest.raises(Exception):
        s3.create_bucket(Bucket='xx')
    with pytest.raises(Exception):
        s3.create_bucket(Bucket='xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx')
    with pytest.raises(Exception):
        s3.create_bucket(Bucket='XXX')
    with pytest.raises(Exception):
        s3.create_bucket(Bucket='.aa')
    with pytest.raises(Exception):
        s3.create_bucket(Bucket='aa.')
    with pytest.raises(Exception):
        s3.create_bucket(Bucket='-aa')
    with pytest.raises(Exception):
        s3.create_bucket(Bucket='aa-')
    with pytest.raises(Exception):
        s3.create_bucket(Bucket='0.0.0.0')
    with pytest.raises(Exception):
        s3.create_bucket(Bucket='xn--foo')
    with pytest.raises(Exception):
        s3.create_bucket(Bucket='sthree-foo')
    with pytest.raises(Exception):
        s3.create_bucket(Bucket='sthree-configurator-foo')
    with pytest.raises(Exception):
        s3.create_bucket(Bucket='bar-s3alias')
    with pytest.raises(Exception):
        s3.create_bucket(Bucket='bar--ol-s3')

    assert True
