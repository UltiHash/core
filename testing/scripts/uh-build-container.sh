#!/bin/bash

[ -z "$UH_TEST_BASE" ] && {
    echo "UH_TEST_BASE environment variable is not set"
}

. $UH_TEST_BASE/activate

print_help()
{
    echo "uh-build-container.sh [options]"
    echo
    echo "Build a debug version of the service base container"
    echo
    echo "options are:"
    echo " -h, --help           print this text"
    echo " -c, --cached         allow caching build results"
}

cached="--no-cache"

while [ -n "$1" ]; do

    case "$1" in
        -h|--help)  print_help
                    exit 0
                    ;;
        -c|--cached)   cached=""
                    ;;
        *)  service="$1"
            ;;
    esac

    shift
done

if ! docker pull $UH_IMAGE_BUILD_BASE; then
    echo "pulling build-base image failed" 1>&2
    exit 1
fi

if docker image inspect $UH_IMAGE_TEST_TAG &> /dev/null; then
    docker image rm --force $UH_IMAGE_TEST_TAG
fi

if ! docker build $cached --build-arg DebugTools=True --build-arg BuildType=RelWithDebInfo --file $UH_IMAGE_DOCKER_FILE --tag $UH_IMAGE_TEST_TAG $UH_CORE_ROOT; then
    echo "docker build failed" 1>&2
    exit 1
fi
