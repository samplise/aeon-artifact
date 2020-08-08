if [ $# -gt 3 ]
then
python generate.py $1 $2 $5 `date "+%s.%N"` $6 $7 > ../../../build/mace/application/appgroupstream/run_$1_$2.sh
chmod 777 ../../../build/mace/application/appgroupstream/run_$1_$2.sh
cd ../../../build/mace/application/appgroupstream
./run_$1_$2.sh $3 $4
pkill groupstream
pkill groupstream2
else
echo "Usage : ./test.sh [num_nodes] [num_transport] [delay] [period] [num_messages] [logging (0|1)]"
fi
