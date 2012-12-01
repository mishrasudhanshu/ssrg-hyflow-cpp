#!/bin/bash

#Run all benchmarks

benchmarks="bank list skipList bst loan tpcc hashTable"


for bench in $benchmarks
do
        echo "Running $bench"
        benchmark=$bench bash scripts/executor_nodes.sh Release/ 2>&1 | tee >$bench.log
        wait
        sleep 4
        mv log log_$bench
done
echo "all benchmarks done"
