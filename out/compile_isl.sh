#!/bin/bash
if [ $# -eq 1 ]
then
    g++ -o ${1%.*} $1 -I../include -L../libs -lisl
else
    g++ -o test test.cpp -I../include -L../libs/ -lisl
fi
