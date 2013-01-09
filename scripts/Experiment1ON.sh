#!/bin/bash

#For constant Inner Txns(3) vary the node and read count and compare to close 

txns=200
threads=1;
build=Debug

if ! [ -z $1 ]
then
   build=$1
fi

benchmarks="list skipList bst hashTable"

for bench in $benchmarks
do
    objs=300
    setBenchMarks="list bst skipList"
    if [[ "$setBenchMarks" =~ "$bench"  ]]
    then
        objs=5
        echo "Got Set BenchMark Objects=$objs"
    fi

    mkdir -p "$bench"_exp1ON
    echo "txns=$txns, objs=$objs, reads 20..100..30 threads 1, nodes=1..24" 

    models="flat closed open"

    for model in $models
    do
        echo "---Nodes Config $model---"
        for exp in 2 3 4 8
        do
            for trial in {1..2}
            do
                echo "For experiment = $exp"
                inTxns=$exp
                for nodes in 2 4 8 12 24 36 48
                do
                    echo "    For nodes = $nodes For objs=$objs and inTxns=$inTxns"
                    for read in {20..100..30}
                    do
                        echo "        For experiment $exp, read $read and nodes $nodes"
                        for (( nodeId=0 ; nodeId < $nodes ; nodeId++ ))
                        do
                            echo "launching $nodeId in $nodes"
                            benchmark=$bench nestingModel=$model innerTxns=$inTxns nodes=$nodes objects=$objs transactions=$txns nodeId=$nodeId reads=$read threads=$threads $build/ssrg-hyflow-cpp $nodeId -&
                            p=`ps -ef|grep "$build/ssrg-hyflow-cpp $nodeId -"| grep -v 'grep'|awk '{print $2}'`
                            echo taskset -c -p $nodeId $p
                            coreId=$((nodeId*1))-$((nodeId*1))
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
            mv log "$bench"_exp1ON/log_"$model"_InTxns_"$inTxns"
        done
    done
    echo "Executor $bench done"
done
echo "Executor All  done"

