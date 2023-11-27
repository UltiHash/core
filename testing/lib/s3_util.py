# ########################################################################
#
# S3 support functions
#
# ########################################################################

import string
import util

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

def random_object_key(length=8):
    return util.random_string(OBJECT_NAME_CHARACTERS, length)

def random_bucket_name(length=8):
    return util.random_string(BUCKET_NAME_CHARACTERS, length)

def unused_bucket_name(s3, length=8):
    while True:
        name = random_bucket_name(length)
        if not has_bucket(s3, name):
            return name

def unused_object_key(s3, bucket, length=8):
    while True:
        name = random_object_key(length)
        if not has_object(s3, bucket, name):
            return name

