#!/bin/bash

print_help()
{
    echo "uh-run-tests-ceph.sh [options] <service>"
    echo
    echo "Open a remote debugger for a given service"
    echo
    echo "options are:"
    echo " -h, --help           print this text"
    echo " -g, --gdb            attach a gdb on this terminal"
    echo " service              entrypoint, directory, deduplicator or storage"
}

run_gdb="0"
service=""

while [ -n "$1" ]; do

    case "$1" in
        -h|--help)  print_help
                    exit 0
                    ;;
        -g|--gdb)   run_gdb=1
                    ;;
        *)  service="$1"
            ;;
    esac

    shift
done

[ -z "$UH_TEST_BASE" ] && {
    echo "UH_TEST_BASE environment variable is not set"
}

[ -z "$service" ] && {
    echo "service is missing"
    print_help
    exit 1
}

. $UH_TEST_BASE/activate

nohup docker compose --project-directory $UH_TEST_BASE exec --user root --interactive --tty $service gdbserver :$UH_DEBUG_CONTAINER_PORT --attach 1 &> gdbserver-$service.log &
echo "$!" > gdbserver-$service.pid

SERVICE=$(echo $service | tr '[:lower:]' '[:upper:]')
service_port_var="UH_DEBUG_PORT_$SERVICE"
local_gdb_server="${!service_port_var}"

timeout=20
while [ "$timeout" -gt "0" ]; do
    if nc -z localhost $local_gdb_server; then
        break
    fi

    timeout=$((timeout - 1))
    sleep 1
done

if [ "$timeout" -eq "0" ]; then
    echo "cannot connect to remote debugger"
    exit 1
fi

if [ "$run_gdb" -eq "1" ]; then
    echo "connecting gdb to gdbserver in $service container"
    gdb --quiet --init-eval-command "target remote localhost:$local_gdb_server"

    kill -TERM $(cat gdbserver-$service.pid)

    echo "Output of remote gdbserver was stored in gdbserver-$service.log."
else
    echo "gdbserver is reachable under localhost:$local_gdb_server"
    echo "Output pipes of the spawned process were redirected to gdbserver-$service.log."
    echo "The pid of the spawned process was stored in gdbserver-$service.pid."
fi
