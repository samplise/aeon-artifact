#!/bin/bash

cd ./mace-fullcontext/builds/application/TagPlayerContextApp/
for((nid = $1;nid<=$2;nid++))
do
./TagAppContext -node_id $nid &
done
