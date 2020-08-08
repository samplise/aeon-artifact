#!/bin/bash

# $1 - input file name with event timings
# $2 - output file name to place distributions

rm event*
rm $2

awk '{printf("%d %s\n", $0, $NF);}' $1 > events
sort -k2 events > all-events.sorted
awk 'BEGIN{last = ""; cnt=0;}
     {
       if ($2 != last) {
         printf("%s %d\n", $2, ++cnt) >> "events.signatures";
       } 
       last = $2; 
       name = "event" cnt; 
       print $1 >> name;
     }' all-events.sorted

export PERCENTILE_INCREMENT=.001

for i in event[1-9]*; do
    NUM=${i:5}
    head -$NUM events.signatures | tail -1 | awk '{print $1}' >> $2
    cat $i | $HOME/mace/tools/percentiles >  percentile$NUM
    wc -l percentile$NUM | awk '{print $1}' >> $2
    cat percentile$NUM >> $2
    rm percentile$NUM
done
