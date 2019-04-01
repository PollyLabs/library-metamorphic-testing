#!/bin/bash
if [ $# -eq 1 ]
then
    echo "Compiling $1"
    g++ -std=c++11 -g -o ${1%.*} $1 -I../include_local -I../include -L../libs_local -lgmpxx -lgmp
else
    echo "Compiling test.cpp"
    g++ -std=c++11 -g -o test test.cpp -I../include_local -I../include -L../libs_local/ -lgmpxx -lgmp
fi
