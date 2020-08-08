#!/bin/bash

cp /dev/stdin tmpfile

cat tmpfile | /home/ckillian/mace/application/modelchecker/visualizeDot.pl -l meIPV4 -e children | dotty - &
cat tmpfile | /home/ckillian/mace/application/modelchecker/visualizeDot.pl -l meIPV4 -e parent -r 1 | dotty - &
cat tmpfile | /home/ckillian/mace/application/modelchecker/visualizeDot.pl -l meIPV4 -e root -r 1 | dotty - &

rm -f tmpfile
