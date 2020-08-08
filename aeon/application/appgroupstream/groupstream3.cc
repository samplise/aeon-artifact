#include <boost/algorithm/string.hpp>
#include "SysUtil.h"
#include "lib/mace.h"
#include "lib/SysUtil.h"

#include "services/ReplayTree/ReplayTree-init.h"
#include "services/RandTree/RandTree-init.h"
#include "services/GenericTreeMulticast/GenericTreeMulticast-init.h"
#include "services/GenericTreeMulticast/DeferredGenericTreeMulticast-init.h"
#include "services/SignedMulticast/SignedMulticast-init.h"
#include "services/SignedMulticast/DeferredSignedMulticast-init.h"
#include "services/Pastry/Pastry-init.h"
#include "services/Bamboo/Bamboo-init.h"
#include "services/Bamboo/DeferredBamboo-init.h"
#include "services/ScribeMS/ScribeMS-init.h"
#include "services/ScribeMS/Scribe-init.h"
#include "services/ScribeMS/DeferredScribeMS-init.h"
#include "services/GenericOverlayRoute/CacheRecursiveOverlayRoute-init.h"
#include "services/GenericOverlayRoute/RecursiveOverlayRoute.h"
#include "services/GenericOverlayRoute/RecursiveOverlayRoute-init.h"
#include "TcpTransport.h"
#include "TcpTransport-init.h"
#include "UdpTransport.h"
#include "UdpTransport-init.h"
#include "services/Transport/DeferredRouteTransportWrapper.h"
#include "services/Transport/DeferredRouteTransportWrapper-init.h"
#include "services/Transport/RouteTransportWrapper.h"
#include "services/Transport/RouteTransportWrapper-init.h"


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

      if (delay > 1000.0) 
      {
        //printf("exception: weird time sent detected in app: %llu %llu\n", TimeUtil::timeu(), boost::lexical_cast<uint64_t>(msg.data()));
      }
      else 
      {
        // calculate critical delay
        assert(msg_id >= 0);
      }
      std::ostringstream os;
      os << "* Message [ "<< strs[0] << " ] from : "<< strs[2] << "  Channel : " << serviceUid << "  Delay : " << delay << " Sent: " << strs[1] << " Now: " << TimeUtil::timeu() << " (source: " << source << " dest: " << dest << " )" << std::endl;
      std::cout << os.str();
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
  params::addRequired("NUM_THREADS", "Number of transport threads.");
  params::addRequired("IP_INTERVAL", "If multiple transport used, number of intervals between IP needs to be specified.");
  params::addRequired("DELAY", "Delay for time.");
  params::addRequired("PERIOD", "Wait time to send message periodically.");
  params::loadparams(argc, argv);

  Log::configure();

  Log::autoAddAll();

  /* Get parameters */
  myip = MaceKey(ipv4, Util::getMaceAddr());
  mygroup = MaceKey(sha160, myip.toString());
  int delay = params::get("DELAY", 1000000);
  int period = params::get("PERIOD", 6000000);
  int num_threads = params::get("NUM_THREADS", 1);
  int ip_interval = params::get("IP_INTERVAL", 1);
  int num_messages = params::get("NUM_MESSAGES", 10);
  double current_time = params::get<double>("CURRENT", 0);

  printf("* myip: %s\n", myip.toString().c_str());
  printf("* mygroup: %s\n", mygroup.toString().c_str());
  printf("* delay: %d\n", delay);
  printf("* period: %d\n", period);
  printf("* num_threads: %d\n", num_threads);
  printf("* ip_interval: %d\n", ip_interval);
  printf("* current_time: %.4f\n", current_time);

  /* Get nodeset and create subgroups */
  NodeSet bootGroups = params::get<NodeSet>("BOOTSTRAP_NODES");  // ipv4
  NodeSet allGroups = params::get<NodeSet>("ALL_NODES");  // ipv4

  std::list<ServiceClass*> thingsToExit;


  /* Create services */
  MyHandler* appHandler;

  appHandler = new MyHandler();

  TransportServiceClass *ntcp = &(TcpTransport_namespace::new_TcpTransport_TransportEx(num_threads));
  TransportServiceClass *udp = &(UdpTransport_namespace::new_UdpTransport_Transport());  // 1

  RouteServiceClass* rtw = &(DeferredRouteTransportWrapper_namespace::new_DeferredRouteTransportWrapper_Route(*ntcp));  // 1
  //RouteServiceClass* rtw = &(RouteTransportWrapper_namespace::new_RouteTransportWrapper_Route(*ntcp));  // 1
  OverlayRouterServiceClass* bamboo = &(Bamboo_namespace::new_Bamboo_OverlayRouter(*rtw, *udp));  // 1
  OverlayRouterServiceClass* dbamboo = &(DeferredBamboo_namespace::new_DeferredBamboo_OverlayRouter(*bamboo));  // 1
  RouteServiceClass* ror = &(RecursiveOverlayRoute_namespace::new_RecursiveOverlayRoute_Route(*dbamboo, *ntcp));  // 1
  ScribeTreeServiceClass *scribe = &(ScribeMS_namespace::new_ScribeMS_ScribeTree(*bamboo, *ror));  // 1
  TreeServiceClass *dscribe = &(DeferredScribeMS_namespace::new_DeferredScribeMS_Tree(*scribe));  // 1
  RouteServiceClass* cror = &(CacheRecursiveOverlayRoute_namespace::new_CacheRecursiveOverlayRoute_Route(*bamboo, *ntcp, 30)); 
  DeferredHierarchicalMulticastServiceClass* gtm = &(GenericTreeMulticast_namespace::new_GenericTreeMulticast_DeferredHierarchicalMulticast(*cror, *dscribe));
  HierarchicalMulticastServiceClass* dgtm = &(DeferredGenericTreeMulticast_namespace::new_DeferredGenericTreeMulticast_HierarchicalMulticast(*gtm));
  MulticastServiceClass* sm = &(SignedMulticast_namespace::new_SignedMulticast_Multicast(*dgtm, delay));
  MulticastServiceClass* dsm = &(DeferredSignedMulticast_namespace::new_DeferredSignedMulticast_Multicast(*sm));
  thingsToExit.push_back(dsm);

//  sm->maceInit();
//  sm->registerHandler(*appHandler, appHandler->uid);
  dsm->maceInit();
  dsm->registerHandler(*appHandler, appHandler->uid);

  /* Now register services */

  /* Now handle groups */
  ASSERT(!allGroups.empty());

  /* Joining overlay */
  std::cout << "* AllGroups listed are : " << allGroups << std::endl;;

  uint64_t sleepJoinO = (uint64_t)current_time * 1000000 + 5000000 - TimeUtil::timeu();
  std::cout << "* Waiting until 5 seconds into run to joinOverlay... ( " << sleepJoinO << " micros left)" << std::endl;
  SysUtil::sleepu(sleepJoinO);

  bamboo->joinOverlay(bootGroups);

  uint64_t sleepJoinG = (uint64_t)current_time * 1000000 + 25000000 - TimeUtil::timeu();
  std::cout << "* Waiting until 25 seconds into run to joinGroup... ( " << sleepJoinG << " micros left)" << std::endl;
  SysUtil::sleepu(sleepJoinG);


  std::cout << "* Creating/joining self-group " << mygroup << std::endl;
  dynamic_cast<GroupServiceClass*>(scribe)->createGroup(mygroup, appHandler->uid);  // create self group

  for (NodeSet::const_iterator j = allGroups.begin();j != allGroups.end(); j++) {
    std::cout << "* Joining group " << (*j) << std::endl;
    dynamic_cast<GroupServiceClass*>(scribe)->joinGroup(MaceKey(sha160, (*j).toString()), appHandler->uid);
  }

  std::cout << "* Done in joining" << std::endl;

  uint64_t sleepMessage = (uint64_t)current_time * 1000000 + 45000000 - TimeUtil::timeu();

  std::cout << "* Waiting until 45 seconds into run to start messaging... ( " << sleepMessage << " left)" << std::endl;

  SysUtil::sleepu(sleepMessage);

  std::cout << "* Messaging..." << std::endl;

  /* Basically, it will keep sending to their groups. */
  int msg_id = 0;
  while(num_messages-->0)
  {
    std::cout << "* Sending message [ "<<msg_id<<" ] " << std::endl;
    std::stringstream s;
    s << msg_id << "," << TimeUtil::timeu() << "," << myip.toString().c_str();
//    sm->multicast(mygroup, s.str(), appHandler->uid);
    dsm->multicast(mygroup, s.str(), appHandler->uid);
    msg_id++;
    SysUtil::sleepu(period);
  }

  SysUtil::sleepu(5000000);

  std::cout << "* Halting." << std::endl;

  // All done.
  for(std::list<ServiceClass*>::iterator ex = thingsToExit.begin(); ex != thingsToExit.end(); ex++) {
    (*ex)->maceExit();
  }
  Scheduler::haltScheduler();

  return 0;

}
