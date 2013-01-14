#!/bin/bash

#This scripts the plot for close nesting Experiments 1 & 2

source_dir=$1
target_dir=$2

for inTxns in 2 5 10
do
    for model in flat checkPointing closed
    do
        name=$model
        if [ "$name" == "closed" ]
        then
            name="Close-Nesting"
        fi
        if [ "$name" == "flat" ]
        then
            name="Flat-Nesting"
        fi
        if [ "$name" == "checkPointing" ]
        then
            name="Checkpointing"
        fi
        echo "#Node" > t_Nodes.tmp
        bash scripts/analyzer.sh $source_dir/log_"$model"_InTxns_"$inTxns" gc | grep " 20 " | awk -F " " '{print $1}' >> t_Nodes.tmp
        echo "$name""-""$inTxns" > t_"$model"_"$inTxns"_20.tmp
        bash scripts/analyzer.sh $source_dir/log_"$model"_InTxns_"$inTxns" gc | grep " 20 " | awk -F " " '{print $3}' >> t_"$model"_"$inTxns"_20.tmp
        echo "$name""-""$inTxns" > t_"$model"_"$inTxns"_50.tmp
        bash scripts/analyzer.sh $source_dir/log_"$model"_InTxns_"$inTxns" gc | grep " 50 " | awk -F " " '{print $3}' >> t_"$model"_"$inTxns"_50.tmp
        echo "$name""-""$inTxns" > t_"$model"_"$inTxns"_80.tmp
        bash scripts/analyzer.sh $source_dir/log_"$model"_InTxns_"$inTxns" gc | grep " 80 " | awk -F " " '{print $3}' >> t_"$model"_"$inTxns"_80.tmp
    done
done

for rp in 20 50 80
do
    paste t_Nodes.tmp > $target_dir/reads"$rp".dat
    for inTxns in 1 2 5 10
    do
        paste $target_dir/reads"$rp".dat t_flat_"$inTxns"_"$rp".tmp t_checkPointing_"$inTxns"_"$rp".tmp t_closed_"$inTxns"_"$rp".tmp > t_"$rp"_"$inTxns".tmp
        mv t_"$rp"_"$inTxns".tmp $target_dir/reads"$rp".dat
    done
    bash scripts/relClosePloter.sh $target_dir/reads"$rp".dat $target_dir/r"$rp"
    echo "Data Generated for inTxns $inTxns Cleaning Up"
done

rm t_*.tmp
echo "Source:$source_dir" >$target_dir/source.dat
