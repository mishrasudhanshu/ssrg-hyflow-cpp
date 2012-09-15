#!/bin/bash

nodeId=$1
txns=1000
objs=1000*$nodeId

for read in {0..100..20}
do
    echo -e "For reads = $read"
    for threads in 1 2 4 8 16 24
    do
        echo -e "    For threads = $threads"
        for exp in 1 2 3
        do
            echo -e "        For experiment $exp, read $read and threads $threads"
            objects=$objs transactions=$txns nodeId=$nodeId reads=$read threads=$threads Debug/ssrg-hyflow-cpp
            break
        done
        break
    done
    break
done
echo "Executor completed"
