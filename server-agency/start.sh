#!/bin/bash

SERVER_NAME="uh-data-node"
SERVER_PORT=12345
TIMEOUT="60"  # empty for infinite

echo "Waiting for ${SERVER_NAME}:${SERVER_PORT} to become available..."

sleep 15

timeout=$TIMEOUT
while ! ncat -z ${SERVER_NAME} ${SERVER_PORT}; do
    if [ "$timeout" = "0" ]; then
        echo "Waiting for server ${SERVER_NAME}:${SERVER_PORT} failed"
        exit 1
    fi

    if [ -n "$timeout" ]; then
        timeout=$((timeout - 1))
    fi
    sleep 1;
done

echo "uh-data-node has become available, moving on..."
uh-agency-node --db-node ${SERVER_NAME}:${SERVER_PORT}
