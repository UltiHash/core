#!/bin/bash

docker buildx build --no-cache --tag ghcr.io/ultihash/database-init:gmt --platform linux/amd64 -f tools/gmt-setup/Dockerfile-database-init . --push
docker buildx build --no-cache --tag ghcr.io/ultihash/core:gmt-storage-0.6.0 --platform linux/amd64 -f tools/gmt-setup/Dockerfile-storage . --push
docker buildx build --no-cache --tag ghcr.io/ultihash/core:gmt-deduplicator-0.6.0 --platform linux/amd64 -f tools/gmt-setup/Dockerfile-deduplicator . --push
docker buildx build --no-cache --tag ghcr.io/ultihash/core:gmt-entrypoint-0.6.0 --platform linux/amd64 -f tools/gmt-setup/Dockerfile-entrypoint . --push
