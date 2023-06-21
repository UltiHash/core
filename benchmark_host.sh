#!/bin/bash

CORPORA_BASE_DIR="/data/corpora"
NUM_LOOPS=5
SLACK_CHANNEL="#1_tech-dev"
#SLACK_CHANNEL="#harry_debug"

#
# Tears down all remaining nightlyBench services
#
# Usage:
#   rm_services
#
teardown()
{
  local services=$(docker service ls | grep nightlyBench_ | awk '{print $2}')
  if [[ -n ${services} ]]; then
    docker service rm ${services} >/dev/null 2>&1
    if [[ -n "$1" ]]; then
      echo "Found left-over benchmark services that may interfere, cleaning up..."
      sleep 15
    fi
  fi

}

#
# Sets up nightlyBench service instances of all UltiHash components involved in the integration test/benchmark
#
# Usage:
#   setup <corpus>
#
setup()
{
  local corpus="$1"; shift;
  local network=$(docker network ls | grep nightlyBench_net | awk '{print $2}')
  if [[ -z ${network} ]]; then
    echo "Docker network \"nightlyBench_net\" didn't exist yet, so it is created now..."
    docker network create --scope swarm --driver overlay nightlyBench_net >/dev/null 2>&1
    sleep 10
  fi
  docker service create --network nightlyBench_net --mode global --name nightlyBench_database --hostname uh-server-db --constraint node.labels.role==database ghcr.io/ultihash/server-db:$(printf '%(%Y%m%d)T' -1) >/dev/null 2>&1
  docker service create --network nightlyBench_net --mode global --name nightlyBench_agency --hostname uh-server-agency --constraint node.labels.role==agency ghcr.io/ultihash/server-agency:$(printf '%(%Y%m%d)T' -1) >/dev/null 2>&1
  docker service create --network nightlyBench_net --mode global --name nightlyBench_client --hostname uh-client-shell --mount type=bind,source=/data,destination=/data --entrypoint "/data/benchmark_container.sh ${corpus}" --publish 1337:1337/udp --constraint node.labels.role==agency ghcr.io/ultihash/client-shell:$(printf '%(%Y%m%d)T' -1) >/dev/null 2>&1
}

#
# Returns the average throughput of the agency_fresh_upload condition from the log files
#
# Usage:
#   get_agency_fresh_upload_throughput <corpus> <log_path_base>
#
get_agency_fresh_upload_throughput()
{
  local corpus="$1"; shift;
  local log_path_base="$1"; shift;
  local log_paths=${log_path_base}*
  RESULT=$(jq -s "map(.agency_fresh_upload.${corpus}.throughput) | add / length" ${log_paths})
  EXIT_STATUS=$?

  if [ $EXIT_STATUS -eq 0 ]; then
    printf "${RESULT}"
    return 0
  else
    printf ""
    return 1
  fi

}

#
# Returns the average deduplication ratio of the agency_fresh_upload condition from the log files
#
# Usage:
#   get_agency_fresh_upload_deduplication <corpus> <log_path_base>
#
get_agency_fresh_upload_deduplication()
{
  local corpus="$1"; shift;
  local log_path_base="$1"; shift;
  local log_paths=${log_path_base}*
  RESULT=$(jq -s "map(.agency_fresh_upload.${corpus}.deduplication) | add / length" ${log_paths})
  EXIT_STATUS=$?

  if [ $EXIT_STATUS -eq 0 ]; then
    printf "${RESULT}"
    return 0
  else
    printf ""
    return 1
  fi

}

#
# Returns the average throughput of the agency_fresh_upload metric from the log files
#
# Usage:
#   get_agency_update_upload <corpus> <log_path_base>
#
get_agency_update_upload_throughput()
{
  local corpus="$1"; shift;
  local log_path_base="$1"; shift;
  local log_paths=${log_path_base}*
  RESULT=$(jq -s "map(.agency_update_upload.${corpus}.throughput) | add / length" ${log_paths})
  EXIT_STATUS=$?

  if [ $EXIT_STATUS -eq 0 ]; then
    printf "${RESULT}"
    return 0
  else
    printf ""
    return 1
  fi

}

#
# Returns the average throughput of the dnode_update_upload metric from the log files
#
# Usage:
#   get_agency_update_upload <corpus> <log_path_base>
#
get_dnode_update_upload_throughput()
{
  local corpus="$1"; shift;
  local log_path_base="$1"; shift;
  local log_paths=${log_path_base}*
  RESULT=$(jq -s "map(.dnode_update_upload.${corpus}.throughput) | add / length" ${log_paths})
  EXIT_STATUS=$?

  if [ $EXIT_STATUS -eq 0 ]; then
    printf "${RESULT}"
    return 0
  else
    printf ""
    return 1
  fi

}

#
# Returns the average throughput of the agency_fresh_upload metric from the log files
#
# Usage:
#   get_agency_download <corpus> <log_path_base>
#
get_agency_download_throughput()
{
  local corpus="$1"; shift;
  local log_path_base="$1"; shift;
  local log_paths=${log_path_base}*
  RESULT=$(jq -s "map(.agency_download.${corpus}.throughput) | add / length" ${log_paths})
  EXIT_STATUS=$?

  if [ $EXIT_STATUS -eq 0 ]; then
    printf "${RESULT}"
    return 0
  else
    printf ""
    return 1
  fi

}

#
# Returns the average throughput of the dnode_download metric from the log files
#
# Usage:
#   get_dnode_download <corpus> <log_path_base>
#
get_dnode_download_throughput()
{
  local corpus="$1"; shift;
  local log_path_base="$1"; shift;
  local log_paths=${log_path_base}*
  RESULT=$(jq -s "map(.dnode_download.${corpus}.throughput) | add / length" ${log_paths})
  EXIT_STATUS=$?

  if [ $EXIT_STATUS -eq 0 ]; then
    printf "${RESULT}"
    return 0
  else
    printf ""
    return 1
  fi

}

#
# Posts benchmark results to Slack channel
#
# Usage:
#   post_results_to_slack <corpus> <log_path_base>
#
post_results_to_slack()
{
  local corpus="$1"; shift;
  local log_path_base="$1"; shift;
  local upload_fresh_throughput=$(get_agency_fresh_upload_throughput "${corpus}" "${log_path_base}")
  upload_fresh_throughput=$(printf "%.2f" ${upload_fresh_throughput})
  local upload_fresh_throughput_exit=$?
  local upload_fresh_deduplication=$(get_agency_fresh_upload_deduplication "${corpus}" "${log_path_base}")
  upload_fresh_deduplication=$(printf "%.2f" ${upload_fresh_deduplication})
  local upload_fresh_deduplication_exit=$?
  local upload_update_throughput=$(get_agency_update_upload_throughput "${corpus}" "${log_path_base}")
  upload_update_throughput=$(printf "%.2f" ${upload_update_throughput})
  local upload_update_throughput_exit=$?
  local download_agency_throughput=$(get_agency_download_throughput "${corpus}" "${log_path_base}")
  download_agency_throughput=$(printf "%.2f" ${download_agency_throughput})
  local download_agency_throughput_exit=$?

  local download_dnode_throughput=$(get_dnode_download_throughput "${corpus}" "${log_path_base}")
  download_dnode_throughput=$(printf "%.2f" ${download_dnode_throughput})
  local download_dnode_throughput_exit=$?

  local upload_dnode_throughput=$(get_dnode_update_upload_throughput "${corpus}" "${log_path_base}")
  upload_dnode_throughput=$(printf "%.2f" ${upload_dnode_throughput})
  local upload_dnode_throughput_exit=$?


  if [ $upload_fresh_throughput_exit -eq 0 ] && [ $upload_update_throughput_exit -eq 0 ] && [ $download_agency_throughput_exit -eq 0 ] && \
      [ $download_dnode_throughput_exit -eq 0 ] && [ $upload_dnode_throughput_exit -eq 0 ]
  then
        printf %b "text=Tonight's benchmark results for the \"${corpus}\" corpus:\n\t- upload throughput (fresh upload via agency-node): ${upload_fresh_throughput} MByte/s.\n\t- de-duplication ratio (fresh upload via agency-node): ${upload_fresh_deduplication}.\n\t- upload throughput (updating upload via agency-node): ${upload_update_throughput} MByte/s.\n\t- download throughput (via agency-node): ${download_agency_throughput} MByte\s.\n" > message.txt
        printf %b "\n\t- upload throughput (via data-node): ${upload_dnode_throughput} MByte\s.\n" >> message.txt
        printf %b "\n\t- download throughput (via data-node): ${download_dnode_throughput} MByte\s.\n" >> message.txt
        curl --data-binary @message.txt -d "channel=${SLACK_CHANNEL}" -H "Authorization: Bearer xoxb-2702783761651-4959893662113-aASbDCYEfMpCRgrnwvC4ZUUn" -X POST https://slack.com/api/chat.postMessage >/dev/null 2>&1
        rm message.txt
	echo "Successfully posted benchmark results for the current corpus to Slack."
  else
        curl -d "text=Tonight's benchmarks did not run properly or there was an error parsing their output." -d "channel=${SLACK_CHANNEL}" -H "Authorization: Bearer xoxb-2702783761651-4959893662113-aASbDCYEfMpCRgrnwvC4ZUUn" -X POST https://slack.com/api/chat.postMessage >/dev/null 2>&1
	echo "Notified Slack channel that the benchmark run using the current corpus didn't appear to deliver valid results."
  fi

}

#
# Posts benchmark results to Prometheus pushgateway
#
# Usage:
#   post_results_to_prometheus <corpus> <log_path_base>
#
post_results_to_prometheus()
{
  local corpus="$1"; shift;
  local log_path_base="$1"; shift;
  local upload_fresh_throughput=$(get_agency_fresh_upload_throughput "${corpus}" "${log_path_base}")
  local upload_fresh_throughput_exit=$?
  local upload_fresh_deduplication=$(get_agency_fresh_upload_deduplication "${corpus}" "${log_path_base}")
  local upload_fresh_deduplication_exit=$?
  local upload_update_throughput=$(get_agency_update_upload_throughput "${corpus}" "${log_path_base}")
  local upload_update_throughput_exit=$?
  local download_agency_throughput=$(get_agency_download_throughput "${corpus}" "${log_path_base}")
  local download_agency_throughput_exit=$?
  local temp_dir=$(mktemp -d)

  if [ $upload_fresh_throughput_exit -eq 0 ]; then
    printf %b "# TYPE uh_nightly_throughput gauge\n# HELP uh_nightly_throughput  Average throughput obtained during nightly benchmarks.\nuh_nightly_throughput ${upload_fresh_throughput}\n" > ${temp_dir}/agency_fresh_upload.txt
    curl --data-binary @${temp_dir}/agency_fresh_upload.txt http://localhost:9091/metrics/job/agency_fresh_upload/instance/${corpus}
  fi

  if [ $upload_fresh_deduplication_exit -eq 0 ]; then
    printf %b "# TYPE uh_nightly_deduplication gauge\n# HELP uh_nightly_deduplication  Average deduplication-ratio obtained during nightly benchmarks.\nuh_nightly_deduplication ${upload_fresh_deduplication}\n" > ${temp_dir}/upload_fresh_deduplication.txt
    curl --data-binary @${temp_dir}/upload_fresh_deduplication.txt http://localhost:9091/metrics/job/agency_fresh_upload/instance/${corpus}
  fi


  if [ $upload_update_throughput_exit -eq 0 ]; then
    printf %b "# TYPE uh_nightly_throughput gauge\n# HELP uh_nightly_throughput  Average throughput obtained during nightly benchmarks.\nuh_nightly_throughput ${upload_update_throughput}\n" > ${temp_dir}/agency_update_upload.txt
    curl --data-binary @${temp_dir}/agency_update_upload.txt http://localhost:9091/metrics/job/agency_update_upload/instance/${corpus}
  fi

  if [ $download_agency_throughput_exit -eq 0 ]; then
    printf %b "# TYPE uh_nightly_throughput gauge\n# HELP uh_nightly_throughput  Average throughput obtained during nightly benchmarks.\nuh_nightly_throughput ${download_agency_throughput}\n" > ${temp_dir}/agency_download.txt
    curl --data-binary @${temp_dir}/agency_download.txt http://localhost:9091/metrics/job/agency_download/instance/${corpus}
  fi

  rm -Rf ${temp_dir}

  echo "Successfully posted benchmark results for the current corpus to Prometheus."
}

teardown "init"

for CORPUS_PATH in "${CORPORA_BASE_DIR}"/*_nightly/; do
  CORPUS=$(basename $CORPUS_PATH)
  LOG_PATH_BASE="/data/logs/benchmark_$(printf '%(%Y%m%d)T' -1)_${CORPUS}_"

  for (( i=1; i<=${NUM_LOOPS}; ++i)); do
    echo "Setting up benchmark for corpus '${CORPUS}, iteration #${i}/${NUM_LOOPS}..."
    setup "${CORPUS}"

    /home/max/core/wait_for_benchmark.py
    docker service logs nightlyBench_client --raw > ${LOG_PATH_BASE}${i}.log

    echo "Benchmark run for corpus '${CORPUS}, iteration #${i}/${NUM_LOOPS} completed, cleaning up setup..."
    teardown
    sleep 15
  done

  post_results_to_slack "${CORPUS}" "${LOG_PATH_BASE}"
  post_results_to_prometheus "${CORPUS}" "${LOG_PATH_BASE}"
done
