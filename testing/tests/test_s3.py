#
# This test case tests for additional problems in our implementation of the
# S3 API.
#

import pytest
import pprint

def has_bucket(s3, bucket):
    buckets = s3.list_buckets()
    pprint.pprint(buckets)
    for b in buckets['Buckets']:
        if b['Name'] == bucket:
            return True

    return False


def test_create_bucket(s3):
    response = s3.create_bucket(Bucket='my_bucket')
    pprint.pprint(response)

    assert has_bucket(s3, 'my_bucket')
