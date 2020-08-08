cat ../../../build/mace/application/appgroupstream/$1 | grep "Final critical" | sed 's/\* Final critical delay = //g'
