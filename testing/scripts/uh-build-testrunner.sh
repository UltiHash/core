#!/bin/bash

[ -z "$UH_TEST_BASE" ] && {
    echo "UH_TEST_BASE environment variable is not set"
}

. $UH_TEST_BASE/activate

if docker image inspect $UH_IMAGE_RUNNER_TAG &> /dev/null; then
    docker image rm --force $UH_IMAGE_RUNNER_TAG
fi

if ! docker build --file $UH_RUNNER_DOCKER_FILE --tag $UH_IMAGE_RUNNER_TAG $UH_TEST_BASE; then
    echo "docker build failed" 1>&2
    exit 1
fi
