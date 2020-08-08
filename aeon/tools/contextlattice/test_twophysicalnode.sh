TEMPLATE_DIR=$1
HOSTNAME=`hostname`
PARAM_FILE="/tmp/params.testrun3"
sed "s/__HOST__/$HOSTNAME/" < $TEMPLATE_DIR/params.testrun3_template > $PARAM_FILE

./testrun $PARAM_FILE -MACE_PORT 9050 &
./testrun $PARAM_FILE -MACE_PORT 9000 &

sleep 100
