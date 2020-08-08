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
#include "MaceTagServiceClass.h"
//#include "load_protocols.h"
//#include "ServiceFactory.h"
#include "MaceTag-init.h"
 
using namespace std;
 
class MaceTagResponseHandler : public MaceTagDataHandler {
 
}; 
 
int main(int argc, char* argv[]) {
  
  params::loadparams(argc, argv);
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
  }

  MaceTagServiceClass& mt = MaceTag_namespace::new_MaceTag_MaceTag();
  //MaceTagServiceClass& mt = mace::ServiceFactory< MaceTagServiceClass >::create( "MaceTag", true );
  mt.maceInit();
  SysUtil::sleep();
	return 0;
} 
