launch_heartbeat
sleep 15
show node
start job.spec job.input
show node
show job
sleep 10
migrate 1
sleep 5
show node
kill all
sleep 15
show node
show job
exit
