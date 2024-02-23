#!/bin/bash

[ -z "$UH_TEST_BASE" ] && {
    echo "UH_TEST_BASE environment variable is not set"
}

. $UH_TEST_BASE/config

docker compose --project-directory $UH_TEST_BASE rm --volumes --stop --force
