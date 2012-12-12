#!/bin/bash

txns=2000
objs=10000
setBenchMarks="list bst skipList"
if [[ "$setBenchMarks" =~ "$bench"  ]]
then
    objs=5
    echo "Got Set BenchMark Objects=$objs"
fi

build=Debug

if ! [ -z $1 ]
then
   build=$1
fi

echo "---Box Config---"
echo "nodeId=$nodeId, txns=$txns, objs=$objs, reads 0..100..20 threads 1...24" 

for exp in {1..3}
do
    echo "For experiment = $exp"
    for read in {0..100..20}
    do
  	echo "    For reads = $read"
       	for threads in 1 2 4 8 12 16
        do
            echo "        For experiment $exp, read $read and threads $threads"
            nodes=1 objects=$objs transactions=$txns nodeId=0 reads=$read threads=$threads $build/ssrg-hyflow-cpp
            sleep 5
        done
        bash scripts/kill_ipcs.sh
    done
done
echo "Executor completed"
