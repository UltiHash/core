#!/bin/bash

echo "Waiting for uh-server-agency to become available..."
RAND=$(( ( RANDOM % 5 )  + 1 ))
sleep $RAND

while true
do
  STDOUT=$(curl -s -f -i http://uh-server-agency:8080/metrics)
  ERRNO="$?"
  if [ "$ERRNO" -eq 0 ] ; then
        echo "uh-server-agency has become available, moving on..."
        break   # end loop
  fi
  RAND=$(( ( RANDOM % 5 )  + 5 ))
  echo "uh-server-agency is not available, retrying in $RAND seconds..."
  sleep $RAND
done

# exit when any command fails
set -e

# keep track of the last executed command
trap 'last_command=$current_command; current_command=$BASH_COMMAND' DEBUG
# echo an error message before exiting
trap 'echo "\"${last_command}\" command filed with exit code $?."' EXIT

# prepare crude test workload, using the uhClient binary itself
mkdir -p test
cp /usr/local/bin/uhClient test

# store checksums of test workload, integrate data into UltiHash volume and delete files afterwards
sha512sum test/uhClient > checksum.txt
uhClient -i test.uh test -a uh-server-agency:21832 -M
rm -Rf test

# retrieve test workload and validate their checksums
yes | uhClient -r -T ./ ./test.uh -a uh-server-agency:21832 -M
cat checksum.txt | sha512sum -c

# cleanup
rm -Rf test*
rm -f checksum.txt

sleep infinity