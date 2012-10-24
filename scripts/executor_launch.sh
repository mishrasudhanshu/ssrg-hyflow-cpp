#!/bin/bash

txns=2000
objs=10000
build=Debug

if ! [ -z $1 ]
then
   echo "updating Values"
   reads=$1
   threads=$2
   nodes=$3
   machines=$4
   machine=$5
   perMachine=$6
   build=$7
fi

echo "---Node Launch Config---"
echo "txns=$txns, objs=$objs, reads=$reads threads=$threads, nodes=$nodes, machines=$machines, machine=$machine, perMachine=$perMachine" 

for (( macNodeId=0 ; macNodeId < $perMachine ; macNodeId++ ))
do
    nodeId=$((perMachine*machine+macNodeId))
    echo "launching $nodeId in $nodes"
    parentIP=10.1.1.30 machines=$machines nodes=$nodes objects=$objs transactions=$txns nodeId=$nodeId reads=$read threads=$threads $build/ssrg-hyflow-cpp $nodeId -&
    p=`ps -ef|grep "$build/ssrg-hyflow-cpp $nodeId -"| grep -v 'grep'|awk '{print $2}'`
    echo taskset -c -p $nodeId $p
    coreId=$((nodeId*9))-$((nodeId*9+8)) 
    echo "Moving process to core set $coreId"
    taskset -c -p $coreId $p
    sleep 2             # Give some time for node 0 to start
done
if [[ $machine -eq 0 ]]; then
	echo "Waiting in launcher on lost"
	wait
fi
bash scripts/kill_ipcs.sh
echo "Launch completed"
