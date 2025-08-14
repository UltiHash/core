#!/bin/bash

set -e

while true; do
  RESPONSE=$(curl -s "$CLUSTER_URL/ultihash/v1/ready")
  READY=$(echo "$RESPONSE" | grep -o '"ready": true')

  if [ -n "$READY" ]; then
    echo "UltiCache seems to be ready at $CLUSTER_URL!"
    break
  fi

  echo "Waiting for UltiCache to become ready at $CLUSTER_URL ..."
  sleep 5
done