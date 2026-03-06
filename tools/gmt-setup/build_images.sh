#!/bin/bash
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


docker buildx build --no-cache --tag ghcr.io/ultihash/database-init:gmt --platform linux/amd64 -f tools/gmt-setup/Dockerfile-database-init . --push
docker buildx build --no-cache --tag ghcr.io/ultihash/core:gmt-storage-0.6.0 --platform linux/amd64 -f tools/gmt-setup/Dockerfile-storage . --push
docker buildx build --no-cache --tag ghcr.io/ultihash/core:gmt-deduplicator-0.6.0 --platform linux/amd64 -f tools/gmt-setup/Dockerfile-deduplicator . --push
docker buildx build --no-cache --tag ghcr.io/ultihash/core:gmt-entrypoint-0.6.0 --platform linux/amd64 -f tools/gmt-setup/Dockerfile-entrypoint . --push
