#!/bin/bash

# Script to run the HyflowCpp across multiple machines, depends on executor_launch.sh

txns=2000
objs=10000
setBenchMarks="list bst skipList"
if [[ "$setBenchMarks" =~ "$bench"  ]]
then
    objs=5
    echo "Got Set BenchMark Objects=$objs"
fi

build=Debug
machines=2

if ! [ -z $1 ]
then
   build=$1
fi

echo "---Multi Machine Config---"
echo "txns=$txns, objs=$objs, reads=20..100..30 threads=4..72..4, nodes=2..2..1, machines=$machines" 

for exp in {1..2..1} 
do
    echo "For experiments = $exp"
    for reads in {20..100..30}
    do
    echo "    For reads = $read"
        for threads in {4..72..4} 
        do
        echo "         For threads = $threads"
            for nodes in {2..2..1}
            do
                perMachine=$((nodes/machines))
                echo "             For experiment $exp, read $read, threads $threads and nodes $nodes"
                for (( machine=0 ; machine < $machines ; machine++ ))
                do
                    if [[ $machine -eq 0 ]]; then 
                        echo "launching lost for machine $machine"
                        cd /home/users/mishra/hyflowCpp/ssrg-hyflow-cpp/
                        bash scripts/executor_launch.sh $reads $threads $nodes $machines $machine $perMachine $build & 
                        sleep 4
                    elif [[ $machine -eq 1 ]]; then
                        echo "launching graham for machine $machine"
                        ssh graham "cd /home/users/mishra/hyflowCpp/ssrg-hyflow-cpp/;nohup bash scripts/executor_launch.sh $reads $threads $nodes $machines $machine $perMachine $build 2>&1 | tee >log/mac_$machine" &
                        sleep 4
                    elif [[ $mchine -eq 2 ]]; then
                        echo "lauching rosella for machine $machine"
                        ssh rosella "cd /home/users/mishra/hyflowCpp/ssrg-hyflow-cpp/;nohup bash scripts/executor_launch.sh $reads $threads $nodes $machines $machine $perMachine $build 2>&1 | tee >log/mac_$machine" &
                        sleep 4
                    else 
                        echo "Incorrect machine No"
                    fi
                done
                echo "Waiting for process to end"
                wait
                sleep 4
                bash scripts/kill_ipcs.sh
                sleep 2
            done
        done
    done
done
bash scripts/kill_ipcs.sh
echo "Launch completed"
