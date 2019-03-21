#!/bin/bash
images=(z3 gmp isl_maint)

if [ "$1" == "all" ]; then
    echo "docker build --no-cache -t alascu/metalib:base -f ./docker/Dockerfile_base ./docker"
    echo "docker push alascu/metalib:base"
    for x in ${images[@]}; do
        echo "docker build --no-cache -t alascu/metalib:base_$x -f ./docker/Dockerfile_base_$x ./docker"
        echo "docker push alascu/metalib:base_$x"
    done
elif [ ! -f ./docker/Dockerfile_base_$x ]; then
    echo "Missing base dockerfile ./docker/Dockerfile_base_$x."
else
    echo "docker build --no-cache -t alascu/metalib:base_$1 -f ./docker/Dockerfile_base_$1 ./docker"
    echo "docker push alascu/metalib:base_$1"
fi
