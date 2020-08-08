#include <pthread.h>
#include <stdio.h>

#include "ContextLock.h"
#include "mace.h"
#include "ThreadStructure.h"
#include "SysUtil.h"
#include "Event.h"

#define NUM_CTXLOCK 10
void* TicketThread(void *p);
void* AgentLockThread(void *p);
void* AgentLockNBThread(void *p);
void* NullAgentLockThread(void *p);
void* CtxlockThread(void *p);
void* NonblockingCtxlockThread(void *p);
void * HierarchicalContextLockThread( void *p );
void *accumulator(void *p);
int acquiredLocks[ NUM_CTXLOCK ];
int test_option = 0;
#define TESTOPTION_TICKET  0
#define TESTOPTION_AGENTLOCK  1
#define TESTOPTION_NULLAGENTLOCK  2
#define TESTOPTION_USECTXLOCK  3
#define TESTOPTION_HIERCTXLOCK  4
#define TESTOPTION_USENBCTXLOCK  5
#define TESTOPTION_AGENTLOCKNB  6

std::vector<std::string> ctxids;
std::list<std::string> ctx_created;
mace::ContextBaseClass* dummyContext;
pthread_mutex_t accumulator_lock = PTHREAD_MUTEX_INITIALIZER;;
bool finished = false;
int main(int argc, char *argv[]){
  params::set("NUM_ASYNC_THREADS", "1");
  params::set("NUM_TRANSPORT_THREADS", "1");
  mace::Init(argc, argv);
  if( params::containsKey("TRACE_ALL") ){
    Log::autoAdd(".*");
  }
  uint64_t beginTicket = 1;
  uint8_t serviceID = 0;
  uint32_t contextID = 1;
  dummyContext = new mace::ContextBaseClass("", beginTicket, serviceID, contextID);
  test_option = params::get<int>("test_option",1);
  // TODO set up one monitor thread periodically checking the state
  // other threads acquire ContextLock and release it continously to see if deadlock occurs.
  pthread_t ctxlock_threads[ NUM_CTXLOCK ];
  for(uint64_t thcounter = 0; thcounter < NUM_CTXLOCK; thcounter++ ){
    acquiredLocks[ thcounter ] = 0;
    switch( test_option ){
      case TESTOPTION_TICKET:{
        if( pthread_create( &ctxlock_threads[thcounter], NULL, TicketThread , (void*)thcounter ) != 0 ){
          perror("pthread_create");
        }
        break;
      }
      case TESTOPTION_AGENTLOCK:{
        if( pthread_create( &ctxlock_threads[thcounter], NULL, AgentLockThread , (void*)thcounter ) != 0 ){
          perror("pthread_create");
        }
        break;
      }
      case TESTOPTION_AGENTLOCKNB:{
        if( pthread_create( &ctxlock_threads[thcounter], NULL, AgentLockNBThread , (void*)thcounter ) != 0 ){
          perror("pthread_create");
        }
        break;
      }
      case TESTOPTION_NULLAGENTLOCK:{
        if( pthread_create( &ctxlock_threads[thcounter], NULL, NullAgentLockThread , (void*)thcounter ) != 0 ){
          perror("pthread_create");
        }
        break;
      }
      case TESTOPTION_USECTXLOCK:{
        if( pthread_create( &ctxlock_threads[thcounter], NULL, CtxlockThread , (void*)thcounter ) != 0 ){
          perror("pthread_create");
        }
        break;
      }
      case TESTOPTION_USENBCTXLOCK:{
        if( pthread_create( &ctxlock_threads[thcounter], NULL, NonblockingCtxlockThread , (void*)thcounter ) != 0 ){
          perror("pthread_create");
        }
        break;
      }
      case TESTOPTION_HIERCTXLOCK:{
        const char* ctxidstr[] = {"","A[0]", "A[0].B[0]", "A[1]", "A[2]", "A[0].B[1]", "A[0].B[2]", "A[0].B[3]", "A[0].B[4]", "A[0].B[5]", "A[0].B[6]", "A[0].B[7]", "A[0].B[8]", "A[0].B[9]", "A[0].B[10]", "A[0].B[11]"};
        ctxids.assign( ctxidstr, &ctxidstr[NUM_CTXLOCK]);
        if( pthread_create( &ctxlock_threads[thcounter], NULL, HierarchicalContextLockThread , (void*)thcounter ) != 0 ){
          perror("pthread_create");
        }
      }
        break;
    }
  }
  pthread_t accumulator_tid;
  ASSERT( pthread_create( &accumulator_tid, NULL, accumulator, (void*)NULL ) == 0 );
  for(int thcounter = 0; thcounter < NUM_CTXLOCK; thcounter++ ){
    void *ret;
    pthread_join( ctxlock_threads[thcounter], &ret  );
  }
  ScopedLock sl( accumulator_lock );
  finished = true;
  sl.unlock();
  void *ret;
  pthread_join( accumulator_tid, &ret  );
  delete dummyContext;
  return 0; 
}
void *accumulator(void *p){
  int last_total = 0;
  //for(int t=0;t< NUM_CTXLOCK;t++ ){
  while( true ){
    int total = 0;
    SysUtil::sleep(1);
    {
      ScopedLock sl( accumulator_lock );
      if( finished )
        break;
    }
    for(int c=0;c< NUM_CTXLOCK;c++){
      std::cout<< acquiredLocks[ c ] << " ";
      total+=acquiredLocks[ c ];
    }
    total -= last_total;
    last_total += total;
    std::cout<<" total= "<< total;
    std::cout<<std::endl;
  }

  pthread_exit(NULL);
  return NULL;
}
#define TICKET_PER_THREAD 1000*1000
void* TicketThread(void *p){
  int myid;
  memcpy(  &myid, (void*)&p, sizeof(int) );
  for( int locks=0; locks <  TICKET_PER_THREAD; locks++ ){
    ThreadStructure::newTicket();

    acquiredLocks[ myid ] ++;
  }
  std::cout<<"thread "<< myid <<" is leaving."<<std::endl;
  pthread_exit(NULL);
  return NULL;
}
#define AGENTLOCK_PER_THREAD 100000
void* AgentLockThread(void *p){
  int myid;
  memcpy(  &myid, (void*)&p, sizeof(int) );
  for( int locks=0; locks <  AGENTLOCK_PER_THREAD; locks++ ){
    ThreadStructure::newTicket();
    mace::AgentLock alock( mace::AgentLock::WRITE_MODE );

    acquiredLocks[ myid ] ++;
    alock.downgrade( mace::AgentLock::READ_MODE );
  }
  std::cout<<"thread "<< myid <<" is leaving."<<std::endl;
  pthread_exit(NULL);
  return NULL;
}
void myfunc(){

}
#define AGENTLOCKNB_PER_THREAD 700000
void* AgentLockNBThread(void *p){
  int myid;
  memcpy(  &myid, (void*)&p, sizeof(int) );
  for( int locks=0; locks <  AGENTLOCKNB_PER_THREAD; locks++ ){
    if( myid == 0 ){
      mace::AgentLock alock( mace::AgentLock::WRITE_MODE );

      alock.downgrade( mace::AgentLock::READ_MODE );

      alock.downgrade( mace::AgentLock::NONE_MODE );
    }else{
      ThreadStructure::newTicket();
      //mace::AgentLockNB alock( mace::AgentLockNB::WRITE_MODE, myfunc );
      //mace::AgentLock::skipTicket();
    }
    acquiredLocks[ myid ] ++;
  }
  std::cout<<"thread "<< myid <<" is leaving."<<std::endl;
  pthread_exit(NULL);
  return NULL;
}
void* NullAgentLockThread(void *p){
  int myid;
  memcpy(  &myid, (void*)&p, sizeof(int) );
  for( int locks=0; locks <  AGENTLOCK_PER_THREAD; locks++ ){
    ThreadStructure::newTicket();
    mace::AgentLock::nullTicket();

    acquiredLocks[ myid ] ++;
  }
  std::cout<<"thread "<< myid <<" is leaving."<<std::endl;
  pthread_exit(NULL);
  return NULL;
}
#define LOCK_PER_THREAD 100000
/* contextlock_test tests the simple functionality of ContextLock, 
 * that several threads can request write lock on the same context 
 * without deadlock */
void* CtxlockThread(void *p){
  int myid;
  uint8_t serviceID = 0;
  uint32_t contextID = 1;
  memcpy(  &myid, (void*)&p, sizeof(int) );
  for( int locks=0; locks <  LOCK_PER_THREAD; locks++ ){
    ThreadStructure::newTicket();
    mace::Event *he;
    {
      //mace::AgentLock alock( mace::AgentLock::WRITE_MODE );
      he = new mace::Event ( mace::Event::UNDEFEVENT );
    }
    he->getSkipIDStorage( serviceID ).set( contextID , he->getEventID());
    ThreadStructure::setEvent( *he );
    mace::ContextLock clock( /*mace::ContextBaseClass::headContext*/ *dummyContext, mace::ContextLock::WRITE_MODE );
    clock.downgrade( mace::ContextLock::NONE_MODE );

    acquiredLocks[ myid ] ++;
    delete he;
  }
  std::cout<<"thread "<< myid <<" is leaving."<<std::endl;
  pthread_exit(NULL);
  return NULL;
}

void* NonblockingCtxlockThread(void *p){

  int myid;
  memcpy(  &myid, (void*)&p, sizeof(int) );

  for( int locks=0; locks <  LOCK_PER_THREAD; locks++ ){
    ThreadStructure::newTicket();
    mace::AgentLock lock( mace::AgentLock::WRITE_MODE ); // global lock is used to ensure new events are created in order
    mace::Event& newEvent = ThreadStructure::myEvent( );
    newEvent.newEventID( mace::Event::UNDEFEVENT );
    { // Release global AgentLock. Acquire head context lock to allow paralellism
      //mace::ContextLock c_lock( mace::ContextBaseClass::headContext, mace::ContextLock::WRITE_MODE );

      newEvent.initialize(  );

        //contextEventRecord.updateContext( extra.targetContextID, newEvent.eventID, newEvent.getSkipIDStorage( instanceUniqueID ) );
      // notify other services about this event
      //BaseMaceService::globalNotifyNewEvent(  );

      //c_lock.downgrade( mace::ContextLock::NONE_MODE );
    }
    lock.downgrade( mace::AgentLock::NONE_MODE );

    acquiredLocks[ myid ] ++;
  }
  std::cout<<"thread "<< myid <<" is leaving."<<std::endl;
  pthread_exit(NULL);
  return NULL;
}

/* this test emulates the full set of hierarchical context, message exchange/forward */
void * HierarchicalContextLockThread( void *p ){
  int myid;
  memcpy(  &myid, (void*)&p, sizeof(int) );

  for( int locks=0; locks <  LOCK_PER_THREAD; locks++ ){
    ThreadStructure::newTicket();
    mace::AgentLock alock( mace::AgentLock::WRITE_MODE );
    mace::Event he( mace::Event::UNDEFEVENT );
    mace::AgentLock::downgrade( mace::AgentLock::NONE_MODE );
    ThreadStructure::setEvent(he.eventID );
    mace::ContextLock clock( /*mace::ContextBaseClass::headContext*/ *dummyContext, mace::ContextLock::WRITE_MODE );
    clock.downgrade( mace::ContextLock::NONE_MODE );

    acquiredLocks[ myid ] ++;
  }

  pthread_exit(NULL);
  return NULL;
}
