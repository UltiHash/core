#!/bin/bash
set -e

docker build --build-arg CMAKE_OPTION=BUILD_SERVER_DB --build-arg SRC_PATH=server-db --build-arg TARGET=uh-data-node --tag ghcr.io/ultihash/server-db:$(printf '%(%Y%m%d)T' -1) .
docker build --build-arg CMAKE_OPTION=BUILD_SERVER_AGENCY --build-arg SRC_PATH=server-agency --build-arg TARGET=uh-agency-node --tag ghcr.io/ultihash/server-agency:$(printf '%(%Y%m%d)T' -1) .
docker build --build-arg CMAKE_OPTION=BUILD_CLIENT_SHELL --build-arg SRC_PATH=client-shell --build-arg TARGET=uh-cli --tag ghcr.io/ultihash/client-shell:$(printf '%(%Y%m%d)T' -1) .
