#!/bin/bash

# This file profiles the data file similar to excel format
# First line contains the columns with name of each type of data

file=$1
outputFile=output

if ! [ -z $2 ]
then
    outputFile=$2
fi

xlabel="Number of Nodes"
if ! [ -z $3 ]
then
    xlabel=$3
fi

eps=$outputFile.eps

g=/tmp/plot.temp

#set environment values
echo "set term postscript eps enhanced color 22" > ${g}
echo "set output \"${eps}\"" >> ${g}
#echo "set title \"$title\"" >> ${g}
echo "set key bottom" >> ${g}
echo "set key right" >> ${g}
echo "set xtics 10" >> ${g}
echo "set xlabel \"$xlabel\"" >> ${g}
echo "set ylabel \"Throughput (transactions/s)\"" >> ${g}
echo "set xrange[0:50]" >> ${g}
echo -n "plot " >> ${g}

columns=$(head -1 $file)
count=1
for token in $columns
do
    if [ "$count" -eq 1 ]
    then
        echo "Reading First column"
    elif [ "$count" -eq 2 ]
    then
        echo -n "\"$file\" using 1:$count title \"$token\" with lines" >>${g} 
    else
        echo -n ", "
        echo -n "\"$file\" using 1:$count title \"$token\" with lines" >>${g} 
    fi
    let count++ 
done

gnuplot ${g}
epstopdf ${eps}

#rm /tmp/plot.temp
rm *.eps 

echo "Plot $outputFile.pdf generated Successfully"
