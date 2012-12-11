#!/bin/bash

# This script is used to collect results.
dir="log/"

if ! [ -z $1 ]
then
    dir=$1
fi

if ! [ -z $2 ]
then
    mode=$2
fi

echo "Using $dir $mode"
java -jar scripts/analyzer.jar $dir $mode
