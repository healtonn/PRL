#!/bin/bash

if [ $# -lt 2 ];then
    numbers=9;
    processors=3;
else
    numbers=$1;
    processors=$2
fi;

mpic++ -o mss mss.cpp

dd if=/dev/random bs=1 count=$numbers of=numbers 2>/dev/null

mpirun -np $processors mss

rm -f mss numbers