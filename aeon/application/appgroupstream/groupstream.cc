#include <boost/algorithm/string.hpp>
#include "SysUtil.h"
#include "lib/mace.h"
#include "lib/SysUtil.h"
#include "GlobalCommit.h"

#include "services/ReplayTree/ReplayTree-init.h"
#include "services/RandTree/RandTree-init.h"
#include "services/GenericTreeMulticast/GenericTreeMulticast-init.h"
#include "services/SignedMulticast/SignedMulticast-init.h"
//#include "services/SignedMulticast/DeferredSignedMulticastWrapper-init.h"
#include "services/Pastry/Pastry-init.h"
#include "services/Bamboo/Bamboo-init.h"
#include "services/ScribeMS/ScribeMS-init.h"
#include "services/ScribeMS/Scribe-init.h"
#include "services/GenericOverlayRoute/CacheRecursiveOverlayRoute-init.h"
#include "services/GenericOverlayRoute/RecursiveOverlayRoute.h"
#include "services/GenericOverlayRoute/RecursiveOverlayRoute-init.h"
#include "services/Transport/TcpTransport.h"
#include "services/Transport/TcpTransport-init.h"
#include "services/Transport/UdpTransport.h"
#include "services/Transport/UdpTransport-init.h"
#include "services/Transport/DeferredRouteTransportWrapper.h"
#include "services/Transport/DeferredRouteTransportWrapper-init.h"

/* Variables */
MaceKey myip;  //This is always the IP address
MaceKey mygroup; // This is the local group_id.

/* Calculate moving average of latency */
int gotten = 0;   // number of pkts received
double avg_lat = 0.0;  // the average latency of received pkts
std::vector<double> arr_lat;
std::map<int,double> arr_crit;
typedef std::set<std::string> ip_list;
std::map<int,ip_list> arr_crit_list;

pthread_mutex_t deliverMutex;


class MyHandler : public ReceiveDataHandler {
  public:
  MyHandler() : uid(NumberGen::Instance(NumberGen::HANDLER_UID)->GetVal())
  {}

  const int uid;

  /* source, dest(=group_id), msg, uid */
  void deliver(const MaceKey& source, const MaceKey& dest, const std::string& msg, registration_uid_t serviceUid)
  {
    double delay = 0.0;
    int msg_id = 0;

    ScopedLock sl(deliverMutex);

    if (msg.size() > sizeof(double)) 
    {

      std::vector<std::string> strs;
      std::string s = msg;
      boost::split(strs, s, boost::is_any_of(","));

      assert(strs.size() > 2 );

      //std::cout << "Message process.." << std::endl;

      msg_id = (int) boost::lexical_cast<uint64_t>(strs[0]);
      delay = ((double)TimeUtil::timeu() - boost::lexical_cast<uint64_t>(strs[1]))/1000000;

      /*if (delay > 1000.0) 
      {
        printf("exception: weird time sent detected in app: %llu %llu\n", TimeUtil::timeu(), boost::lexical_cast<uint64_t>(msg.data()));
      }*/
      //else 
      //{

        // calculate critical delay

        assert(msg_id >= 0);

        std::map<int,double>::iterator it = arr_crit.find(msg_id);

        if( it != arr_crit.end() )    // if already exist..
        {
          // find if we already inserted with given node string
          ip_list list = arr_crit_list[msg_id];
          std::set<std::string>::iterator jt = list.find(strs[2]);

          if( arr_crit[msg_id] < delay && jt == list.end() )   // Delay should be bigger than already stored one AND it should be the first ip address as seen so far.
          {
            arr_crit[msg_id] = delay;
            arr_crit_list[msg_id].insert(strs[2]);    // insert ip address
          }
        }
        else
        {
          arr_crit[msg_id] = delay;
          arr_crit_list[msg_id].insert(strs[2]);    // insert ip address
        }

        // Now calculate average delay
        arr_lat.push_back(delay);

        double total_sum = 0;
        for( int i=0; i<(int)arr_lat.size(); i++)
        {
          total_sum += arr_lat[i];
        }
        avg_lat = total_sum / arr_lat.size();

        gotten++;
      //}
      std::cout << "* Message ["<< strs[0] << "] from : "<< strs[2] << "  Channel : " << serviceUid << "  Delay : " << delay << "  Critical : " << arr_crit[msg_id] << "  Average : " << avg_lat << " (checking source: " << source << " dest: " << dest << " )" << std::endl;
    }
    return;
  }
};


int main(int argc, char* argv[]) {

  pthread_mutex_init(&deliverMutex, 0);

  /* Add required params */
  params::addRequired("MACE_PORT", "Port to use for connections. Allow a gap of 10 between processes");
  params::addRequired("BOOTSTRAP_NODES", "Addresses of nodes to add. e.g. IPV4/1.1.1.1:port IPV4/2.2.2.2 ... (be aware of uppercases!)");
  params::addRequired("ALL_NODES", "Addresses of nodes to add. e.g. IPV4/1.1.1.1:port IPV4/2.2.2.2 ... (be aware of uppercases!)");
  params::addRequired("NUM_TRANSPORT", "Number of transports.");
  params::addRequired("IP_INTERVAL", "If multiple transport used, number of intervals between IP needs to be specified.");
  params::addRequired("DELAY", "Delay for time.");
  params::addRequired("PERIOD", "Wait time to send message periodically.");
  params::loadparams(argc, argv);

  Log::configure();

  /* Get parameters */
  myip = MaceKey(ipv4, Util::getMaceAddr());
  mygroup = MaceKey(sha160, myip.toString());
  int delay = params::get("DELAY", 1000000);
  int period = params::get("PERIOD", 6000000);
  int num_transport = params::get("NUM_TRANSPORT", 1);
  int ip_interval = params::get("IP_INTERVAL", 1);
  int num_messages = params::get("NUM_MESSAGES", 10);
  double current_time = params::get<double>("CURRENT", 0);

  printf("* myip: %s\n", myip.toString().c_str());
  printf("* mygroup: %s\n", mygroup.toString().c_str());
  printf("* delay: %d\n", delay);
  printf("* period: %d\n", period);
  printf("* num_transport: %d\n", num_transport);
  printf("* ip_interval: %d\n", ip_interval);
  printf("* current_time: %.4f\n", current_time);

  /* Get nodeset and create subgroups */
  NodeSet bootGroups = params::get<NodeSet>("BOOTSTRAP_NODES");  // ipv4
  NodeSet allGroups = params::get<NodeSet>("ALL_NODES");  // ipv4

  std::list<ServiceClass*> thingsToExit;

  //Log::autoAddAll();
//  Log::autoAdd("TRACE");
//  Log::autoAdd("BaseTransport::deliverSetMessage");
//  Log::autoAdd("BaseTransport::BaseTransport");
//  Log::autoAdd("BaseTransport::setupThreadPool");
//  Log::autoAdd("BaseTransport::startDeliverThread");
//  Log::autoAdd("DeliveryTransport");
//  Log::autoAdd("DeliveryTransport::setMessage");
//  Log::autoAdd("DeliveryTransport::runDeliver");
//  Log::autoAdd("DeliveryTransport::isReady");
//  Log::autoAdd("ThreadPool");
//  Log::autoAdd("ERROR");

  /* Create services */

  TransportServiceClass *tcp = &(TcpTransport_namespace::new_TcpTransport_Transport());  // 1
  //   thingsToExit.push_back(tcp);
  TransportServiceClass *udp = &(UdpTransport_namespace::new_UdpTransport_Transport());  // 1
  //   thingsToExit.push_back(udp);
  RouteServiceClass* rtw = &(DeferredRouteTransportWrapper_namespace::new_DeferredRouteTransportWrapper_Route(*tcp));  // 1
  //   thingsToExit.push_back(rtw);
  OverlayRouterServiceClass* bamboo = &(Bamboo_namespace::new_Bamboo_OverlayRouter(*rtw, *udp));  // 1
  //   thingsToExit.push_back(bamboo);
  RouteServiceClass* ror = &(RecursiveOverlayRoute_namespace::new_RecursiveOverlayRoute_Route(*bamboo, *tcp));  // 1
  //   thingsToExit.push_back(ror);
  TreeServiceClass *scribe = &(ScribeMS_namespace::new_ScribeMS_Tree(*bamboo, *ror));  // 1
  //   thingsToExit.push_back(scribe);
  
  std::vector<MyHandler*> appHandler;
  std::vector<MaceKey> multicast_local_addr;
  std::vector<NodeSet*> subGroups;

  //std::vector<TransportServiceClass*> ntcp;    // N
  std::vector<RouteServiceClass*> cror;    // N
  std::vector<HierarchicalMulticastServiceClass*> gtm;    // N
  std::vector<MulticastServiceClass*> sm;    // N

  appHandler.reserve(num_transport);
  subGroups.reserve(num_transport);

  //ntcp.reserve(num_transport);
  cror.reserve(num_transport);
  gtm.reserve(num_transport);
  sm.reserve(num_transport);

  /* Now register services */

  TransportServiceClass *ntcp;                   // N (threads)
  ntcp = &(TcpTransport_namespace::new_TcpTransport_TransportEx(num_transport));  // N + 1
  //   thingsToExit.push_back(ntcp);

  for( int i=0; i<num_transport; i++ )
  {
    appHandler[i] = new MyHandler();
//    ntcp[i] = &(TcpTransport_namespace::new_TcpTransport_TransportEx(10));  // N + 1
//    thingsToExit.push_back(ntcp[i]);
    cror[i] = &(CacheRecursiveOverlayRoute_namespace::new_CacheRecursiveOverlayRoute_Route(*bamboo, *ntcp, 30));  // N
    //     thingsToExit.push_back(cror[i]);
    gtm[i] = &(GenericTreeMulticast_namespace::new_GenericTreeMulticast_HierarchicalMulticast(*cror[i], *scribe));  // N
    //     thingsToExit.push_back(gtm[i]);
    sm[i] = &(SignedMulticast_namespace::new_SignedMulticast_Multicast(*gtm[i], delay));  // N
    thingsToExit.push_back(sm[i]);

    sm[i]->maceInit();
    sm[i]->registerHandler(*appHandler[i], appHandler[i]->uid);
    multicast_local_addr.push_back(sm[i]->localAddress());
    subGroups[i] = new NodeSet();
  }

  /* Now handle groups */
  ASSERT(!allGroups.empty());

  /* Joining overlay */
  std::cout << "* AllGroups listed are : " << std::endl;;

  int t = 0;
  int pos = -1;
  for (NodeSet::const_iterator j = allGroups.begin(); j != allGroups.end(); j++, t++) {
    subGroups[t % num_transport]->insert(*j);    // insert into subgroup
    MaceKey group = MaceKey(sha160, (*j).toString());    // sha160
    if (*j == myip) {
      pos = t;
    }
    std::cout << (*j) << " (" << group.toString() << ")" << std::endl;
  }
  ASSERT(pos != -1);

  uint64_t sleepJoinO = (uint64_t)current_time * 1000000 + 5000000 - TimeUtil::timeu();
  std::cout << "* Waiting until 5 seconds into run to joinOverlay... ( " << sleepJoinO << " micros left)" << std::endl;
  SysUtil::sleepu(sleepJoinO);

  bamboo->joinOverlay(bootGroups);

  uint64_t sleepJoinG = (uint64_t)current_time * 1000000 + 25000000 - TimeUtil::timeu();
  std::cout << "* Waiting until 25 seconds into run to joinGroup... ( " << sleepJoinG << " micros left)" << std::endl;
  SysUtil::sleepu(sleepJoinG);


  std::cout << "* Creating/joining self-group " << mygroup << std::endl;
  dynamic_cast<GroupServiceClass*>(scribe)->createGroup(mygroup, appHandler[pos % num_transport]->uid);  // create self group

  for( int t=0; t < num_transport; t++ ) {
    for (NodeSet::const_iterator j = subGroups[t]->begin();j != subGroups[t]->end(); j++) {
      std::cout << "* Joining group " << (*j) << "  number = " << t << std::endl;
      dynamic_cast<GroupServiceClass*>(scribe)->joinGroup(MaceKey(sha160, (*j).toString()), appHandler[t]->uid);
    }
  }

  std::cout << "* Done in joining" << std::endl;

  // Deserialize nodes
  // To maximize the effect, sending time should be SYNCHRONIZED.

  //   std::cout << "* Waiting for 10 more seconds and then start messaging.." << std::endl;

  //   std::cout << "* current_time + 10 : " << (current_time + 10) << std::endl;
  //   std::cout << "* TimeUtil::timeu() : " << TimeUtil::timeu() << std::endl;

  uint64_t sleepMessage = (uint64_t)current_time * 1000000 + 45000000 - TimeUtil::timeu();

  std::cout << "* Waiting until 45 seconds into run to start messaging... ( " << sleepMessage << " left)" << std::endl;

  SysUtil::sleepu(sleepMessage);

  std::cout << "* Messaging..." << std::endl;

  /* Basically, it will keep sending to their groups. */
  int msg_id = 0;
  while(num_messages-->0)
  {
    std::cout << "* Sending message ["<<msg_id<<"] to channel " << pos % num_transport << std::endl;
    std::stringstream s;
    s << boost::lexical_cast<std::string>(msg_id) << "," << boost::lexical_cast<std::string>(TimeUtil::timeu()) << "," << myip.toString().c_str();
    sm[pos%num_transport]->multicast(mygroup, s.str(), appHandler[pos%num_transport]->uid);
    msg_id++;
    usleep(period);
  }

  usleep(5000000);

  std::cout << "* Final average delay = " << avg_lat << std::endl;

  double crit_lat = 0;

  for( std::map<int,double>::iterator it = arr_crit.begin(); it != arr_crit.end(); it++ )
  {
    if( it->second > crit_lat )
      crit_lat = it->second;
  }

  std::cout << "* Final critical delay = " << crit_lat << std::endl;


  // All done.
  for(std::list<ServiceClass*>::iterator ex = thingsToExit.begin(); ex != thingsToExit.end(); ex++) {
    (*ex)->maceExit();
  }
  Scheduler::haltScheduler();

  return 0;

  //////////// Original Handler ////////////
//
//  std::vector<MyHandler*> appHandler;
//  std::deque<GroupServiceClass*> groupServicesToJoin;
//  std::list<ServiceClass*> thingsToExit;
//  std::vector<MulticastServiceClass*> multicast;
//  std::vector<MaceKey> multicast_local_addr; // This is the return of globalRoute->getLocalAddress
//  std::vector<TreeServiceClass*> scribe;
//  
//  std::vector<RouteServiceClass*> route;
//  std::vector<MulticastServiceClass*> globalGenericTreeMulticast;
//
//  /* Reserve capacity */
//  appHandler.reserve(num_transport);
//  multicast.reserve(num_transport);
//  route.reserve(num_transport);
//  scribe.reserve(num_transport);
//  globalGenericTreeMulticast.reserve(num_transport);
//  subGroups.reserve(num_transport);
//
//  OverlayRouterServiceClass* bamboo;
//  bamboo = &(Bamboo_namespace::new_Bamboo_OverlayRouter());
//
//
//  /* Now register services */
//  for( int i=0; i<num_transport; i++ )
//  {
//    appHandler[i] = new MyHandler();
//    scribe[i] = &(ScribeMS_namespace::new_ScribeMS_Tree(*bamboo));
//    groupServicesToJoin.push_back(dynamic_cast<GroupServiceClass*>(scribe[i]));
//    route[i] = &CacheRecursiveOverlayRoute_namespace::new_CacheRecursiveOverlayRoute_Route(*bamboo);
//    ASSERT(scribe[i] != NULL);
//
//    globalGenericTreeMulticast[i] = &GenericTreeMulticast_namespace::new_GenericTreeMulticast_HierarchicalMulticast(*route[i], *scribe[i]);
//    multicast[i] = &SignedMulticast_namespace::new_SignedMulticast_Multicast(*globalGenericTreeMulticast[i], delay);
//    multicast[i]->maceInit();
//    thingsToExit.push_back(multicast[i]);
//    multicast[i]->registerHandler(*appHandler[i], appHandler[i]->uid);
//    multicast_local_addr.push_back(multicast[i]->getLocalAddress());
//    subGroups[i] = new NodeSet();
//  }
//
//  /* Wait for 10 seconds, create self-group, and join to the all other nodes(=groups) */
//  ASSERT(!allGroups.empty());
//
//  /* Joining overlay */
//  std::cout << "* AllGroups listed are : " << std::endl;;
//
//  int pos = -1;
//  int t = 0;
//  for (NodeSet::const_iterator j = allGroups.begin(); j != allGroups.end(); j++) {
//    subGroups[(t++) % num_transport]->insert(*j);    // insert into subgroup
//    MaceKey group = MaceKey(sha160, (*j).toString());    // sha160
//    if (*j == myip) {
//      pos = t;
//    }
//    std::cout << (*j) << " (" << group.toString() << ")" << std::endl;
//  }
//  ASSERT(pos != -1);
//  std::cout << std::endl;
//
//  bamboo->joinOverlay(allGroups); // ipv4

  /* Create group and join to the allGroups */
//
//  std::cout << "Creating/joining self-group " << mygroup << std::endl;
//  dynamic_cast<GroupServiceClass*>(scribe[pos % num_transport])->createGroup(mygroup, appHandler[pos % num_transport]->uid);
//
//  t = 0;
//  for(std::deque<GroupServiceClass*>::iterator i = groupServicesToJoin.begin(); i != groupServicesToJoin.end(); i++) {
//
//    for (NodeSet::const_iterator j = subGroups[t]->begin(); j != subGroups[t]->end(); j++) {
//      std::cout << "  Joining group " << (*j) << std::endl;
//      (**i).joinGroup(MaceKey(sha160, (*j).toString()), appHandler[t]->uid);
//    }
//    t++;
//  }
//
//  usleep(num_nodes * 100000);
//
//  /* Basically, it will keep sending to their groups. */
//  while(num_messages-->0)
//  {
//    //for( int i=0; i<num_transport; i++ )
//    //{
//      std::cout << "Sending message to Transport " << pos % num_transport << std::endl;
//      multicast[pos%num_transport]->multicast(mygroup, boost::lexical_cast<std::string>(TimeUtil::timeu()), appHandler[pos%num_transport]->uid);
//    //}
//
//    usleep(period); //usleep(avg_lat * 1000000 * 3);
//  }
//
//  std::cout << "Final average delay = " << avg_lat << std::endl;
//
//  // All done.
//  for(std::list<ServiceClass*>::iterator ex = thingsToExit.begin(); ex != thingsToExit.end(); ex++) {
//    (*ex)->maceExit();
//  }
//  Scheduler::haltScheduler();
//
//  return 0;
}
