# Name : generate.py
# Description : It generates run_xx script.
import sys

#machine = "128.210.18.91"
#machine = "augustus"
bootstrap = "IPV4/cloud01:10000"
#machines = ["cloud01", "cloud02", "cloud03", "cloud04", "cloud05", "cloud06", "cloud07", "cloud08", "cloud09", "cloud10", "cloud11", "cloud12", "cloud13", "cloud14", "cloud15"]
machines = ["cloud01", "cloud02", "cloud03", "cloud04", "cloud05", "cloud06", "cloud07", "cloud08", "cloud09", "cloud10"]

# machine = "ruapehu"
#macedir = '/home/vision82/mace-project'
macedir = '/homes/yoo7/scratch/mace-project'
bindir = '/homes/yoo7/scratch/mace-project/build/mace/application/appgroupstream/'
ip_start = 10000
#node = int(sys.argv[1])
node_per = int(sys.argv[1])
num_threads = int(sys.argv[2])
num_messages = int(sys.argv[3])
current = sys.argv[4]
logdir = '/homes/yoo7/scratch/logs/'+str(current)+'/'
ip_interval = 5
allgroup = ""
gdb_cmd = "gdb --command=/homes/yoo7/scratch/mace-project/mace/application/appgroupstream/gdb_cmd.txt --args "
#gdb_cmd = ""
logging_para = ""
downgrade_to_none = ""
if int(sys.argv[5]) == 1:
  logging_para = " -MACE_LOG_LEVEL 1 -MACE_LOG_AUTO_SELECTORS \"AgentLock BaseMaceService ERROR snapshot\" "
  #logging_para = " -MACE_LOG_LEVEL 1 -MACE_LOG_AUTO_ALL 1 -MACE_LOG_ASYNC_FLUSH 1 "

if int(sys.argv[6]) == 1:
  downgrade_to_none = " -DOWNGRADE_TO_NONE 1 "

for machine in machines: 
  for i in range(0,node_per):
  	if allgroup == "":
  		allgroup = "IPV4/"+machine+":"+str(ip_start + i*(4*ip_interval))
  	else:
  		allgroup += " IPV4/"+machine+":"+str(ip_start + i*(4*ip_interval))

print "host=`hostname -s`\n\
pkill groupstream\n\
pkill groupstream2\n\
pkill groupstream3\n\
ulimit -c unlimited\n\
echo \"Running $host\"\n\
"
for machine in machines:
  print 'if [ "$host" == "' + machine + "\" ]\n\
         then\n\
         "
  for i in range(0,node_per):
        print 'mkdir -p '+logdir+'/'+machine+'_'+str(i)+"\n"
        print 'cd '+logdir+'/'+machine+'_'+str(i)+"\n"
        #print "rm ./*\n" 
        print gdb_cmd+bindir+'/groupstream3 -BOOTSTRAP_NODES "' + bootstrap + '" -ALL_NODES "' + allgroup + '" ' + logging_para + downgrade_to_none + ' -MACE_ADDRESS_ALLOW_LOOPBACK 1 -MACE_PORT '+str(ip_start + i*(4*ip_interval)) + " -NUM_MESSAGES "+str(num_messages)+" -NUM_THREADS "+str(num_threads)+" -IP_INTERVAL "+str(ip_interval)+" -DELAY $1 -PERIOD $2 -CURRENT "+str(current)+" > ../log_"+str(node_per)+"_"+str(num_threads)+"_$1_$2_" + machine + "_" + str(i) + ".txt 2>&1 &\n"
  print "fi\nwait\n"

