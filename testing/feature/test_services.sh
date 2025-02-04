#!/bin/bash

### TESTS SERVICE STARTUP WITH INVALID LICENSES

DOCKER_IMAGE=$1
INVALID_LICENSE_KEY=("" "invalid_key")
services=("entrypoint" "storage" "deduplicator")

echo "*** testing service startup with invalid licenses ..."

if [ -z "$DOCKER_IMAGE" ]; then
    echo "usage: $0 <docker-image>"
    exit 1
fi

for service in "${services[@]}"; do

  echo -n "$service : "

  for invalid_license in "${INVALID_LICENSE_KEY[@]}"; do
    export UH_LICENSE_JSON=$invalid_license
    if docker run --rm -e UH_LICENSE_JSON $DOCKER_IMAGE uh-cluster --registry mocked-etcd:1111 $service 2>&1 | \
        grep -q -e "license loaded for"; then
      echo "failed"
      exit 1
    else
      echo -n "passed "
    fi
  done
  echo
done
