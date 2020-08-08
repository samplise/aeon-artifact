#ifndef _HIERARCHICALCONTEXTLOCK_H
#define _HIERARCHICALCONTEXTLOCK_H

// including headers
//#include "mace.h"
// uses snapshot by default
#include "Event.h"
#include "ScopedLock.h"
#include <pthread.h>
#include "MaceKey.h"
#include "ContextMapping.h"
//#include "GlobalCommit.h"
#include "Accumulator.h"
#include "mace.h"
#include "ContextLock.h"
#include "Message.h"

namespace mace{

/**
 * \brief this class is obsolete. it's a wrong design and should not be used any more.
 *
 * */
class HierarchicalContextLock{
  private:
public:
    HierarchicalContextLock(Event& event, mace::string msg) {
        ADD_SELECTORS("HierarchicalContextLock::(constructor)");
        mace::OrderID myEventId = event.getEventID();
        macedbg(1) << "Event " << myEventId << " being served!" << Log::endl;
        
    }
    /*
    static uint64_t getUncommittedEvents(){
      return ( now_serving - now_committing );
    }
    */
    static void commit(){
      ABORT("DEFUNCT");
        ADD_SELECTORS("HierarchicalContextLock::commit");
        const mace::OrderID& myEventId = ThreadStructure::myEventID();
        
        Accumulator::Instance(Accumulator::EVENT_COMMIT_COUNT)->accumulate(1); // increment committed event number

        if( myEventId == mace::Event::exitEventId ){
          endEventCommitted = true;
        }

        if( ThreadStructure::myEvent().eventType == mace::Event::HEADMIGRATIONEVENT ){
          // TODO: After HEADMIGRATION event is committed, this head node is not needed anymore. Terminate.
        }

        //c_lock.downgrade( mace::ContextLock::NONE_MODE );
    }
    /*
    static uint64_t nextCommitting(){
      return now_committing;
    }
    */
private:
    
    static std::map<uint64_t, pthread_cond_t* >  enteringEvents;
    static mace::OrderID now_serving;
    static mace::OrderID now_committing;
    static uint32_t noLeafContexts;
    static mace::map<mace::OrderID, mace::set<mace::string> > eventSnapshotContextIDs;
    static std::map<mace::OrderID, pthread_cond_t*> commitConditionVariables; // Support for per-thread CVs, which gives per ticket CV support. Note: can just use the front of the queue to avoid lookups 
    static pthread_mutex_t ticketbooth;

    static mace::map<mace::OrderID, mace::string> eventsQueue;
    static mace::map<mace::OrderID, uint16_t> committingQueue;
    static mace::OrderID expectedCommiteEvent;


public:
    static bool endEventCommitted; // is true if the ENDEVENT commits
};

}
#endif
