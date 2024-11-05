#!/bin/bash

TEST_MANIFEST_PATH=$1
POD_NAME=$2
ENVIRONMENT_NAME=$3
TEST_TIMEOUT=$4

# Run the performance test and wait until it finishes
kubectl apply -f $TEST_MANIFEST_PATH -n $ENVIRONMENT_NAME
kubectl wait --for=condition=ready=true --timeout=10m -n $ENVIRONMENT_NAME pod/$POD_NAME
kubectl wait --for=condition=ready=false --timeout=$TEST_TIMEOUT -n $ENVIRONMENT_NAME pod/$POD_NAME
          
# Obtain the performance testing logs
kubectl logs pod/$POD_NAME -n $ENVIRONMENT_NAME

# Remove the tester pod
kubectl delete -f $TEST_MANIFEST_PATH -n $ENVIRONMENT_NAME