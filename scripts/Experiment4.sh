#!/bin/bash

# In this experiment, we increase the Transactional Length

txns=200
threads=1;
build=Debug

if ! [ -z $1 ]
then
   build=$1
fi


benchmarks="bank list skipList bst hashTable"

for bench in $benchmarks
do
    objs=300
    setBenchMarks="list bst skipList"
    if [[ "$setBenchMarks" =~ "$bench"  ]]
    then
        objs=5
        echo "Got Set BenchMark Objects=$objs"
    fi

    mkdir -p "$bench"_exp4
    echo "txns=$txns, objs=$objs, reads 0..100..20 threads 1, nodes=1..24" 

    models="flat checkPointing close"

    for model in $models
    do
        echo "---Nodes Config $model---"
        for exp in 0 1 5 10 20 
        do
            for trial in {1..2}
            do
                echo "For experiment = $exp"
                inTxns=2
                txnLen=$exp
                for nodes in 2 4 8 16 32 48
                do
                    echo "    For nodes = $nodes For objs=$objs and inTxns=$inTxns"
                    for read in {20..100..30}
                    do
                        echo "        For experiment $exp, read $read and nodes $nodes"
                        for (( nodeId=0 ; nodeId < $nodes ; nodeId++ ))
                        do
                            echo "launching $nodeId in $nodes"
                            benchmark=$bench transactionLength=$txnLen nestingModel=$model innerTxns=$inTxns nodes=$nodes objects=$objs transactions=$txns nodeId=$nodeId reads=$read threads=$threads $build/ssrg-hyflow-cpp $nodeId -&
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
            mv log "$bench"_exp4/log_"$model"_TxnLength_"$inTxns"
        done
    done
    echo "Executor $bench completed"
done
echo "Executor completed"

