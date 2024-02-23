#!/bin/bash

[ -z "$UH_TEST_BASE" ] && {
    echo "UH_TEST_BASE environment variable is not set"
}

. $UH_TEST_BASE/config

if ! docker pull $UH_IMAGE_BUILD_BASE; then
    echo "pulling build-base image failed" 1>&2
    exit 1
fi

if docker image inspect $UH_IMAGE_TEST_TAG &> /dev/null; then
    docker image rm --force $UH_IMAGE_TEST_TAG
fi

if ! docker build --no-cache --build-arg DebugTools=True --build-arg BuildType=RelWithDebInfo --file $UH_IMAGE_DOCKER_FILE --tag $UH_IMAGE_TEST_TAG $UH_CORE_ROOT; then
    echo "docker build failed" 1>&2
    exit 1
fi
