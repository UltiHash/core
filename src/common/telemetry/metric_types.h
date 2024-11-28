#pragma once

namespace uh::cluster {

enum metric_type {
    gdv_l1_cache_hit_counter,
    gdv_l1_cache_miss_counter,
    gdv_l2_cache_hit_counter,
    gdv_l2_cache_miss_counter,
    deduplicator_set_fragment_counter,
    deduplicator_set_fragment_size_counter,
    entrypoint_ingested_data_counter,
    entrypoint_egressed_data_counter,
    entrypoint_original_data_volume_gauge,
    storage_available_space_gauge,
    storage_used_space_gauge,
    storage_read_fragment_req,
    storage_read_address_req,
    storage_read_req,
    storage_write_req,
    storage_sync_req,
    storage_link_req,
    storage_unlink_req,
    storage_used_req,
    deduplicator_req,
    entrypoint_abort_multipart_req,
    entrypoint_complete_multipart_req,
    entrypoint_create_bucket_req,
    entrypoint_delete_bucket_req,
    entrypoint_delete_object_req,
    entrypoint_delete_objects_req,
    entrypoint_get_metrics_req,
    entrypoint_get_object_req,
    entrypoint_head_object_req,
    entrypoint_init_multipart_req,
    entrypoint_list_buckets_req,
    entrypoint_list_multipart_req,
    entrypoint_list_objects_req,
    entrypoint_list_objects_v2_req,
    entrypoint_multipart_req,
    entrypoint_put_object_req,
    active_connections,
    success,
    failure
};

enum metric_unit {
    count,
    byte,
    mebibyte,
};

} // namespace uh::cluster
