#include "SysUtil.h"
#include "lib/mace.h"
#include "GenericGameOfLifeServiceClass.h"

#include "TcpTransport-init.h"
#include "GameOfLife-init.h"

MaceKey me;
static bool isClosed = false;
#include "RandomUtil.h"

int main(int argc, char* argv[]) {
  ADD_SELECTORS("main");
  params::addRequired("MACE_PORT", "Port to use for connections.");
  //params::addRequired("bootstrap", "IP:port of bootstrap node");
  params::loadparams(argc, argv);

  // Logging for HW1Epidemic
  //Log::autoAdd("MicroBenchmark::");
  if( params::get<bool>("TRACE_ALL") == true )
      Log::autoAdd(".*");
  else if( params::containsKey("TRACE_SUBST") ){
        std::istringstream in( params::get<std::string>("TRACE_SUBST") );
        while(in){
            std::string logPattern;
            in >> logPattern;
            if( logPattern.length() == 0 ) break;

            Log::autoAdd(logPattern);
        }
  }

  MaceKey master = MaceKey(ipv4, params::get<std::string>("MACE_AUTO_BOOTSTRAP_PEERS") );
  
  TransportServiceClass& tcp =
    TcpTransport_namespace::new_TcpTransport_Transport();
  MaceKey me = tcp.localAddress();
 
  GenericGameOfLifeServiceClass& gameoflifeApp = GameOfLife_namespace::new_GameOfLife_GenericGameOfLife(tcp);
  // Call maceInit
  gameoflifeApp.maceInit();
  
  //gameoflifeApp.start();

  std::cout<<"me="<<me<<",master="<<master<<std::endl;
  while( isClosed == false ){
      if (me == master) {
          std::cout<<"i'm master"<<std::endl;
          SysUtil::sleepm(1000*240);
      }else{
          std::cout<<"i'm worker"<<std::endl;
          //SysUtil::sleepm(1000*5);
          SysUtil::sleepm(1000* 240);
      }
      isClosed = true;
  }
  std::cout<<"sleep finished"<<std::endl;

  gameoflifeApp.maceExit();
  std::cout<<"maceExit() called"<<std::endl;
  Scheduler::haltScheduler();
  std::cout<<"scheduler halt"<<std::endl;
  //gossip.subscribeGossip(0);

  /*benchmark.joinBenchmark( master );
  if (me == bootstrap) {
    //benchmark.queryLatencyTest();
  }else{
    std::string bootstrapper_node = params::get<std::string>("MACE_AUTO_BOOTSTRAP_PEERS");
    MaceKey master(ipv4, bootstrapper_node);
    // Just listen
    //std::cout << "Listening for gossips.." << std::endl;
    SysUtil::sleep(0);
    return 0;
  }*/
  //std::cout<<"Initiates Micro benchmark"<<std::endl;

  //benchmark.

  /*std::string msg = "hello-world-";
  int msgId = 0;
  while (1) {
    std::cout << "Enter 'y' to publish, 'n' to quit" << std::endl;
    char choice;
    std::cin >> choice;
    if (choice == 'n')
      break;
    if (choice == 'y') {
      std::ostringstream ss;
      ss << msg << msgId++;
      std::cout << "Publishing : (" << ss.str() << ")" << std::endl;
      // Publish
      gossip.publishGossip(0, ss.str());
    }
  }*/

  return 0;
}
