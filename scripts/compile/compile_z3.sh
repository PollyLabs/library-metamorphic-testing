#!/bin/bash
if [ $# -eq 1 ]
then
    echo "Compiling $1"
    g++ -g -std=c++11 -g -o ${1%.*} $1 -I../include -lz3
else
    echo "Compiling test.cpp"
    g++ -g -std=c++11 -g -o test test.cpp -I../include -lz3
fi
