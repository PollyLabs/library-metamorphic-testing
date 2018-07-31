#!/bin/bash
z=0
for x in `seq 0 100`
do
    y=$(./gen_and_run_one.sh $x 2>&1)
    echo -ne $x\\r
    if [[ $y =~ false ]]
    then
        ((z++))
    fi
done
((++x))
echo $z of $x
