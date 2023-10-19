#
# This test case tests for additional problems in our implementation of the
# S3 API.
#

import pytest

def has_bucket(s3, bucket):
    buckets = s3.list_buckets()
    for b in buckets['Buckets']:
        if b['Name'] == 'my_bucket':
            return True

    return False


def test_create_bucket(s3):
    s3.create_bucket(Bucket='my_bucket')
    assert has_bucket(s3, 'my_bucket')
