#!/bin/bash

txns=2000
objs=10000

echo "---Nodes Config---"
echo "txns=$txns, objs=$objs, reads 0..100..20 threads 1, nodes=1..24" 
for read in {0..100..20}
do
    echo -e "For reads = $read"
    for nodes in 1 2 4 8 16 24
    do
        echo -e "    For nodes = $nodes"
        for exp in 1..3 
        do
            echo -e "        For experiment $exp, read $read and nodes $nodes"
            for (( nodeId=0 ; nodeId < $nodes ; nodeId++ ))
            do
                echo "launching $nodeId"
                nodeCount=$nodes objects=$objs transactions=$txns nodeId=$nodeId reads=$read threads=1 Debug/ssrg-hyflow-cpp&
            done
            pids=$(pgrep ssrg-hyflow-cpp)   
            for pid in $pids
            do 
                echo "waiting for $pid"
                wait $pid
            done
            sleep 1
        done
    done
done
echo "Executor completed"


