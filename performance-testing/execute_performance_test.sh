#!/bin/bash

TEST_MANIFEST_PATH=$1
ENVIRONMENT_NAME=$2
TEST_TIMEOUT=$3

# Run the performance test and wait until it finishes
kubectl apply -f $TEST_MANIFEST_PATH -n $ENVIRONMENT_NAME
kubectl wait --for=condition=ready=true --timeout=10m -n $ENVIRONMENT_NAME pod/performance-tester
kubectl wait --for=condition=ready=false --timeout=$TEST_TIMEOUT -n $ENVIRONMENT_NAME pod/performance-tester
          
# Obtain the performance testing logs
kubectl logs pod/performance-tester -n $ENVIRONMENT_NAME

# Remove the tester pod
kubectl delete -f $TEST_MANIFEST_PATH -n $ENVIRONMENT_NAME