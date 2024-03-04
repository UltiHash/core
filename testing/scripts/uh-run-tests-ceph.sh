#!/bin/bash

print_help()
{
    echo "uh-run-tests-ceph.sh [options] [-- pytest options]"
    echo
    echo "options are:"
    echo " -h, --help           print this text"
    echo " -u URL, --url URL    run tests against an existing cluster"
    echo
    echo "pytest options can be seen by running uh-run-tests-ulti.sh -- --help"
}

cluster_url="http://localhost:8080"

while [ -n "$1" ]; do
    if [ "$1" = "--" ]; then
        shift
        break
    fi

    case "$1" in
        -h|--help)  print_help
                    exit 0
                    ;;
        -u|--url)   shift
                    cluster_url="$1"
                    case "$cluster_url" in
                        http://*);;
                        *) cluster_url="http://$cluster_url";;
                    esac
                    ;;
        *)  echo "Unknown parameter '$1'."
            print_help
            exit 1
    esac

    shift
done

[ -z "$UH_TEST_BASE" ] && {
    echo "UH_TEST_BASE environment variable is not set"
}

. $UH_TEST_BASE/activate

cluster_host=$(echo "$cluster_url" | sed -e 's/http:\/\///' -e 's/:[0-9]\+$//')
cluster_port=$(echo "$cluster_url" | grep -oE '[0-9]+$')

echo "*** running Ceph test suite ..."
sed -e "s/%TESTING_S3_HOST%/$cluster_host/" \
    -e "s/%TESTING_S3_PORT%/$cluster_port/" \
    -e "s/%TESTING_AWS_KEY_ID%/$UH_AWS_ACCESS_KEY_ID/" \
    -e "s/%TESTING_AWS_SECRET_ACCESS_KEY%/$UH_AWS_SECRET_ACCESS_KEY/" \
    < $UH_SAMPLE_CEPH_S3TESTS_CONF > $UH_CEPH_CONF

docker run --network="host" --interactive --tty \
    --env S3TEST_CONF=/s3test.conf \
    --env LOCAL_UID=$(id -u $USER) --env LOCAL_GID=$(id -g $USER) \
    --volume $UH_CEPH_CONF:/s3test.conf \
    --volume $UH_TEST_SUITE_CEPH:/tests:rw \
    $UH_IMAGE_RUNNER_TAG \
    pytest /tests -m uhclustertest $@
