#!/bin/bash

venv_dir="$(pwd)/.venv"
requirements_file="$(pwd)/python-requirements.txt"
AWS_ACCESS_KEY_ID="aws_access_key_id"
AWS_SECRET_ACCESS_KEY="aws_secret_access_key"
SAMPLE_CEPH_S3TESTS_CONF="$(pwd)/ceph-s3tests.conf.in"
CEPH_CONF="$(pwd)/ceph_s3_test.conf"

cluster_url=""
run_ultihash=1
run_ceph=1

print_help()
{
    echo "start-e2e.sh [options] -- [py_test options]"
    echo
    echo "options are:"
    echo " -h, --help           print this text"
    echo " -u URL, --url URL    run tests against an existing cluster"
    echo " -U, --no-ulti        do not run UltiHash test suite"
    echo " -C, --no-ceph        do not run Ceph test suite"
}

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
        -U|--no-ulti) run_ultihash=0
                      ;;
        -C|--no-ceph) run_ceph=0
                      ;;
        *)  echo "Unknown parameter '$1'."
            print_help
            exit 1
    esac

    shift
done

if [ "$(basename $(pwd))" != "testing" ]; then
    echo "Script is not executed from the testing directory, exiting..."
    exit 1
fi

docker build --no-cache --file ../Dockerfile --tag uh-cluster:testing ..

if [ ! -d "$venv_dir" ] || [ "$venv_dir" -ot "$requirements_file" ]; then
    echo "Creating virtual environment ..."

    python3 -m venv "$venv_dir"

    . "$venv_dir/bin/activate"

    pip --require-virtualenv install --requirement "$requirements_file" || {
        deactivate
        rm -rf "$venv_dir"
        exit 1
    }

    deactivate
    touch "$venv_dir"
fi

. "$venv_dir/bin/activate"


if [ -z "$cluster_url" ]; then
    docker compose up --detach
    trap "docker-compose down" SIGHUP SIGINT SIGQUIT SIGABRT EXIT
    cluster_url="http://localhost:8080"
fi

timeout=60
while [ "$timeout" -gt "0" ]; do

    if curl --silent --output /dev/null $cluster_url; then
        break
    fi

    timeout=$((timeout - 1))
    sleep 1
done

if [ "$timeout" -eq "0" ]; then
    echo "connection to cluster URL failed"
    exit 1
fi

export PYTHONDONTWRITEBYTECODE=1

success=1

if [ "$run_ultihash" -eq "1" ]; then
    echo "*** running UltiHash test suite ..."
    pytest "tests" --cluster-url="$cluster_url" \
        --aws-access-key-id="$AWS_ACCESS_KEY_ID" --aws-secret-access-key="$AWS_SECRET_ACCESS_KEY" $@
    if [ "$?" != "0" ]; then
        success=0
    fi
fi

if [ "$run_ceph" -eq "1" ]; then
    cluster_host=$(echo "$cluster_url" | sed -e 's/http:\/\///' -e 's/:[0-9]\+$//')
    cluster_port=$(echo "$cluster_url" | grep -oE '[0-9]+$')

    echo "*** running Ceph test suite ..."
    sed -e "s/%TESTING_S3_HOST%/$cluster_host/" \
        -e "s/%TESTING_S3_PORT%/$cluster_port/" \
        -e "s/%TESTING_AWS_KEY_ID%/$AWS_ACCESS_KEY_ID/" \
        -e "s/%TESTING_AWS_SECRET_ACCESS_KEY%/$AWS_SECRET_ACCESS_KEY/" \
        < $SAMPLE_CEPH_S3TESTS_CONF > $CEPH_CONF

    S3TEST_CONF=$CEPH_CONF tox -c "s3-tests/tox.ini" --workdir "s3-tests" -- -m 'uhclustertest'
    if [ "$?" != "0" ]; then
        success=0
    fi
fi

if [ "$success" == "1" ]; then
    echo "*** all tests passed"
    exit 0
fi

echo "*** there are test errors"
exit 1
