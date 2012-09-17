#!/bin/bash

# This script is used to collect results.
dir="log/"

if ! [ -z $1 ]
then
    dir=$1
fi

echo "Using $dir"
java -jar scripts/analyzer.jar $dir
