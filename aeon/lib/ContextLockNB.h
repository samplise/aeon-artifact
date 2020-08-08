#ifndef _CONTEXTLOCKNB_H
#define _CONTEXTLOCKNB_H

#include "mace.h"
#include "GlobalCommit.h"
#include "ContextBaseClass.h"
#include "pthread.h"
#include "ThreadStructure.h"
#include "Message.h"

namespace mace{

#ifdef _USE_CONTEXTLOCKNB_
// maybe I don't need this.
class ContextBaseClass;
class ContextLockNB;

class ContextLockNB {
private:
    BaseMaceService* service;
    ContextBaseClass& context;
    ContextThreadSpecific *contextThreadSpecific;
  public:
    const int8_t requestedMode;
    Message* msg;
    const bool blockingMode;
    //const int8_t priorMode;
    int8_t priorMode;
    uint64_t myTicketNum;
  private:
    static pthread_mutex_t _context_ticketbooth; // chuangw: single ticketbooth for now. we will see if it'd become a bottleneck.

  public:
    static const int8_t WRITE_MODE = 1;
    static const int8_t READ_MODE = 0;
    static const int8_t NONE_MODE = -1;

    static const int8_t BLOCKING = 1;
    static const int8_t NONBLOCKING = 0;
  public:

public:
    void printError(){
      maceerr<< "[" << context.contextID<<"] myTicketNum = "<< myTicketNum <<"\n";
      maceerr<< "size of uncommittedEvents: "<< context.uncommittedEvents.size() <<"\n";
      for(uceventIt = context.uncommittedEvents.begin(); uceventIt != context.uncommittedEvents.end(); uceventIt++){
          maceerr<< "uncommit event: ticket="<< uceventIt->first <<", mode=" << (int16_t)uceventIt->second  << "\n";
      }
      maceerr<< "context.now_serving="<< context.now_serving <<", context.now_committing="<< context.now_committing<<Log::endl;
    }
    ContextLockNB( BaseMaceService* service, ContextBaseClass& ctx, int8_t requestedMode, const Message* msg, const bool blockingMode ): 
      service(service), context(ctx),  requestedMode( requestedMode), msg(msg), 
        blockingMode( blockingMode ), myTicketNum(ThreadStructure::myEvent().eventID){

      ADD_SELECTORS("ContextLockNB::(constructor)");
      ASSERT( myTicketNum > 0 );

      if( blockingMode == BLOCKING ){
        contextThreadSpecific = ctx.init();
      }

      ScopedLock sl(_context_ticketbooth);
      if( myTicketNum < context.now_serving ){
        mace::map<uint64_t, int8_t>::iterator uceventIt = context.uncommittedEvents.find( myTicketNum );
        if( uceventIt != context.uncommittedEvents.end() ){
            priorMode = uceventIt->second;
            if(  priorMode >= READ_MODE  && requestedMode == priorMode ){
                return; // ready to go!
            }else if( (priorMode >= READ_MODE ) && requestedMode < priorMode ){
                downgrade( requestedMode );
                return;
            }else{
                printError();
                ABORT("unexpected event mode change");
            }
        }else{
            printError();
            ABORT("ticket number is less than now_serving, but the ticket did not appear in uncommittedEvents list");
        }
        break;
      }
      if (requestedMode == NONE_MODE) {
        nullTicket();
      } else { 
        upgradeFromNone(); 
      }
      //macedbg(1) << "[" <<context.contextID <<"] CONTINUING.  priorMode " << (int16_t)priorMode << " requestedMode " << (int16_t)requestedMode << " myTicketNum " << myTicketNum << Log::endl;

    }
    ~ContextLockNB(){ 
    }
    
private:
    void upgradeFromNone(){ 
      ADD_SELECTORS("ContextLockNB::upgradeFromNone");
      ASSERTMSG(requestedMode == READ_MODE || requestedMode == WRITE_MODE, "Invalid mode requested!");

      bool isReady = ticketBoothWait(requestedMode);
      if (requestedMode == READ_MODE) {
        // chuangw: this is the tricky case...
      }else{ // WRITE_MODE
        if( isReady ){
          ASSERT(context.numReaders == 0);
          ASSERT(context.numWriters == 0);
          //context.uncommittedEvents[ myTicketNum ] = WRITE_MODE;
          context.numWriters = 1;
          context.lastWrite = myTicketNum;

        }else{
          //context.eventQueue[ myTicketNum ] = mace::pair< int8_t, Message *>( requestedMode, msg );
        }
      }
    }
    void nullTicket() {
      ADD_SELECTORS("ContextLockNB::nullTicket");
      ASSERTMSG(requestedMode == NONE_MODE , "unexpected mode!");
      ADD_SELECTORS("ContextLockNB::nullTicket");
      bool isReady = ticketBoothWait(requestedMode);

      commitOrderWait();

    }

    bool ticketBoothWait(int8_t requestedMode){
      ADD_SELECTORS("ContextLockNB::ticketBoothWait");



      ASSERT(myTicketNum == context.now_serving); //Remove once working.

      context.now_serving++;
    }

    void downgrade(int8_t newMode) {
      ADD_SELECTORS("ContextLockNB::downgrade");
    }
    void downgradeToNone(int8_t runningMode) {
        ADD_SELECTORS("ContextLockNB::downgradeToNone");
    }
    void downgradeToRead() {
      ADD_SELECTORS("ContextLockNB::downgradeToRead");
    }
    void commitOrderWait() {
      ADD_SELECTORS("ContextLockNB::commitOrderWait");
    }
};
#endif

}

#endif
