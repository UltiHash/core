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

mkdir -p test
cp /usr/local/bin/uhClient -i test.uh test -a uh-server-agency:21832 -M
sha512sum test/uhClient > checksum.txt
uhClient write test

rm -Rf test
yes | uhClient read -r ./test.uh -a localhost:21832 -M
mv test test_reconstructed
mv test_reconstructed/test .
rm -Rf test_reconstructed

cat checksum.txt | sha512sum -c
rm -Rf test*

sleep infinity