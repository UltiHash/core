#!/bin/bash

if [ -z "$RUN_AS_ACTION" ]
then
  docker-compose -f docker-compose-integration.yml up -d --build
  ./wait_for_benchmark.py
fi

docker-compose logs > logs.txt
docker-compose stop
yes | docker-compose rm

RESULT=$(cat logs.txt | grep uh-client-shell | tail -n 1 | grep OK)

if [ $? -eq 0 ]
then
  cat logs.txt
  echo "Integration test passed."
else
  cat logs.txt
  echo "Integration test failed."
  exit 1
fi