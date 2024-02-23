#!/bin/bash

print_help()
{
    echo "uh-run-tests-ulti.sh [options] [-- pytest options]"
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

. $UH_TEST_BASE/config

echo "*** running UltiHash test suite ..."

pytest "tests" --cluster-url="$cluster_url" \
    --aws-access-key-id="$UH_AWS_ACCESS_KEY_ID" --aws-secret-access-key="$UH_AWS_SECRET_ACCESS_KEY" $@
