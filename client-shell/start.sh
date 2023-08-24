#!/bin/bash

ncat -e /bin/cat -k -u -l 1337 &

SERVER_NAME="uh-server-agency"
SERVER_PORT=21832
TIMEOUT="60"  # empty for infinite

echo "Waiting for ${SERVER_NAME}:${SERVER_PORT} to become available..."

#proper health check by sending a hello message
#echo -e "\x01\x01\x01\x01\x07\x6e\x65\x74\x63\x61\x74\x0a" | nc -q 1 172.18.0.5 12345

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

# exit when any command fails
set -e

# keep track of the last executed command
trap 'last_command=$current_command; current_command=$BASH_COMMAND' DEBUG
# echo an error message before exiting
trap 'echo "\"${last_command}\" command filed with exit code $?."' EXIT

# prepare crude test workload, using the uh-cli binary itself
mkdir -p test
cp /usr/local/bin/uh-cli test

# store checksums of test workload, integrate data into UltiHash volume and delete files afterwards
sha512sum test/uh-cli > checksum.txt
uh-cli --integrate test.uh test --agency-node ${SERVER_NAME}:${SERVER_PORT}
rm -Rf test

# retrieve test workload and validate their checksums
mkdir test
yes | uh-cli --retrieve test.uh --target ./test --agency-node ${SERVER_NAME}:${SERVER_PORT}
cat checksum.txt | sha512sum -c

# cleanup
rm -Rf test*
rm -f checksum.txt

pkill ncat
sleep infinity
