# Overview

This document defines the metric parameters implemented by UltiHash
cluster.

## Internal Message-Level Metrics

Each servce measures the count of different internal message types it sends or receives. These are as follows:

### Storage Service Messages:
- `storage_read_fragment_req`: a request sent to the storage service for reading a fragment
- `storage_read_address_req`: a request sent to the storage service for reading an address
- `storage_write_req`: a request sent to the storage service for writing data
- `storage_sync_req`: a request sent to the storage service to sync data to disk
- `storage_remove_fragment_req`: a request sent to the storage service to sync data to disk
- `storage_used_req`: a request sent to the storage service to get the used space

### Deduplicator Service Messages:

- `deduplicator_req`: a request sent to deduplicator to deduplicate data

### Directory Service Messages:

- `directory_bucket_list_req`: a request sent to the directory service to list buckets
- `directory_bucket_put_req`: a request sent to the directory service to insert a bucket
- `directory_bucket_delete_req`: a request sent to the directory service to delete a bucket
- `directory_bucket_exists_req`: a request sent to the directory service to check if a bucket exists
- `directory_object_list_req`: a request sent to the directory service to list objects in a bucket
- `directory_object_put_req`: a request sent to the directory service to integrate an object
- `directory_object_get_req`: a request sent to the directory service to retrieve an object
- `directory_object_delete_req`: a request sent to the directory service to delete an object 

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
- `total_ingested_size_mb`: The total digested data
- `total_egressed_size_mb`: The total retrieved data
- `total_effective_size_mb`: The total effective size
- `total_size_mb`: The total data size (considering insertions and deletions)