#!/bin/bash

loops=3
BENCH_NETWORK_NAME="nightlyBench_net"
CREATE_NETWORK="docker network create --scope swarm --driver overlay ${BENCH_NETWORK_NAME}"
CREATE_COMMON="docker service create --network ${BENCH_NETWORK_NAME} --mode global"

CREATE_DB_SERVICE="${CREATE_COMMON} --name nightlyBench_database --hostname uh-server-db --constraint node.labels.role==database ghcr.io/ultihash/server-db:$(printf '%(%Y%m%d)T' -1)"
CREATE_AGENCY_SERVICE="${CREATE_COMMON} --name nightlyBench_agency --hostname uh-server-agency --constraint node.labels.role==agency ghcr.io/ultihash/server-agency:$(printf '%(%Y%m%d)T' -1)"
CREATE_CLIENT_SERVICE="${CREATE_COMMON} --name nightlyBench_client --hostname uh-client-shell --mount type=bind,source=/data,destination=/data --entrypoint '/data/benchmark_container.sh yt8m-nightly' --publish 1337:1337/udp --constraint node.labels.role==agency ghcr.io/ultihash/client-shell:$(printf '%(%Y%m%d)T' -1)"

WAIT_SCRIPT="./wait_for_benchmark.py"

PRINT_LOG="docker service logs nightlyBench_client --raw"


RM_NETWORK="docker network rm ${BENCH_NETWORK_NAME}"
RM_DB_SERVICE="docker service rm nightlyBench_database"
RM_AGENCY_SERVICE="docker service rm nightlyBench_agency"
RM_CLIENT_SERVICE="docker service rm nightlyBench_client"


#VERBOSITY=">/dev/null 2>&1"

echo "Make sure no old benchmark services may interfere..."
eval ${RM_CLIENT_SERVICE} ${VERBOSITY}
eval ${RM_AGENCY_SERVICE} ${VERBOSITY}
eval ${RM_DB_SERVICE} ${VERBOSITY}
eval ${RM_NETWORK} ${VERBOSITY}
sleep 10

for (( i=0; i<${loops}; ++i)); do
	echo "Initiating benchmark setup..."
	eval ${CREATE_NETWORK} ${VERBOSITY}
	eval ${CREATE_DB_SERVICE} ${VERBOSITY}
	eval ${CREATE_AGENCY_SERVICE} ${VERBOSITY}
	eval ${CREATE_CLIENT_SERVICE} ${VERBOSITY}

	echo "Instantiated benchmark setup, now waiting for benchmark to complete..."
	eval ${WAIT_SCRIPT}
	eval ${PRINT_LOG} > /data/logs/benchmark_$(printf '%(%Y%m%d)T' -1)_yt8m-nightly_${i}.log

	echo "Benchmark complete, cleaning up setup..."
	eval ${RM_CLIENT_SERVICE} ${VERBOSITY}
	eval ${RM_AGENCY_SERVICE} ${VERBOSITY} 
	eval ${RM_DB_SERVICE} ${VERBOSITY}
	eval ${RM_NETWORK} ${VERBOSITY}
	sleep 30
done
