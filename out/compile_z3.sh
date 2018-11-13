#!/bin/bash
if [ $# -eq 1 ]
then
    echo "Compiling $1"
    g++ -g -std=c++11 -o ${1%.*} $1 -I../include/z3 -I../include -L../libs -lz3
else
    echo "Compiling test.cpp"
    g++ -g -std=c++11 -o test test.cpp -I../include/z3 -I../include -L../libs/ -lz3
fi
