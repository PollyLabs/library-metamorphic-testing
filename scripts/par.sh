#!/bin/bash
for x in `seq 0 5`
do
	taskset -c $x ./meta_runner.py continuous --append-id 1>/dev/null &
	pids[${x}]=$!
done

for pid in ${pids[*]}
do
	wait $pid
done
