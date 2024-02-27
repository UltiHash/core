# Overview

This document defines the metric parameters implemented by UltiHash
cluster.

## Internal Message-Level Metrics

Each servce measures the count of different internal message types it sends or receives. These are as follows:

### Storage Service Messages:
- `storage_read_fragment_req`: number of requests received for reading a fragment
- `storage_read_address_req`: number of requests received for reading an address
- `storage_write_req`: number of requests received for writing data
- `storage_sync_req`: number of requests received to sync data to persistent storage
- `storage_remove_fragment_req`: number of requests received to remove a fragment from storage
- `storage_used_req`: number of requests received to get the used space

### Deduplicator Service Messages:

- `deduplicator_req`: number of requests received to deduplicate uploaded data

### Directory Service Messages:

- `directory_bucket_list_req`: number of requests received to list buckets
- `directory_bucket_put_req`: number of requests received to insert a bucket
- `directory_bucket_delete_req`: number of requests received to delete a bucket
- `directory_bucket_exists_req`: number of requests received to check if a bucket exists
- `directory_object_list_req`: number of requests received to list objects in a bucket
- `directory_object_put_req`: number of requests received to create an object
- `directory_object_get_req`: number of requests received to retrieve an object
- `directory_object_delete_req`: number of requests received to delete an object 

### Entrypoint S3 Requests:

- `entrypoint_abort_multipart`: number of [`AbortMultipartUpload`](https://docs.aws.amazon.com/AmazonS3/latest/API/API_AbortMultipartUpload.html) requests received 
- `entrypoint_complete_multipart`: number of [`CompleteMultipartUpload`](https://docs.aws.amazon.com/AmazonS3/latest/API/API_CompleteMultipartUpload.html) requests received
- `entrypoint_create_bucket`: number of [`CreateBucket`](https://docs.aws.amazon.com/AmazonS3/latest/API/API_control_CreateBucket.html) requests received
- `entrypoint_delete_bucket`: number of [`DeleteBucket`](https://docs.aws.amazon.com/AmazonS3/latest/API/API_control_DeleteBucket.html) requests received
- `entrypoint_delete_object`: number of [`DeleteObject`](https://docs.aws.amazon.com/AmazonS3/latest/API/API_DeleteObject.html) requests received
- `entrypoint_delete_objects`: number of [`DeleteObjects`](https://docs.aws.amazon.com/AmazonS3/latest/API/API_DeleteObjects.html) requests received
- `entrypoint_get_bucket`: number of [`GetBucket`](https://docs.aws.amazon.com/AmazonS3/latest/API/API_control_GetBucket.html) requests received
- `entrypoint_get_object`: number of [`GetObject`](https://docs.aws.amazon.com/AmazonS3/latest/API/API_GetObject.html) requests received
- `entrypoint_init_multipart`: number of [`CreateMultipartUpload`](https://docs.aws.amazon.com/AmazonS3/latest/API/API_CreateMultipartUpload.html) requests received
- `entrypoint_list_buckets`: number of [`ListBuckets`](https://docs.aws.amazon.com/AmazonS3/latest/API/API_ListBuckets.html) requests received
- `entrypoint_list_multipart`: number of [`ListMultipartUploads`](https://docs.aws.amazon.com/AmazonS3/latest/API/API_ListMultipartUploads.html) requests received
- `entrypoint_list_objects`: number of [`ListObjects`](https://docs.aws.amazon.com/AmazonS3/latest/API/API_ListObjects.html) requests received
- `entrypoint_list_objects_v2`: number of [`ListObjectsV2`](https://docs.aws.amazon.com/AmazonS3/latest/API/API_ListObjectsV2.html) requests received
- `entrypoint_multipart`: number of [`UploadPart`](https://docs.aws.amazon.com/AmazonS3/latest/API/API_UploadPart.html) requests received
- `entrypoint_put_object`: number of [`PutObject`](https://docs.aws.amazon.com/AmazonS3/latest/API/API_PutObject.html) requests received

### Common Messages - Responses

- `success`: Operation successful
- `failure`: Operation failed

## Usage Metrics

- `l1_cache_hit`: Hit count of the L1 cache in the `global_data_view`
- `l1_cache_miss`: Miss count of the L1 cache in the `global_data_view`
- `l2_cache_hit`: Hit count of the L2 cache in the `global_data_view`
- `l2_cache_miss`: Miss count of the L2 cache in the `global_data_view`
- `dedupe_set_frag_count`: The count of fragments pointed in the deduplicator set
- `dedupe_set_frag_size`: The aggregated size of fragments pointed in the deduplicator set
- `total_ingested_size_mb`: The total ingested data
- `total_egressed_size_mb`: The total egressed data
- `total_effective_size_mb`: The total effective size
- `total_size_mb`: The total data size (considering insertions and deletions)