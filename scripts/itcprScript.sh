#!/bin/bash

#This scripts the plot for close nesting Experiments 1 & 2

source_dir=$1
target_dir=$2

InnerTxns="2 10"
Itcprs="1 10"

for inTxns in $InnerTxns
do
    for itcpr in $Itcprs 
    do
        model=checkPointing
        name=$model
        if [ "$name" == "flat" ]
        then
            name="Flat-Nesting"
        fi
        if [ "$name" == "checkPointing" ]
        then
            name="Checkpointing"
        fi
        echo "#Node" > t_Nodes.tmp
        bash scripts/analyzer.sh $source_dir/log_"$model"_ITCPR_"$itcpr"_InTxns_"$inTxns" gc | grep " 20 " | awk -F " " '{print $1}' >> t_Nodes.tmp
        echo "$name""-""$inTxns""-""$itcpr" > t_"$itcpr"_"$inTxns"_20.tmp
        bash scripts/analyzer.sh $source_dir/log_"$model"_ITCPR_"$itcpr"_InTxns_"$inTxns" gc | grep " 20 " | awk -F " " '{print $3}' >> t_"$itcpr"_"$inTxns"_20.tmp
        echo "$name""-""$inTxns""-""$itcpr" > t_"$itcpr"_"$inTxns"_50.tmp
        bash scripts/analyzer.sh $source_dir/log_"$model"_ITCPR_"$itcpr"_InTxns_"$inTxns" gc | grep " 50 " | awk -F " " '{print $3}' >> t_"$itcpr"_"$inTxns"_50.tmp
        echo "$name""-""$inTxns""-""$itcpr" > t_"$itcpr"_"$inTxns"_80.tmp
        bash scripts/analyzer.sh $source_dir/log_"$model"_ITCPR_"$itcpr"_InTxns_"$inTxns" gc | grep " 80 " | awk -F " " '{print $3}' >> t_"$itcpr"_"$inTxns"_80.tmp
    done
done

for inTxns in $InnerTxns
do
    for itcpr in $Itcprs 
    do
        model=checkPointing
        name=$model
        if [ "$name" == "flat" ]
        then
            name="Flat-Nesting"
        fi
        if [ "$name" == "checkPointing" ]
        then
            name="Checkpointing"
        fi
        echo "$name""-""$inTxns""-""$itcpr" > t_"$itcpr"_"$inTxns"_20.tmp
        bash scripts/analyzer.sh $source_dir/log_"$model"_ITCPR_"$itcpr"_InTxns_"$inTxns" gc AbortRate | grep " 20 " | awk -F " " '{print $3}' >> t_abortrate_"$itcpr"_"$inTxns"_20.tmp
        echo "$name""-""$inTxns""-""$itcpr" > t_"$itcpr"_"$inTxns"_50.tmp
        bash scripts/analyzer.sh $source_dir/log_"$model"_ITCPR_"$itcpr"_InTxns_"$inTxns" gc AbortRate | grep " 50 " | awk -F " " '{print $3}' >> t_abortrate_"$itcpr"_"$inTxns"_50.tmp
        echo "$name""-""$inTxns""-""$itcpr" > t_"$itcpr"_"$inTxns"_80.tmp
        bash scripts/analyzer.sh $source_dir/log_"$model"_ITCPR_"$itcpr"_InTxns_"$inTxns" gc AbortRate | grep " 80 " | awk -F " " '{print $3}' >> t_abortrate_"$itcpr"_"$inTxns"_80.tmp
    done
done

for rp in 20 50 80
do
    paste t_Nodes.tmp > $target_dir/reads"$rp".dat
    for inTxns in $InnerTxns
    do
        for itcpr in $Itcprs
        do
            paste $target_dir/reads"$rp".dat t_"$itcpr"_"$inTxns"_"$rp".tmp > t_"$rp"_"$inTxns".tmp
            mv t_"$rp"_"$inTxns".tmp $target_dir/reads"$rp".dat
        done
        # Add abort rate
        for itcpr in 1 10
        do
            paste $target_dir/reads"$rp".dat t_abortrate_"$itcpr"_"$inTxns"_"$rp".tmp > t_"$rp"_"$inTxns".tmp
            mv t_"$rp"_"$inTxns".tmp $target_dir/reads"$rp".dat
        done
    done
    bash scripts/relItcprPloter.sh $target_dir/reads"$rp".dat $target_dir/r"$rp"
    echo "Data Generated for inTxns $inTxns Cleaning Up"
done

rm t_*.tmp
echo "Source:$source_dir" >$target_dir/source.dat
