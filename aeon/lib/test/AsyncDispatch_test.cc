
#include "mace.h"
#include "SysUtil.h"
#include "signal.h"
#include "AsyncDispatch.h"
#include "pthread.h"
#include "mace.h"
class Handler: public AsyncEventReceiver{
public:
  void func(void *p);
private:
  static pthread_mutex_t lock;
};

int count = 0;
Handler h;
pthread_mutex_t Handler::lock = PTHREAD_MUTEX_INITIALIZER;
#define NEVENT 1
void Handler::func(void *p){
  ScopedLock sl( lock );
  count++;
  int c = count;
  sl.unlock();
  //mace::AgentLock alock( mace::AgentLock::WRITE_MODE );
  if( c % NEVENT == NEVENT-1 ){
    for(int i =0; i< NEVENT;i ++){
      AsyncDispatch::enqueueEvent(this, (AsyncDispatch::asyncfunc)&Handler::func, (void*)( &c )  ) ;
    }
  }
  //alock.downgrade( mace::AgentLock::NONE_MODE );
  //mace::AgentLock::skipTicket();
}
void forceExit(int signum){
  exit(EXIT_SUCCESS);
}
int main(int argc, char* argv[]){
  //mace::Init(argc, argv);
  params::loadparams(argc, argv);
  Log::configure(); //This should probably be first
  AsyncDispatch::init();
  if( params::get<bool>("gprof", false) ){
    SysUtil::signal(SIGINT, forceExit);
  }

  //AsyncDispatch::init();
  int zero = 0;
  AsyncDispatch::enqueueEvent(&h, (AsyncDispatch::asyncfunc)&Handler::func, (void*)( &zero )  ) ;

  int total_time=10;
  int last_count = 0;
  for(int i=0;i< total_time; i++){
    sleep(1);
    std::cout<< count-last_count << std::endl;
    last_count = count;
  }
  
  //mace::Shutdown();
  AsyncDispatch::haltAndWait();
  Scheduler::haltScheduler(); //Keep this last!
}
