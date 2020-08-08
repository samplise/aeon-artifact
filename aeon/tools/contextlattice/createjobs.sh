#!/bin/bash
cp template.job heartbeat.job
echo "queue $1" >> heartbeat.job
condor_submit heartbeat.job
