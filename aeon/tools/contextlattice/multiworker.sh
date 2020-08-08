#!/bin/bash
for (( pid=$1; pid<=$2; pid++ ))
do
    ./heartbeat -isworker 1 -pid $pid -TRACE_ALL 1 &
done
