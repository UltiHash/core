add_library(uh_commands multipart.cpp list_objects.cpp init_multipart.cpp delete_object.cpp list_buckets.cpp get_bucket_policy.cpp put_bucket_policy.cpp head_object.cpp complete_multipart.cpp abort_multipart.cpp head_bucket.cpp delete_bucket.cpp delete_bucket_policy.cpp create_bucket.cpp get_object.cpp put_object.cpp copy_object.cpp list_objects_v2.cpp delete_objects.cpp get_metrics.cpp list_multipart.cpp)
target_link_libraries(uh_commands uh_telemetry uh_http uh_service_interfaces uh_crypto uh_utils uh_global_data uh_entrypoint)
add_subdirectory(iam)
