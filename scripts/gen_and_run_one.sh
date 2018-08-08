#!/bin/bash

SRC_DIR=/home/sentenced/Documents/Internships/2018_ETH/work/sets
BUILD_DIR=${SRC_DIR}/build
OUT_DIR=${SRC_DIR}/out

cd ${BUILD_DIR}
if [ $# -ne 1 ]; then
    echo "Expected 1 parameter, seed."
    exit
fi
./test_emitter -s $1
cd ${OUT_DIR}
./compile_isl.sh test.cpp
timeout 120 ./test
echo $?
cd ..
