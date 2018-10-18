#!/bin/bash
if [ $# -eq 1 ]
then
    echo "Compiling $1"
    g++ -std=c++11 -o ${1%.*} $1 -I../include -L../libs -lomega
else
    echo "Compiling test.cpp"
    g++ -std=c++11 -o test test.cpp -I../include -L../libs/ -lomega
fi
