#!/bin/bash

if ! [ -z $1 ]
then
   file=$1
fi

outputFile=$2
eps=$outputFile.eps
#title=$3

g=temp

#set environment values
echo "set term postscript eps enhanced color 22" > ${g}
echo "set output \"${eps}\"" >> ${g}
#echo "set title \"$title\"" >> ${g}
echo "set linestyle 6 lt 1 lc 2" >> ${g}
echo "set key bottom" >> ${g}
echo "set key right" >> ${g}
echo "set xtics 10" >> ${g}
echo "set xlabel \"Number of Nodes\"" >> ${g}
echo "set ylabel \"Throughput (transactions/s)\"" >> ${g}
echo "set xrange[0:50]" >> ${g}
echo -n "plot " >> ${g}

grep " 20 " $file > r20
grep " 50 " $file > r50
grep " 80 " $file > r80

echo -n "\"r20\" using 1:3 title \"reads 20%\" with lines" >>${g} 
echo -n ", " >>${g}
echo -n "\"r50\" using 1:3 title \"reads 50%\" with lines" >>${g} 
echo -n ", " >>${g}
echo -n "\"r80\" using 1:3 title \"reads 80%\" with lines" >>${g} 

gnuplot ${g}
epstopdf ${eps}

rm temp
rm r20 r50 r80
rm *.eps 

echo "Plot $outputFile.pdf generated Successfully"
