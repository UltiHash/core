#!/bin/bash

TEST_MANIFEST_PATH=$1
POD_NAME=$2
ENVIRONMENT_NAME=$3
TEST_TIMEOUT=$4  # e.g., 3h, 30m, or 1800s

# Helper to print pod logs
print_logs() {
  echo "--- Logs from pod $POD_NAME ---"
  kubectl logs "$POD_NAME" -n "$ENVIRONMENT_NAME" || true
}

# Helper to clean up the pod
cleanup() {
  echo "Cleaning up pod $POD_NAME..."
  kubectl delete -f "$TEST_MANIFEST_PATH" -n "$ENVIRONMENT_NAME"
}

# Convert timeout to seconds (supports h/m/s suffixes)
convert_to_seconds() {
  local value="$1"
  echo "$value" | awk '
    /h$/ { gsub("h", ""); print $1 * 3600; next }
    /m$/ { gsub("m", ""); print $1 * 60; next }
    /s$/ { gsub("s", ""); print $1; next }
    /^[0-9]+$/ { print $1; next }
    { print 0 }
  '
}

# Deploy the pod
kubectl apply -f "$TEST_MANIFEST_PATH" -n "$ENVIRONMENT_NAME"

# Wait for pod to be ready
echo "Waiting for pod $POD_NAME to be ready..."
kubectl wait --for=condition=Ready --timeout=10m -n "$ENVIRONMENT_NAME" pod/"$POD_NAME"

# Wait for pod to complete
echo "Waiting for pod $POD_NAME to complete (timeout: $TEST_TIMEOUT)..."
SECONDS=0
TIMEOUT_SECONDS=$(convert_to_seconds "$TEST_TIMEOUT")

while true; do
  PHASE=$(kubectl get pod "$POD_NAME" -n "$ENVIRONMENT_NAME" -o jsonpath='{.status.phase}')
  
  if [[ "$PHASE" == "Succeeded" ]]; then
    echo "Pod $POD_NAME completed successfully."
    print_logs
    cleanup
    break
  elif [[ "$PHASE" == "Failed" ]]; then
    echo "Pod $POD_NAME failed."
    print_logs
    cleanup
    exit 1
  fi

  if (( SECONDS >= TIMEOUT_SECONDS )); then
    echo "Timeout reached: pod $POD_NAME did not finish in time."
    print_logs
    cleanup
    exit 1
  fi

  sleep 5
done