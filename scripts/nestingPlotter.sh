#!/bin/bash

#use of this script is to create the data files using which plots can be generated
 
souce_dir=$1
target_dir=$2

for inTxns in 1 2 5 10
do
    for model in flat checkPointing closed
    do
	name=$model
	if [ "$name" == "closed" ]
	then
            name="close"
	fi
        echo "#Node" > t_Nodes.tmp
        bash scripts/analyzer.sh $souce_dir/log_"$model"_InTxns_"$inTxns" gc | grep " 20 " | awk -F " " '{print $1}' >> t_Nodes.tmp
        echo "$name" > t_"$model"_"$inTxns"_20.tmp
        bash scripts/analyzer.sh $souce_dir/log_"$model"_InTxns_"$inTxns" gc | grep " 20 " | awk -F " " '{print $3}' >> t_"$model"_"$inTxns"_20.tmp
        echo "$name" > t_"$model"_"$inTxns"_50.tmp
        bash scripts/analyzer.sh $souce_dir/log_"$model"_InTxns_"$inTxns" gc | grep " 50 " | awk -F " " '{print $3}' >> t_"$model"_"$inTxns"_50.tmp
        echo "$name" > t_"$model"_"$inTxns"_80.tmp
        bash scripts/analyzer.sh $souce_dir/log_"$model"_InTxns_"$inTxns" gc | grep " 80 " | awk -F " " '{print $3}' >> t_"$model"_"$inTxns"_80.tmp
    done

    sub_target_dir=$target_dir/InTxns_"$inTxns"
    mkdir -p $sub_target_dir
    for rp in 20 50 80
    do
        paste t_Nodes.tmp t_flat_"$inTxns"_"$rp".tmp t_checkPointing_"$inTxns"_"$rp".tmp t_closed_"$inTxns"_"$rp".tmp > $sub_target_dir/reads"$rp".dat
	bash scripts/relDataPloter.sh $sub_target_dir/reads"$rp".dat $sub_target_dir/r"$rp"
    done

    echo "Data Generated for inTxns $inTxns Cleaning Up"
    rm t_*.tmp
done

