#!/bin/bash

### TESTS SERVICE STARTUP WITH INVALID LICENSES

INVALID_LICENSE_KEY=("" "invalid_key")
services=("entrypoint" "storage" "deduplicator" "directory")

echo "*** testing service startup with invalid licenses ..."

for service in "${services[@]}"; do
  echo -n "$service : "
  for invalid_license in "${INVALID_LICENSE_KEY[@]}"; do
    export UH_LICENSE=$invalid_license
    docker compose up --no-deps --abort-on-container-exit "$service" > /dev/null 2>&1

    if docker compose logs "$service" | grep -q -e "error in license" -e "license is required"; then
        echo -n "passed "
    else
        echo "failed"
        exit 1
    fi
  done
  echo
done
