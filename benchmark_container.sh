#!/bin/bash

ADDR_AGENCY_NODE=uh-server-agency:21832
ADDR_DB_NODE=uh-server-db:12345

PATH_CLIENT="/usr/local/bin/uhClient"
CORPORA_BASE_DIR="/data/corpora"

if [ $# -gt 0 ]; then
  corpus="$1"
else
  exit -1
fi

#
# Upload a corpus to a given host
#
# Usage:
#   upload_corpus <volume> <path-to-corpus> <remote-address> <additional-flags>
#use pro
upload_corpus ()
{
    local volume="$1"; shift;
    local corpus="$1"; shift;
    local remote="$1"; shift;
    local flags=$*

    ${PATH_CLIENT} --integrate "${volume}" "${corpus}" --agency-node "${remote}" ${flags}
}

#
# Perform a single upload benchmark run and return measured throughput and deduplication as JSON in stdout.
#
# Usage:
#   perform_upload_benchmark <path-to-corpus> <remote-address> <additional-flags>
#
perform_upload_benchmark ()
{
    local corpus="$1"; shift;
    local remote="$1"; shift;
    local flags=$*

    local temp_dir=$(mktemp -d)
    local upload_raw_output=$(upload_corpus "${temp_dir}/volume.uh" "${corpus}" "${remote}" ${flags})

    local throughput=$(echo -n "${upload_raw_output}" | \
      grep --extended-regexp '^encoding speed:\s+[0-9]+(\.[0-9]+) Mb/s' | \
      grep --extended-regexp --only-matching '[0-9]+(\.[0-9]+)')
    if [ -z "${throughput}" ]; then
      throughput=0
    fi
    echo "      \"throughput\": ${throughput},"

    local deduplication=$(echo -n "${upload_raw_output}" | \
      grep --extended-regexp '^de-duplication ratio:\s+[0-9]+(\.[0-9]*)*' | \
      grep --extended-regexp --only-matching '[0-9]+(\.[0-9]*)*')
    if [ -z "${deduplication}" ]; then
      deduplication=0
    fi
    echo "      \"deduplication\": ${deduplication}"

    rm -rf "${temp_dir}"
}

#
# Download files of a volume to a directory.
#
# Usage:
#   download_volume <volume file> <remote> <destination dir> <additonal-flags>
#
download_volume ()
{
    local volume="$1"; shift;
    local remote="$1"; shift;
    local dest="$1"; shift;
    local flags=$*

    yes | ${PATH_CLIENT} --retrieve "${volume}" --target "${dest}" --agency-node "${remote}" ${flags}
}

#
# Perform a single download benchmark run and reutrn measured throughput as JSON in stdout.
#
# Usage:
#   perform_download_benchmark <path-to-volume> <remote-address> <additional-flags>
#
perform_download_benchmark ()
{
    local volume="$1"; shift;
    local remote="$1"; shift;
    local flags=$*

    local temp_dir=$(mktemp -d)
    local download_raw_output=$(download_volume "${volume}" "${remote}" "${temp_dir}" ${flags})

    local throughput=$(echo -n "${download_raw_output}" | \
      grep --extended-regexp '^decoding speed:\s+[0-9]+(\.[0-9]+) Mb/s' | \
      grep --extended-regexp --only-matching '[0-9]+(\.[0-9]+)')
    if [ -z "${throughput}" ]; then
      throughput=0
    fi
    echo "      \"throughput\": ${throughput}"

    rm -rf "${temp_dir}"
}


ncat -e /bin/cat -k -u -l 1337 &
#
# Test Plan Definition
#
echo "{"

echo "  \"agency_fresh_upload\": {"
echo "    \"${corpus}\": {"
perform_upload_benchmark "${CORPORA_BASE_DIR}/${corpus}" "${ADDR_AGENCY_NODE}"
echo "    }"
echo "  },"

echo "  \"agency_update_upload\": {"
echo "    \"${corpus}\": {"
perform_upload_benchmark "${CORPORA_BASE_DIR}/${corpus}" "${ADDR_AGENCY_NODE}"
echo "    }"
echo "  },"

echo "  \"agency_download\": {"
temp_dir=$(mktemp -d)
upload_corpus "${temp_dir}/volume.uh" "${CORPORA_BASE_DIR}/${corpus}" "${ADDR_AGENCY_NODE}" > /dev/null
echo "    \"${corpus}\": {"
perform_download_benchmark "${temp_dir}/volume.uh" "${ADDR_AGENCY_NODE}"
rm -rf "${temp_dir}"
echo "    }"
echo "  }"

echo "}" # test_results

pkill ncat
sleep infinity
