#!/bin/bash

# Following test runs the hyflowCPP and profiles data
# This script must run as sudo user and requires linux-tools installed
# Use perf report -T -n -g -i HyflowPerf to analysis profiling results

build=Debug

if ! [ -z $1 ]
then
   build=$1
fi

nodes=2
objects=10000
transactions=200000
reads=99
threads=4
p=0

for (( nodeId=0 ; nodeId < $nodes ; nodeId++ ))
do
    nodes=$nodes objects=$objs transactions=$txns nodeId=$nodeId reads=$read threads=$threads $build/ssrg-hyflow-cpp $nodeId -&
    p=`ps -ef|grep "$build/ssrg-hyflow-cpp $nodeId -"| grep -v 'grep'|awk '{print $2}'`
    echo taskset -c -p $nodeId $p
    coreId=$((nodeId*threads*4))-$((nodeId*threads*4+4*threads-1)) 
    echo "Moving process to core set $coreId $p"
    #taskset -c -p $coreId $p
    sleep 2             # Give some time for node 0 to start
done
# As all process are identical perf only last one
echo "Recording process $p"
perf record -p $p -F 10000 -T -s -g -o profileResults/HyflowPerf_$p sleep 20 
wait
echo "Done Collecting perf"
