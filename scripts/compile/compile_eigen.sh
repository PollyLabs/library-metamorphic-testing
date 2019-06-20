#!/bin/bash
if [ $# -eq 1 ]
then
    echo "Compiling $1"
    g++ -std=c++11 -g -o ${1%.*} $1 -I/data/pgharat/project3/library-metamorphic-testing/include/eigen -I/data/pgharat/project3/library-metamorphic-testing/include
else
    echo "Compiling test.cpp"
    g++ -std=c++11 -g -o test test.cpp -I/data/pgharat/project3/library-metamorphic-testing/include/eigen -I/data/pgharat/project3/library-metamorphic-testing/include
fi
