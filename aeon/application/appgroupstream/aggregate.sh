cat ../../../build/mace/application/appgroupstream/$1 | grep "Final average" | sed 's/\* Final average delay = //g'
