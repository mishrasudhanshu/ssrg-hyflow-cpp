#!/bin/bash

build=Debug

if ! [ -z $1 ]
then
   build=$1
fi

# Launch this script from project root directory
#Usage: nohup bash scripts/executor.sh 2>&1 | tee >/dev/null

echo "backup old logs" 2>&1 | tee >run.log
timeStamp=$(eval date +%Y"-"%m"-"%d"_"%H"-"%M"-"%S)
mv log log_old_$timeStamp
mv log_* backupLogs/.

echo "Launching the box test" 2>&1 | tee >>run.log
bash scripts/executor_box.sh $build 2>&1 | tee >>run.log
timeStamp=$(eval date +%Y"-"%m"-"%d"_"%H"-"%M"-"%S)
mv log log_box_$timeStamp

sleep 1

echo "Launching the nodes test" 2>&1 | tee >>run.log
bash scripts/executor_nodes.sh $build 2>&1 | tee >>run.log
timeStamp=$(eval date +%Y"-"%m"-"%d"_"%H"-"%M"-"%S)
mv log log_nodes_$timeStamp

sleep 1

echo "Launching the Mixed test" 2>&1 | tee >>run.log
bash scripts/executor_mix.sh $build 2>&1 | tee >>run.log
timeStamp=$(eval date +%Y"-"%m"-"%d"_"%H"-"%M"-"%S)
mv log log_mixed_$timeStamp
