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
loops=1

enable_agency_upload=true
enable_direct_upload=false
enable_agency_download=true
enable_direct_download=false

#
# Upload a corpus to a given host
#
# Usage:
#   upload_corpus <volume> <path-to-corpus> <remote-address> <additional-flags>
#
upload_corpus ()
{
    local volume="$1"; shift;
    local corpus="$1"; shift;
    local remote="$1"; shift;
    local flags=$*

    ${PATH_CLIENT} --integrate "${volume}" "${corpus}" --agency-node "${remote}" ${flags}
}

#
# Perform a single upload benchmark run and return measured throughput in stdout.
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
    upload_corpus "${temp_dir}/volume.uh" "${corpus}" "${remote}" ${flags} | \
        grep --extended-regexp '^encoding speed:\s+[0-9]+(\.[0-9]+) Mb/s' | \
        grep --extended-regexp --only-matching '[0-9]+(\.[0-9]+)'
    rm -rf "${temp_dir}"
}

#
# Measure upload throughput for a given corpus to a remote address for N times.
# Output throughputs as JSON array to stdout.
#
# Usage:
#   measure_upload_throughput <path-to-corpus> <remote-address> <loops> <cleanup> <additional-flags>
#
measure_upload_throughput ()
{
    local corpus="$1"; shift;
    local remote="$1"; shift;
    local loops="$1"; shift;
    local cleanup="$1"; shift;
    local flags=$*

    local values=()
    for (( i=0; i<${loops}; ++i)); do
        ${cleanup}
        throughput=$(perform_upload_benchmark "${corpus}" "${remote}" ${flags})
        values+=( ${throughput} )
    done

    echo -n "[ "
    (IFS=,; echo -n "${values[*]}")
    echo -n " ]"
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

    yes | ${PATH_CLIENT} --retrieve "${volume}" "${dest}" --agency-node "${remote}" ${flags}
}

#
# Perform a single download benchmark run and reutrn measured throughput in stdout.
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
    download_volume "${volume}" "${remote}" "${temp_dir}" ${flags} | \
        grep --extended-regexp '^decoding speed:\s+[0-9]+(\.[0-9]+) Mb/s' | \
        grep --extended-regexp --only-matching '[0-9]+(\.[0-9]+)'

    rm -rf "${temp_dir}"
}

#
# Measure download throughput for a given UH volume file from remote address for N times.
# Output throughputs as JSON array to stdout.
#
# Usage:
#   measure_download_throughput <path-to-volume> <remote-address> <loops> <additional-flags>
#
measure_download_throughput ()
{
    local volume="$1"; shift;
    local remote="$1"; shift;
    local loops="$1"; shift;
    local flags=$*

    local values=()
    for (( i=0; i<${loops}; ++i)); do
        throughput=$(perform_download_benchmark "${volume}" "${remote}" ${flags})
        values+=( ${throughput} )
    done

    echo -n "[ "
    (IFS=,; echo -n "${values[*]}")
    echo -n " ]"
}


ncat -e /bin/cat -k -u -l 1337 &
#
# Test Plan Definition
#
echo "{"

step_delim=""

if ${enable_agency_upload}; then

    echo "\"agency_fresh_upload\": {"
    delim=""
    echo -ne ${delim}
    echo -n "\"${corpus}\": "
    measure_upload_throughput "${CORPORA_BASE_DIR}/${corpus}" "${ADDR_AGENCY_NODE}" "${loops}" 
    delim=",\n"
    echo -e "\n},"

    echo "\"agency_update_upload\": {"
    delim=""
    echo -ne ${delim}
    echo -n "\"${corpus}\": "
    measure_upload_throughput "${CORPORA_BASE_DIR}/${corpus}" "${ADDR_AGENCY_NODE}" "${loops}"
    delim=",\n"
    echo "}"
    step_delim=","
fi

if ${enable_direct_upload}; then
    echo "${step_delim}"

    echo "\"direct_fresh_upload\": {"
    delim=""
    echo -ne ${delim}
    echo -n "\"${corpus}\": "
    measure_upload_throughput "${CORPORA_BASE_DIR}/${corpus}" "${ADDR_DB_NODE}" "${loops}"
    delim=",\n"
    echo "},"

    echo "\"direct_update_upload\": {"
    delim=""
    echo -ne ${delim}
    echo -n "\"${corpus}\": "
    measure_upload_throughput "${CORPORA_BASE_DIR}/${corpus}" "${ADDR_DB_NODE}" "${loops}"
    delim=",\n"
    echo "}"
    step_delim=","
fi

if ${enable_agency_download}; then
    echo "${step_delim}"
    echo "\"agency_download\": {"
    delim=""
    temp_dir=$(mktemp -d)
    upload_corpus "${temp_dir}/volume.uh" "${CORPORA_BASE_DIR}/${corpus}" "${ADDR_AGENCY_NODE}" > /dev/null

    echo -ne ${delim}
    echo -n "\"${corpus}\": "
    measure_download_throughput "${temp_dir}/volume.uh" "${ADDR_AGENCY_NODE}" "${loops}"
    delim=",\n"

    rm -rf "${temp_dir}"
    echo "}"
    step_delim=","
fi

if ${enable_direct_download}; then
    echo "${step_delim}"
    echo "\"direct_download\": {"

    delim=""
    temp_dir=$(mktemp -d)

    upload_corpus "${temp_dir}/volume.uh" "${CORPORA_BASE_DIR}/${corpus}" "${ADDR_DB_NODE}" > /dev/null

    echo -ne ${delim}
    echo -n "\"${corpus}\": "
    measure_download_throughput "${temp_dir}/volume.uh" "${ADDR_DB_NODE}" "${loops}"
    delim=",\n"

    rm -rf "${temp_dir}"

    echo "}"
    step_delim=","
fi

echo "}" # test_results

pkill ncat
sleep infinity
