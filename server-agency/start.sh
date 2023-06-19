#!/bin/bash

echo "Waiting for uh-server-db to become available..."
RAND=$(( ( RANDOM % 5 )  + 40 ))
sleep $RAND

while true
do
  STDOUT=$(curl -s -f -i http://uh-server-db:8080/metrics)
  ERRNO="$?"
  if [ "$ERRNO" -eq 0 ] ; then
        echo "uh-server-db has become available, moving on..."
        break   # end loop
  fi
  RAND=$(( ( RANDOM % 5 )  + 5 ))
  echo "uh-server-db is not available, retrying in $RAND seconds..."
  sleep $RAND
done

uh-agency-node --db-node uh-server-db:12345 --activate-replace "GZHG-MWDK-DHNK-2J02"