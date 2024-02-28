# Overview

This document defines the metric parameters implemented by UltiHash cluster. 

There are two types of metrics currently supported: counters and gauges.
- depending on the data type they are initialized with, counters can be monotonic, 
and can only be modified explicitly based on deltas that are added/subtracted
- gauges read absolute values from callbacks, and thus are updated implicitly

## Service-specific request metrics

Each service measures the number of requests it receives and handles using monotonic counters. These are as follows:

### Storage service requests (internal, custom protocol):
- `storage_read_fragment_req`: number of requests received for reading a fragment
- `storage_read_address_req`: number of requests received for reading an address
- `storage_write_req`: number of requests received for writing data
- `storage_sync_req`: number of requests received to sync data to persistent storage
- `storage_remove_fragment_req`: number of requests received to remove a fragment from storage
- `storage_used_req`: number of requests received to get the used space

### Deduplicator service requests (internal, custom protocol):

- `deduplicator_req`: number of requests received to deduplicate uploaded data

### Directory service requests (internal, custom protocol):

- `directory_bucket_list_req`: number of requests received to list buckets
- `directory_bucket_put_req`: number of requests received to insert a bucket
- `directory_bucket_delete_req`: number of requests received to delete a bucket
- `directory_bucket_exists_req`: number of requests received to check if a bucket exists
- `directory_object_list_req`: number of requests received to list objects in a bucket
- `directory_object_put_req`: number of requests received to create an object
- `directory_object_get_req`: number of requests received to retrieve an object
- `directory_object_delete_req`: number of requests received to delete an object 

### Entrypoint service requests (external, S3 protocol):

- `entrypoint_abort_multipart_req`: number of [`AbortMultipartUpload`](https://docs.aws.amazon.com/AmazonS3/latest/API/API_AbortMultipartUpload.html) requests received 
- `entrypoint_complete_multipart_req`: number of [`CompleteMultipartUpload`](https://docs.aws.amazon.com/AmazonS3/latest/API/API_CompleteMultipartUpload.html) requests received
- `entrypoint_create_bucket_req`: number of [`CreateBucket`](https://docs.aws.amazon.com/AmazonS3/latest/API/API_control_CreateBucket.html) requests received
- `entrypoint_delete_bucket_req`: number of [`DeleteBucket`](https://docs.aws.amazon.com/AmazonS3/latest/API/API_control_DeleteBucket.html) requests received
- `entrypoint_delete_object_req`: number of [`DeleteObject`](https://docs.aws.amazon.com/AmazonS3/latest/API/API_DeleteObject.html) requests received
- `entrypoint_delete_objects_req`: number of [`DeleteObjects`](https://docs.aws.amazon.com/AmazonS3/latest/API/API_DeleteObjects.html) requests received
- `entrypoint_get_bucket_req`: number of [`GetBucket`](https://docs.aws.amazon.com/AmazonS3/latest/API/API_control_GetBucket.html) requests received
- `entrypoint_get_object_req`: number of [`GetObject`](https://docs.aws.amazon.com/AmazonS3/latest/API/API_GetObject.html) requests received
- `entrypoint_init_multipart_req`: number of [`CreateMultipartUpload`](https://docs.aws.amazon.com/AmazonS3/latest/API/API_CreateMultipartUpload.html) requests received
- `entrypoint_list_buckets_req`: number of [`ListBuckets`](https://docs.aws.amazon.com/AmazonS3/latest/API/API_ListBuckets.html) requests received
- `entrypoint_list_multipart_req`: number of [`ListMultipartUploads`](https://docs.aws.amazon.com/AmazonS3/latest/API/API_ListMultipartUploads.html) requests received
- `entrypoint_list_objects_req`: number of [`ListObjects`](https://docs.aws.amazon.com/AmazonS3/latest/API/API_ListObjects.html) requests received
- `entrypoint_list_objects_v2_req`: number of [`ListObjectsV2`](https://docs.aws.amazon.com/AmazonS3/latest/API/API_ListObjectsV2.html) requests received
- `entrypoint_multipart_req`: number of [`UploadPart`](https://docs.aws.amazon.com/AmazonS3/latest/API/API_UploadPart.html) requests received
- `entrypoint_put_object_req`: number of [`PutObject`](https://docs.aws.amazon.com/AmazonS3/latest/API/API_PutObject.html) requests received

### Common response messages

- `success`: Operation successful
- `failure`: Operation failed

## Utilization Metrics

### Counters ###
- `gdv_l1_cache_hit_counter`: Hit count of the L1 cache in the `global_data_view`
- `gdv_l1_cache_miss_counter`: Miss count of the L1 cache in the `global_data_view`
- `gdv_l2_cache_hit_counter`: Hit count of the L2 cache in the `global_data_view`
- `gdv_l2_cache_miss_counter`: Miss count of the L2 cache in the `global_data_view`
- `deduplicator_set_fragment_counter`: The number of fragments pointed in the deduplicator set maintained by the `deduplicator service`
- `deduplicator_set_fragment_size_counter`: The aggregated size of fragments pointed in the deduplicator set maintained by the `deduplicator service`
- `entrypoint_ingested_data_counter`: The total data volume ingested by a `entrypoint service`
- `entrypoint_egressed_data_counter`: The total data volume egressed by a `entrypoint service`
- 
### Gauges ###
- `directory_deduplicated_data_volume_gauge`: The deduplicated data volume in the storage cluster, maintained by the `directory service`
- `directory_original_data_volume_gauge`: The original/raw data volume in the storage cluster, maintained by the `directory service`
- `storage_available_space_gauge`: Storage space available to a `storage service` instance
- `storage_used_space_gauge`: Storage space used by a `storage service` instance
