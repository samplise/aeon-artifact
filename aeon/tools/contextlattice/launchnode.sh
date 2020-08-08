#!/bin/bash

# chuangw: job scheduler process uses launchnode.sh to request nodes and maintain thread pool size.
# The EC2 launcher is dependent on EC2 commandline tools.
# Please install Amazon EC2 API Tools by following the instruction at 
# http://aws.amazon.com/developertools/351

if [ "$1" == "cloud" ]
then
  echo "Execute nodes at local cloud machines"
  ssh cloud02 ./runworker.sh $2
elif [ "$1" == "condor" ]
then
  echo "Execute nodes at condor"
  ssh condor-fe02.rcac ./createjobs.sh $2
elif [ "$1" == "ec2" ]
then
  echo "Execute nodes at Amazon EC2 platform"
  # TODO: request a list of nodes
  NODES=`ec2-describe-instances | grep '^INSTANCE' | cut -f4`
  echo "==========Available nodes========="
  echo $NODES
#ec2-describe-instances | grep '^INSTANCE' | cut -f4 | xargs -I"HOST" ssh -i ~/.ssh/contextlattice.pem ubuntu@HOST ls
  echo "==========End====================="
  # execute one process at each of the nodes.
else
  echo "No matched configuration found"
fi
