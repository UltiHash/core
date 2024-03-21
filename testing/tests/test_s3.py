#
# This test case tests for additional problems in our implementation of the
# S3 API.
#

import pytest
import string
import util
from s3_util import has_bucket, has_object, unused_bucket_name, unused_object_key

def test_create_bucket(s3):
    name = unused_bucket_name(s3)
    response = s3.create_bucket(Bucket=name)

    assert has_bucket(s3, name)
    with pytest.raises(Exception):
        s3.create_bucket(Bucket=name)

    s3.delete_bucket(Bucket=name)

def test_put_object(s3):
    bucket = unused_bucket_name(s3)
    s3.create_bucket(Bucket=bucket)

    name = unused_object_key(s3, bucket)
    s3.put_object(Bucket=bucket, Key=name)

    assert has_object(s3, bucket, name)

    s3.delete_object(Bucket=bucket, Key=name)
    s3.delete_bucket(Bucket=bucket)

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

    s3.delete_bucket(Bucket=bucket)

def test_object_content(s3):
    bucket = unused_bucket_name(s3)
    s3.create_bucket(Bucket=bucket)

    name = unused_object_key(s3, bucket)
    body = util.random_string(string.printable, 64 * 1024)
    s3.put_object(Bucket=bucket, Key=name, Body=body)

    resp = s3.get_object(Bucket=bucket, Key=name)
    assert body.encode('ascii') == resp['Body'].read()

    s3.delete_object(Bucket=bucket, Key=name)
    s3.delete_bucket(Bucket=bucket)


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
