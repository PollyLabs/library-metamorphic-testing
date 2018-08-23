#!/bin/bash
test_count=20000
for x in `seq 0 5`
do
	taskset -c $x ./meta_runner.py bounded --seed-min $((x * test_count)) --seed-max $(((x + 1) * test_count)) --config-file /home/isl_testing/isl-metamorphic-testing/config_files/par_config_files/config_$x.yaml 1>/dev/null &
	pids[${x}]=$!
done

for pid in ${pids[*]}
do
	wait $pid
done
