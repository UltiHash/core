#!/bin/bash

UH_TEST_BASE=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
export UH_TEST_BASE

. $UH_TEST_BASE/activate

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
    echo " -U, --run-ulti       run UltiHash test suite"
    echo " -C, --run-ceph       run Ceph test suite"
}

set -o errexit

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
        -U|--run-ulti) run_ceph=0
                      ;;
        -C|--run-ceph) run_ultihash=0
                      ;;
        *)  echo "Unknown parameter '$1'."
            print_help
            exit 1
    esac

    shift
done

echo "*** running start-e2e.sh on $(hostname --all-fqdns)"
echo "*** uname: $(uname -a)"
echo "*** id: $(id -a)"
echo "*** pwd: $PWD, pid: $BASHPID"
TERM=vt100 pstree -H $BASHPID

if [ -z "$cluster_url" ]; then
    uh-build-container.sh

    ./test_services.sh
    uh-container-start.sh

    trap "uh-container-stop.sh" SIGHUP SIGINT SIGQUIT SIGABRT EXIT
    cluster_url="http://localhost:8080"
fi

uh-build-testrunner.sh

set +o errexit

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
    uh-run-tests-ulti.sh --url "$cluster_url" $@
    if [ "$?" != "0" ]; then
        success=0
    fi
fi

if [ "$run_ceph" -eq "1" ]; then
    uh-run-tests-ceph.sh --url "$cluster_url" $@
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
