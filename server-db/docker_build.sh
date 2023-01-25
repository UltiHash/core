#!/bin/bash

docker build --tag ghcr.io/ultihash/server-db:$(printf '%(%Y%m%d)T' -1)-nightly .
