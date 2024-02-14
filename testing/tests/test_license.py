#
# This file contains tests related to licensing.
#

import pytest
import botocore
import string
import util
from s3_util import unused_bucket_name, unused_object_key


def test_put_object_limit_1GB(s3):
    bucket = unused_bucket_name(s3)
    s3.create_bucket(Bucket=bucket)

    body = util.random_string(string.printable, 128 * 1024 * 1024)

    objects = []
    failure_received = False

    for index in range(1, 10):
        name = unused_object_key(s3, bucket)
        try:
            s3.put_object(Bucket=bucket, Key=name, Body=body)
            objects += [name]
        except botocore.exceptions.ClientError as e:
            if e.response['Error']['Code'] == 'StorageLimitExceeded':
                failure_received = True
                break
            raise

    assert failure_received

    for name in objects:
        s3.delete_object(Bucket=bucket, Key=name)

    name = unused_object_key(s3, bucket)
    s3.put_object(Bucket=bucket, Key=name, Body=body)

    assert True
