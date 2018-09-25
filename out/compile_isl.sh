#!/bin/bash
if [ $# -eq 1 ]
then
    g++ -std=c++11 -o ${1%.*} $1 -I../include -L../libs -lisl
else
    g++ -std=c++11 -o test test.cpp -I../include -L../libs/ -lisl
fi
