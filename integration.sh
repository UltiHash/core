#!/bin/bash

if [ -z "$RUN_AS_ACTION" ]
then
  docker compose -p integration -f docker-compose-integration.yml up -d --build
  ./wait_for_benchmark.py
fi

docker compose -p integration logs > logs.txt

if [ -z "$RUN_AS_ACTION" ]
then
  docker compose -p integration stop
  docker compose -p integration rm -f
fi

RESULT=$(grep integration-uh-cli logs.txt | tail -n 1 | grep OK)

if [ $? -eq 0 ]
then
  cat logs.txt
  echo "Integration test passed."
else
  cat logs.txt
  echo "Integration test failed."
  exit 1
fi