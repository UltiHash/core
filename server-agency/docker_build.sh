#!/bin/bash

docker build --tag ghcr.io/ultihash/server-agency:$(printf '%(%Y%m%d)T' -1) .
