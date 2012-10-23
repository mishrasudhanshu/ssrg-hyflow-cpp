#!/bin/bash

txns=2000
objs=10000
threads=1;
build=Debug

if ! [ -z $1 ]
then
   build=$1
fi

echo "---Nodes Config---"
echo "txns=$txns, objs=$objs, reads 0..100..20 threads 1, nodes=1..24" 
for exp in {1..3}
do
    echo "For experiment = $exp"
    for nodes in 1 2 4 8 12
    do
        echo "    For nodes = $nodes"
	for read in {0..100..20}
        do
            echo "        For experiment $exp, read $read and nodes $nodes"
            for (( nodeId=0 ; nodeId < $nodes ; nodeId++ ))
            do
                echo "launching $nodeId in $nodes"
                nodes=$nodes objects=$objs transactions=$txns nodeId=$nodeId reads=$read threads=$threads $build/ssrg-hyflow-cpp $nodeId -&
                p=`ps -ef|grep "$build/ssrg-hyflow-cpp $nodeId -"| grep -v 'grep'|awk '{print $2}'`
                echo taskset -c -p $nodeId $p
                coreId=$((nodeId*threads*4))-$((nodeId*threads*4+4*threads-1)) 
                echo "Moving process to core set $coreId"
                taskset -c -p $coreId $p
                sleep 2             # Give some time for node 0 to start
            done
            wait
            bash scripts/kill_ipcs.sh
            sleep 2
        done
    done
done
echo "Executor completed"


