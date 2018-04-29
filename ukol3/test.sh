#!/bin/bash
set -e

if [[ $# -eq 1 ]];then
    tree=$1
    amountOfProcessors=$((${#tree} * 2 - 2))
else
    echo wrong arguments
    exit 1
fi;

mpic++ -o pro pro.cpp

mpirun -np $amountOfProcessors pro $1
