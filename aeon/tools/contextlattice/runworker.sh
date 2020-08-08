#!/bin/bash

# TODO: evenly distribute nodes onto ten machines
HOSTS=10
DIV=`expr  $1 / $HOSTS `
MOD=`expr $1 % $HOSTS `
procIDStart=1
procIDEnd=0
for (( hostNo=1; hostNo<= $HOSTS; hostNo++ ))
do
    machineID=`printf "%02d" $hostNo`

    if [ $1 -le $HOSTS ]
    then
        procIDStart=$hostNo
        procIDEnd=$hostNo
    else
        procIDStart=`expr $procIDEnd + 1`
        if [ $hostNo -le $MOD ]
        then
            procIDEnd=`expr $procIDEnd + $DIV + 1`
        else
            procIDEnd=`expr $procIDEnd + $DIV`
        fi
    fi

    if [ $hostNo -le $1 ]
    then
        echo "ssh cloud"$machineID".cs.purdue.edu ./multiworker.sh $procIDStart $procIDEnd &" | /bin/bash > /dev/null
    fi
done
