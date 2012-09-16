#!/bin/bash

txns=2000
objs=10000

echo "---Box Config---"
echo "nodeId=$nodeId, txns=$txns, objs=$objs, reads 0..100..20 threads 1...24" 
for read in {0..100..20}
do
    echo -e "For reads = $read"
    for threads in 1 2 4 8 16 24
    do
        echo -e "    For threads = $threads"
        for exp in 1 2 3
        do
            echo -e "        For experiment $exp, read $read and threads $threads"
            nodeCount=1 objects=$objs transactions=$txns nodeId=0 reads=$read threads=$threads Debug/ssrg-hyflow-cpp
            sleep 5
        done
    done
done
echo "Executor completed"
