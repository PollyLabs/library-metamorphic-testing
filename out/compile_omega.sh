#!/bin/bash
if [ $# -eq 1 ]
then
    g++ -o ${1%.*} $1 -I../include -L../libs -lomega
else
    g++ -o test test.cpp -I../include -L../libs/ -lomega
fi
