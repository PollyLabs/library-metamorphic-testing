#!/bin/bash

SRC_DIR=/home/sentenced/Documents/Internships/2018_ETH/work/sets
BUILD_DIR=${SRC_DIR}/build
OUT_DIR=${SRC_DIR}/out

cd ${BUILD_DIR}
if [ $# -ne 1 ]; then
    echo "Expected 1 parameter, seed."
    exit
fi
./isl_tester -m SET_META_NEW -s $1
cd ${OUT_DIR}
./compile.sh
timeout 120 ./test
echo $?
cd ..
