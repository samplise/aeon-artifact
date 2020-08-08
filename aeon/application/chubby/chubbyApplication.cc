#include "SysUtil.h"
#include "lib/mace.h"

#include "TcpTransport-init.h"
#include "load_protocols.h"
#include <signal.h>
#include <string>
#include "mlist.h"

#include "RandomUtil.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <pthread.h>
//#include "TagServiceClass.h"
//#include "Tag-init.h"
#include "ContextJobApplication.h"
#include "ChubbyServiceClass.h"
 
using namespace std;
 
/*class TagResponseHandler : public TagDataHandler {
 
};*/ 

void writeOutProf( int signum ){
  exit(EXIT_SUCCESS);
}

 
int main(int argc, char* argv[]) {
  mace::Init(argc, argv);
  load_protocols();

  if( params::get<bool>("gprof", false ) ){

    SysUtil::signal( SIGINT, writeOutProf ); // intercept ctrl+c and call exit to force gprof output
  }

  uint64_t runtime =  (uint64_t)(params::get<double>("run_time", 2) * 1000 * 1000);
  mace::string service = "SimpleChubby";
  mace::ContextJobApplication<ChubbyServiceClass> app;
  app.installSignalHandler();

  params::print(stdout);

  app.loadContext();


  std::cout << "Starting at time " << TimeUtil::timeu() << std::endl;
  app.startService( service );
  app.waitService( runtime );

  app.globalExit();
  
  return 0;
  /*params::loadparams(argc, argv);
  if( params::get<bool>("TRACE_ALL",false) == true )
      Log::autoAdd(".*");
  else if( params::containsKey("TRACE_SUBST") ){
        std::istringstream in( params::get<std::string>("TRACE_SUBST") );
        while(in){
            std::string logPattern;
            in >> logPattern;
            if( logPattern.length() == 0 ) break;

            Log::autoAdd(logPattern);
        }
  }*/

  //TagServiceClass& mt = Tag_namespace::new_Tag_Tag();
  //mt.maceInit();
  //timeval tim;
  //gettimeofday(&tim, NULL);
  /*double t1 = tim.tv_sec + (tim.tv_usec / 1000000.0);
  for (uint64_t i = 0; i < 4000000000; i++) {
  
  }
  gettimeofday(&tim, NULL);
  double t2 = tim.tv_sec + (tim.tv_usec / 1000000.0);
  printf("%.6lf seconds elapsed\n", t2 - t1);*/
  //SysUtil::sleep();
	return 0;
} 
