# Copyright 2026 UltiHash Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

add_library(entrypoint
            garbage_collector.cpp
            object.cpp
            limits.cpp
            cors/parser.cpp
            cors/module.cpp
            handler.cpp
            commands/iam/list_user_policies.cpp
            commands/iam/delete_access_key.cpp
            commands/iam/put_user_policy.cpp
            commands/iam/delete_user_policy.cpp
            commands/iam/create_access_key.cpp
            commands/iam/create_user.cpp
            commands/iam/delete_user.cpp
            commands/iam/get_user_policy.cpp
            commands/uh/get_license_info.cpp
            commands/uh/get_ready.cpp
            commands/uh/get_metrics.cpp
            commands/s3/put_bucket_cors.cpp
            commands/s3/list_objects.cpp
            commands/s3/list_object_versions.cpp
            commands/s3/get_bucket_policy.cpp
            commands/s3/put_object.cpp
            commands/s3/get_bucket_cors.cpp
            commands/s3/head_bucket.cpp
            commands/s3/list_buckets.cpp
            commands/s3/head_object.cpp
            commands/s3/delete_bucket.cpp
            commands/s3/get_object.cpp
            commands/s3/delete_objects.cpp
            commands/s3/multipart.cpp
            commands/s3/delete_bucket_policy.cpp
            commands/s3/delete_bucket_cors.cpp
            commands/s3/list_objects_v2.cpp
            commands/s3/init_multipart.cpp
            commands/s3/abort_multipart.cpp
            commands/s3/copy_object.cpp
            commands/s3/delete_object.cpp
            commands/s3/list_multipart.cpp
            commands/s3/create_bucket.cpp
            commands/s3/put_bucket_policy.cpp
            commands/s3/complete_multipart.cpp
            commands/s3/put_bucket_versioning.cpp
            commands/s3/get_bucket_versioning.cpp
            http/stream.cpp
            http/range.cpp
            http/raw_body.cpp
            http/string_body.cpp
            http/response.cpp
            http/chunked_body.cpp
            http/basic_auth.cpp
            http/raw_request.cpp
            http/aws4_hmac_sha256.cpp
            http/no_auth.cpp
            http/request_factory.cpp
            http/chunk_body_sha256.cpp
            http/request.cpp
            http/command_exception.cpp
            http/raw_body_sha256.cpp
            formats.cpp
            multipart_state.cpp
            directory.cpp
            service.cpp
            user/db.cpp
            aws/arn.cpp
            command_factory.cpp
            utils.cpp
            policy/policy.cpp
            policy/parser.cpp
            policy/module.cpp
            policy/variables.cpp)

target_link_libraries(entrypoint db etcd nlohmann_json deduplicator storage uhcrypto utils license)
