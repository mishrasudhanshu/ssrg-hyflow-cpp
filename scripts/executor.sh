#!/bin/bash

# Launch this script from project root directory
#Usage: nohup bash scripts/executor.sh 2>&1 | tee >/dev/null

echo "backup old logs" 2>&1 | tee >run.log
timeStamp=$(eval date +%Y"-"%m"-"%d"_"%H"-"%M"-"%S)
mv logs logs_old_$timeStamp

echo "Launching the box test" 2>&1 | tee >>run.log
bash scripts/executor_box.sh 2>&1 | tee >>run.log
timeStamp=$(eval date +%Y"-"%m"-"%d"_"%H"-"%M"-"%S)
mv log log_box_$timeStamp

sleep 1

echo "Launching the nodes test" 2>&1 | tee >>run.log
bash scripts/executor_nodes.sh 2>&1 | tee >>run.log
timeStamp=$(eval date +%Y"-"%m"-"%d"_"%H"-"%M"-"%S)
mv log log_nodes_$timeStamp
