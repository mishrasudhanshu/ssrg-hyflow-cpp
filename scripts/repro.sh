#!/bin/bash

core=core*
count=${1:-1}

if ! [ -z $2 ]
then
   build=$2
fi

for (( i=0; i<count ; i++ ))
do
	threads=1 reads=0 transactions=250 nodes=2 objects=10 nodeId=0 $build/ssrg-hyflow-cpp >/dev/null &
	sleep 1
	threads=1 reads=0 transactions=250 nodes=2 objects=18 nodeId=0 $build/ssrg-hyflow-cpp >/dev/null &
	sleep 1
	#threads=3 reads=0 transactions=5 nodes=4 objects=10 nodeId=2 $build/ssrg-hyflow-cpp >/dev/null &
	#sleep 1
	#threads=3 reads=0 transactions=5 nodes=4 objects=10 nodeId=3 $build/ssrg-hyflow-cpp >/dev/null &
	wait
    sleep 1
	if [ -f $core ]
	then
        	echo "Got core"
		break
	fi
	grep "fail" log/0_fatal.log >tmp
	if [[ -s tmp ]] 
	then
        	echo "Got log"
		break
	fi
	echo "$i"
	bash scripts/kill_ipcs.sh
	#sleep 30
done
echo "got it!!"
