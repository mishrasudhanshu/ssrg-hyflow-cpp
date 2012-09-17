#!/bin/bash

txns=2000
objs=10000

echo "---Nodes Config---"
echo "txns=$txns, objs=$objs, reads 0..100..20 threads 1, nodes=1..24" 
for read in {0..100..20}
do
    echo "For reads = $read"
    for nodes in 1 2 4 8 16 24
    do
        echo "    For nodes = $nodes"
        for exp in {1..3} 
        do
            echo "        For experiment $exp, read $read and nodes $nodes"
            for (( nodeId=0 ; nodeId < $nodes ; nodeId++ ))
            do
                echo "launching $nodeId in $nodes"
                nodes=$nodes objects=$objs transactions=$txns nodeId=$nodeId reads=$read threads=1 Debug/ssrg-hyflow-cpp $nodeId -&
                p=`ps -ef|grep "Debug/ssrg-hyflow-cpp $nodeId -"| grep -v 'grep'|awk '{print $2}'`
                echo taskset -c -p $nodeId $p
                taskset -c -p $nodeId $p
                sleep 2             # Give some time for node 0 to start
            done
            pids=$(pgrep ssrg-hyflow-cpp)   
            for pid in $pids
            do 
                echo "waiting for $pid"
                wait $pid
            done
            sleep 5
        done
    done
done
echo "Executor completed"


