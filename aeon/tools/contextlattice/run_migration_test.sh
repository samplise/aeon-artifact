TEMPLATE_DIR=$1
HOSTNAME=`hostname`
PARAM_FILE="/tmp/params.testrun_twonode"
sed "s/__HOST__/$HOSTNAME/" < $TEMPLATE_DIR/params.testrun_twonode_template > $PARAM_FILE

./testrun $PARAM_FILE -MACE_PORT 9050 &
./testrun $PARAM_FILE -MACE_PORT 9000 &

sleep 40
killall testrun
