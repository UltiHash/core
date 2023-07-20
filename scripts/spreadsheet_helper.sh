
export first_arg=$1
export second_arg=$2
export third_arg=$3

echo "project base dir: ${first_arg}"
echo "to integrate: ${second_arg}"
echo "db root: ${third_arg}"

start_db(){
echo "Processing UltiHash project path = ${first_arg}, integration folder path = ${second_arg}"

echo "Removing contents of ${third_arg}"
rm "${third_arg}/*"

${first_arg}/cmake-build-release/server-db/uh-data-node --threads 15 --port 12345 --data-directory ${third_arg}
}

integrate_retrieve(){
{
sleep 10

result_data="${second_arg}.result"

echo "UHV target: ${second_arg}.uh"
echo "Integrate from: ${second_arg}"
echo "Result at: ${result_data}"

echo "Integrate:$(${first_arg}/cmake-build-release/client-shell/uh-cli --integrate ${second_arg}.uh ${second_arg} --agency-node localhost:12345)" > "${result_data}"
}||{
echo "Integration failed!"
}

{
echo "Retrieve:$(${first_arg}/cmake-build-release/client-shell/uh-cli --retrieve ${second_arg}.uh --target ${second_arg} --agency-node localhost:12345)" >> "${result_data}"
}||{
echo "Retrieval failed!"
}

killall uh-data-node
}

export -f start_db && export -f integrate_retrieve
parallel -j2 --lb --halt-on-error now,fail=1 ::: start_db integrate_retrieve
