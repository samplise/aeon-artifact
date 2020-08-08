#ifndef _CONTEXTLOCK_H
#define _CONTEXTLOCK_H

// including headers
#include "mace.h"
#include "GlobalCommit.h"
#include "ContextBaseClass.h"
#include "pthread.h"
#include "ThreadStructure.h"
#include "m_map.h"
// uses snapshot by default

#define  MARK_RESERVED NULL
namespace mace{

class ContextBaseClass;
class ContextLock;

class ContextLock {
private:
    ContextBaseClass& context;
public:
  const int8_t requestedMode;
  int8_t priorMode;
  OrderID myEventId;
private:
  pthread_mutex_t& _context_ticketbooth;
  pthread_cond_t threadCond;
  bool toRelease;
    
public:
  static const int8_t MIGRATION_MODE = 2;
  static const int8_t WRITE_MODE = 1;
  static const int8_t COMMIT_MODE = -1;
  static const int8_t READ_MODE = 0;
  static const int8_t RELEASE_WRITE_MODE = -2;
  static const int8_t RELEASE_READ_MODE = -3;
  static const int8_t MIGRATION_RELEASE_MODE = -4;
  static const int8_t NONE_MODE = -5;

public:

    ContextLock( ContextBaseClass& ctx, mace::OrderID const& eventId, bool toRelease = true, int8_t requestedMode = WRITE_MODE ): context(ctx), requestedMode( requestedMode), 
        myEventId(eventId), _context_ticketbooth(context._context_ticketbooth ), toRelease(toRelease) {
        ADD_SELECTORS("ContextLock::(constructor)");
        ScopedLock sl(_context_ticketbooth);

        priorMode = NONE_MODE;
        //macedbg(1) << "[" << context.contextName<<"] STARTING.  priorMode " << (int16_t)priorMode << " requestedMode " << (int16_t)requestedMode << " executeEventTicket " << executeEventTicket << Log::endl;
        // do what's needed
        if (requestedMode == NONE_MODE) {
          nullTicketNoLock(ctx);
        } else if( requestedMode == COMMIT_MODE ) { // commit context
          ASSERT(false);
          uint64_t executeTicket = context.getExecuteEventTicket(myEventId);
          if( executeTicket > 0 ) { // sync event still helds this context
            ASSERT( executeTicket == context.now_serving_execute_ticket);
            context.now_serving_execute_ticket ++;
            macedbg(1) << "context("<< context.contextName<<") now_serving_execute_ticket = " << context.now_serving_execute_ticket << Log::endl;
            downgradeToNone(WRITE_MODE);
          }
        } else if( requestedMode == RELEASE_WRITE_MODE ) {
          downgradeToNone(WRITE_MODE);
        } else if( requestedMode == RELEASE_READ_MODE ) {
          downgradeToNone(READ_MODE);
        } else if( requestedMode == MIGRATION_RELEASE_MODE ) {
          uint64_t executeTicket = context.getExecuteEventTicket(myEventId);
          ASSERT(executeTicket == context.now_serving_execute_ticket);
          downgradeToNone(WRITE_MODE);
        } else { // read or write mode
          macedbg(1) << "ContextId=" << ctx.contextId << " EventId=" << myEventId << " mode = "<< (uint16_t)requestedMode << Log::endl;
          pthread_cond_init( &threadCond, NULL );
          upgradeFromNone(); 
        }
        //macedbg(1) << "[" <<context.contextName <<"] CONTINUING.  priorMode " << (int16_t)priorMode << " requestedMode " << (int16_t)requestedMode << " executeEventTicket " << executeEventTicket << Log::endl;
    }

    ~ContextLock(){ 
    }

    static void nullTicket(ContextBaseClass& ctx) {
      ScopedLock sl(ctx._context_ticketbooth);
      nullTicketNoLock(ctx);
    }

    void downgrade(int8_t newMode) {
      ScopedLock sl(_context_ticketbooth);
      downgradeNoLock( newMode );
    }
   
    static void notifyMigrationEvent( ContextBaseClass& context ){
      ADD_SELECTORS("ContextLock::notifyMigrationEvent");
      
      if ( !context.conditionVariables.empty() ){
        mace::ContextBaseClass::QueueItemType const& top =  context.conditionVariables.top();
        if(  top.first == context.now_serving_execute_ticket) {
          macedbg(1) << "[" << context.contextName<<"] Signalling CV " << top.second << " for ticket " << context.now_serving_execute_ticket << Log::endl;
          pthread_cond_broadcast(top.second); 
        }
      }
    }
  

private:
    static void notifyNext( ContextBaseClass& context ){
      ADD_SELECTORS("ContextLock::notifyNext");
        //bypassEvent(context);
        if ( !context.conditionVariables.empty() ){
          mace::ContextBaseClass::QueueItemType const& top =  context.conditionVariables.top();
          if(  top.first == context.now_serving_execute_ticket) {
            macedbg(1) << "[" << context.contextName<<"] Signalling CV " << top.second << " for ticket " << context.now_serving_execute_ticket << Log::endl;
            pthread_cond_broadcast(top.second); // only signal if this is a reader -- writers should signal on commit only.
          }else{
            //context.signalContextThreadPool();
            ASSERTMSG(context.conditionVariables.empty() || top.first > context.now_serving_execute_ticket, "conditionVariables map contains CV for ticket already served!!!");
          }
        } else {
          //context.signalContextThreadPool();
          ASSERTMSG(context.conditionVariables.empty() || context.conditionVariables.top().first > context.now_serving_execute_ticket, "conditionVariables map contains CV for ticket already served!!!");
        }
    }
    static void nullTicketNoLock(ContextBaseClass& context) {// chuangw: OK, I think.
      ADD_SELECTORS("ContextLock::nullTicket");

      // chuangw: Instead of waiting, just simply mark this event as committed.

      uint64_t executeTicket = context.getExecuteEventTicket(ThreadStructure::myEventID());
      ASSERT( executeTicket > 0 );

      notifyNext( context );

      /*
      if ( !context.commitConditionVariables.empty() && context.commitConditionVariables.top().first == context.now_committing) {
        macedbg(1)<<  "[" << context.contextName << "] Now signalling ticket number " << context.now_committing << ", CV "<< context.commitConditionVariables.top().second << " (my ticket is " << myTicketNum << " )" << Log::endl;
        pthread_cond_broadcast(context.commitConditionVariables.top().second); // only signal if this is a reader -- writers should signal on commit only.
      }
      else {
        ASSERTMSG(context.commitConditionVariables.empty() || context.commitConditionVariables.top().first > context.now_committing, "conditionVariables map contains CV for ticket already served!!!");
      }
      */

    }
    void printError(){
      ADD_SELECTORS("ContextLock::printError");
      maceerr<< "[" << context.contextName<<"] myEventId = "<< myEventId <<"\n";
      /*maceerr<< "size of uncommittedEvents: "<< context.uncommittedEvents.size() <<"\n";
      for(mace::map<uint64_t, int8_t>::iterator uceventIt = context.uncommittedEvents.begin(); uceventIt != context.uncommittedEvents.end(); uceventIt++){
          maceerr<< "uncommit event: ticket="<< uceventIt->first <<", mode=" << (int16_t)uceventIt->second  << "\n";
      }*/
      maceerr<< "uncommit event: ticket="<< context.uncommittedEvents.first <<", mode=" << (int16_t)context.uncommittedEvents.second  << "\n";

      maceerr<< "context.now_serving="<< context.now_serving_execute_ticket <<", context.now_committing="<< context.execute_now_committing_ticket<<Log::endl;
    }

    static void bypassEvent(ContextBaseClass& context){
      ADD_SELECTORS("ContextLock::bypassEvent");
      // increment now_serving counter if bypassQueue already contains that number
      /*
      mace::ContextBaseClass::BypassQueueType::iterator bypassIt =  context.bypassQueue.begin();
      while( bypassIt != context.bypassQueue.end() ){
        if( (*bypassIt).first == context.now_serving_execute_ticket ){
          context.now_serving = (*bypassIt).second+1;
          bypassIt++;
          macedbg(1)<< "[" << context.contextName<< "] increment now_serving to "<< context.now_serving <<Log::endl;
        }else{
          break;
        }
      }
      context.bypassQueue.erase( context.bypassQueue.begin(), bypassIt );
      */
    }
    static void bypassEventCommit(ContextBaseClass& context){
      ADD_SELECTORS("ContextLock::bypassEventCommit");
      mace::ContextBaseClass::BypassQueueType::iterator bypassIt =  context.commitBypassQueue.begin();
      while( bypassIt != context.commitBypassQueue.end() ){
        if( (*bypassIt).first == context.execute_now_committing_ticket ){
          context.execute_now_committing_ticket = (*bypassIt).second+1;
          bypassIt++;
          macedbg(1)<< "[" << context.contextName<< "] increment now_committing to "<< context.execute_now_committing_ticket <<Log::endl;
        }else{
          break;
        }
      }
      context.commitBypassQueue.erase( context.commitBypassQueue.begin(), bypassIt );
    }

    void upgradeFromNone(){ 
      ADD_SELECTORS("ContextLock::upgradeFromNone");
      ASSERTMSG(requestedMode == READ_MODE || requestedMode == WRITE_MODE || requestedMode == MIGRATION_MODE, "Invalid mode requested!");

      const uint64_t executeTicket = context.getExecuteEventTicket(myEventId);
      ASSERTMSG ( executeTicket > 0, "In ContextLock::upgradeFromNone: the execute ticket is invalid" );

      if( !context.checkReEnterEvent(myEventId) ) {
        ticketBoothWait(requestedMode);
        if (requestedMode == READ_MODE) {
          ASSERT(context.numWriters == 0);
        
          context.uncommittedEvents.first = executeTicket;
          context.uncommittedEvents.second = READ_MODE;
          context.numReaders ++;
          context.now_serving_eventId = myEventId;
          context.addReaderEvent(myEventId);
          if( context.numReaders > 1 ) {
            macedbg(1) << "In context("<< context.contextName <<"), numReaders=" << context.numReaders << Log::endl;
          }
          // wake up the next waiting thread (which has the next smallest ticket number)
          if ( !context.conditionVariables.empty() ){
          //bypassEvent(context);
            if(  context.conditionVariables.top().first == context.now_serving_execute_ticket) {
              //macedbg(1) << "[" << context.contextName <<"] Now signalling ticket number " << context.now_serving_execute_ticket << " (my eventId is " << myEventId << " )" << Log::endl;
              pthread_cond_broadcast(context.conditionVariables.top().second); // only signal if this is a reader -- writers should signal on commit only.
            }
          } else {
            ASSERTMSG(context.conditionVariables.empty() || context.conditionVariables.top().first > context.now_serving_execute_ticket, "conditionVariables map contains CV for ticket already served!!!");
          }
        } else if (requestedMode == WRITE_MODE) {
          //Acquire write lock
          ASSERT(context.numReaders == 0);
          ASSERT(context.numWriters == 0);
          
          context.numWriters = 1;
          context.lastWrite = executeTicket;
          context.execute_serving_flag = true;
          context.now_serving_eventId = myEventId;
          context.addWriterEvent(myEventId);
        } else if (requestedMode == MIGRATION_MODE) {
          ASSERT(context.numReaders == 0);
          ASSERT(context.numWriters == 0);
          ASSERT(context.execute_now_committing_ticket == executeTicket);
          context.numWriters = 1;
          context.addWriterEvent(myEventId);
          context.lastWrite = executeTicket;
          context.execute_serving_flag = true;
          context.now_serving_eventId = myEventId;
        }
        macedbg(2) << "After Event("<< myEventId <<") context("<< context.contextName<<") numReaders = " << context.numReaders << " numWriters = " << context.numWriters << Log::endl;
      } else {
        macedbg(1) << "Event("<< myEventId<<") re-enter " << context.contextName << Log::end;
      }
    }

    /* When events are enqueued execute queue, they are assigned an execute ticket and assign to execute. And they could really execute when now_serving_execute_ticket is equal to their
     * assigned executed ticket
     */
    void ticketBoothWait(int8_t requestedMode){
      ADD_SELECTORS("ContextLock::ticketBoothWait");

      uint64_t executeTicket = context.getExecuteEventTicket(myEventId);
      ASSERT( executeTicket > 0 );
      if ( executeTicket > context.now_serving_execute_ticket ||
          ( requestedMode == READ_MODE && (context.numWriters != 0) ) ||
          ( requestedMode == WRITE_MODE && (context.numReaders != 0 || context.numWriters != 0) ) ||
          ( requestedMode == MIGRATION_MODE && context.execute_now_committing_ticket != executeTicket )
         ) {
        macedbg(2)<< "[" << context.contextName << "] Storing condition variable " << &threadCond << " at eventId " <<  executeTicket << Log::endl;
        context.conditionVariables.push( std::pair<uint64_t, pthread_cond_t*>( executeTicket, &threadCond ) );
      }


      while ( executeTicket > context.now_serving_execute_ticket ||
          ( requestedMode == READ_MODE && (context.numWriters != 0) ) ||
          ( requestedMode == WRITE_MODE && (context.numReaders != 0 || context.numWriters != 0) ) ||
          ( requestedMode == MIGRATION_MODE && context.execute_now_committing_ticket != executeTicket )
          )    {

        macedbg(2)<< "[" << context.contextName << "] Waiting for my turn on cv " << &threadCond << ".  myEventId " << myEventId << " wait until ticket " << executeTicket 
          << ", now_serving " << context.now_serving_execute_ticket << " requestedMode " << (int16_t)requestedMode << " numWriters " << context.numWriters << " numReaders " 
          << context.numReaders << Log::endl;
        pthread_cond_wait(&threadCond, &_context_ticketbooth);
      }

      macedbg(2) << "[" << context.contextName<< "] event " << myEventId << " being served! executeTicket = "<< executeTicket << Log::endl;

      //If we added our cv to the map, it should be the front, since all earlier tickets have been served.
      if (!context.conditionVariables.empty() && context.conditionVariables.top().first ==  executeTicket) {
        macedbg(2) << "[" << context.contextName<<"] Erasing our cv from the map." << Log::endl;
        context.conditionVariables.pop();
      }
      else if (!context.conditionVariables.empty()) {
        macedbg(2) << "[" << context.contextName<<"] FYI, first cv in map is for ticket " << context.conditionVariables.top().first << Log::endl;
      }

      ASSERT(executeTicket <= context.now_serving_execute_ticket); //Remove once working.

      if( requestedMode == READ_MODE) {
        if( executeTicket == context.now_serving_execute_ticket) {
          context.now_serving_execute_ticket ++;
          macedbg(1) << "Event("<< myEventId <<") context("<< context.contextName<<") now_serving_execute_ticket = " << context.now_serving_execute_ticket << Log::endl;
          context.enqueueReadyExecuteEventQueueWithLock();
        }
      }
      macedbg(1) << "Context("<< context.contextName << ") now_serving_execute_ticket = " << context.now_serving_execute_ticket << Log::endl;

    }
    void downgradeNoLock(int8_t newMode) {
      ADD_SELECTORS("ContextLock::downgrade");
      /*
      const mace::OrderID& myEventId = ThreadStructure::myEventID();
      uint64_t ticket = 0;

      ASSERTMSG( myTicketNum == context.uncommittedEvents.first, "ticket number not found in uncommittedEvent");
      //uint8_t runningMode = ticketIt->second;
      uint8_t runningMode = context.uncommittedEvents.second;
      macedbg(1) << "[" << context.contextName<<"] Downgrade requested. myTicketNum " << myTicketNum << " runningMode " << (int16_t)runningMode << " newMode " << (int16_t)newMode << Log::endl;

      if( newMode == NONE_MODE ){ // remove from uncommited event list.
        //context.uncommittedEvents.erase( ticketIt );
        context.uncommittedEvents.first = 0;
        context.uncommittedEvents.second = -1;
      }

      if (newMode == NONE_MODE && runningMode != NONE_MODE) 
        downgradeToNone( runningMode );
      else if (newMode == READ_MODE && runningMode == WRITE_MODE) 
        downgradeToRead();
      else 
        macewarn << "[" << context.contextName<<"] Why was downgrade called?  Current mode is: " << (int16_t)runningMode << " and mode requested is: " << (int16_t)newMode << Log::endl;
      macedbg(1) << "[" << context.contextName<<"] Downgrade exiting" << Log::endl;
      */
    }
    void downgradeToNone(int8_t runningMode) {
        ADD_SELECTORS("ContextLock::downgradeToNone");
        macedbg(2) << "Before Event("<< myEventId <<") context("<< context.contextName<<") numReaders = " << context.numReaders << " numWriters = " << context.numWriters << Log::endl;
      
        //bool doGlobalRelease = false;
        if (runningMode == READ_MODE) {
          ASSERT(context.numReaders > 0);
          context.numReaders --;
          context.removeReaderEvent(myEventId);
          // context.enqueueReadyExecuteEventQueueWithLock();
        } else if (runningMode == WRITE_MODE) {
          ASSERT(context.numReaders == 0 && context.numWriters == 1);
          context.numWriters=0;
          uint64_t executeTicket = context.getExecuteEventTicket(myEventId);
          if( executeTicket != context.now_serving_execute_ticket ) {
            maceerr << "In context("<< context.contextName <<") event("<< myEventId <<") executeTicket=" << executeTicket << " now_serving_execute_ticket=" << context.now_serving_execute_ticket <<Log::endl; 
            ASSERT(false);
          }
          context.now_serving_execute_ticket ++;
          context.removeWriterEvent(myEventId);
          macedbg(1) << "Event ("<< myEventId <<")'s ticket("<< context.getExecuteEventTicket(myEventId) <<") context("<< context.contextName<<") now_serving_execute_ticket = " << context.now_serving_execute_ticket << Log::endl;
          context.enqueueReadyExecuteEventQueueWithLock();
        } else {
          ABORT("Invalid running mode!");
        }
        macedbg(2) << "After Event("<< myEventId <<") context("<< context.contextName<<") numReaders = " << context.numReaders << " numWriters = " << context.numWriters << Log::endl;
        notifyNext( context );
        //commitOrderWait();
    }
    void downgradeToRead() {
      ADD_SELECTORS("ContextLock::downgradeToRead");
        macedbg(1) << "[" << context.contextName<<"] Downgrade to READ_MODE reqested" << Log::endl;
        uint64_t executeTicket = context.getExecuteEventTicket(myEventId);

        ASSERT(context.numWriters == 1 && context.numReaders == 0);
        ASSERT(context.now_serving_execute_ticket == executeTicket + 1); // We were in exclusive mode, and holding the lock, so we should still be the one being served...
        // Delay committing until end.
        context.numWriters = 0;
        //contextThreadSpecific->setSnapshotVersion(context.lastWrite);
        context.snapshot( context.lastWrite );

        //context.uncommittedEvents[ myTicketNum ] = READ_MODE;
        context.uncommittedEvents.first = executeTicket;
        context.uncommittedEvents.second = READ_MODE;

        bypassEvent(context);
        if (!context.conditionVariables.empty() && context.conditionVariables.top().first == context.now_serving_execute_ticket) {
          macedbg(1) << "[" << context.contextName<<"] Signalling CV " << context.conditionVariables.top().second << " for ticket " << context.now_serving_execute_ticket << Log::endl;
          pthread_cond_broadcast(context.conditionVariables.top().second); // only signal if this is a reader -- writers should signal on commit only.
        }
        else {
          ASSERTMSG(context.conditionVariables.empty() || context.conditionVariables.top().first > context.now_serving_execute_ticket, "conditionVariables map contains CV for ticket already served!!!");
        }
    }
    void commitOrderWait() {
      ADD_SELECTORS("ContextLock::commitOrderWait");
      uint64_t executeTicket = context.getExecuteEventTicket(myEventId);
      ASSERT( executeTicket > 0 );
      
      if ( executeTicket > context.execute_now_committing_ticket ) {
        macedbg(1)<< "[" << context.contextName << "] Storing condition variable " << &threadCond << " at ticket " <<  executeTicket << Log::endl;
        context.commitConditionVariables.push( std::pair< uint64_t, pthread_cond_t*>( executeTicket, &threadCond ) );
      }
      while ( executeTicket > context.execute_now_committing_ticket) {
        macedbg(1)<< "[" <<  context.contextName << "] Waiting for my turn on cv " << &threadCond << ".  myEventId " << myEventId << " wait until ticket " << executeTicket << ", now_committing " << context.execute_now_committing_ticket << Log::endl;

        pthread_cond_wait(&(threadCond), &_context_ticketbooth);
      }

      macedbg(1) << "[" <<  context.contextName<<"] Ticket " << executeTicket << " being committed at context '" <<context.contextName << "'! executeTicket = "<< executeTicket << Log::endl;

      //If we added our cv to the map, it should be the front, since all earlier tickets have been served.
      if ( !context.commitConditionVariables.empty() && context.commitConditionVariables.top().first ==  executeTicket) {
        macedbg(1)<<  "[" << context.contextName << "] Erasing our cv from the map." << Log::endl;
        context.commitConditionVariables.pop();
      }
      else if ( !context.commitConditionVariables.empty()) {
        macedbg(1)<<  "[" << context.contextName << "] FYI, first cv in map is for ticket " << context.commitConditionVariables.top().first << Log::endl;
      }
      ASSERT(executeTicket == context.execute_now_committing_ticket); //Remove once working.

      context.execute_now_committing_ticket = executeTicket+1;

      if ( !context.commitConditionVariables.empty() && context.commitConditionVariables.top().first == context.execute_now_committing_ticket) {
        macedbg(1)<<  "[" << context.contextName << "] Now signalling ticket number " << context.execute_now_committing_ticket << ", CV "<< context.commitConditionVariables.top().second << " (my ticket is " << executeTicket << " )" << Log::endl;
        pthread_cond_broadcast(context.commitConditionVariables.top().second); // only signal if this is a reader -- writers should signal on commit only.
      }
      else {
        ASSERTMSG(context.commitConditionVariables.empty() || context.commitConditionVariables.top().first > context.execute_now_committing_ticket, "conditionVariables map contains CV for ticket already served!!!");
      }
    }
};

}

#endif
