#ifndef _MACE_GLOBAL_COMMIT_H
#define _MACE_GLOBAL_COMMIT_H

#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>       
#include "mset.h"
#include "mdeque.h"
#include "m_map.h"
#include "CommitWrapper.h"
#include "Accumulator.h"
#include "ThreadStructure.h"
namespace mace {

/*
  extern static set<commit_executor*> registered;
  extern static set<CommitWrapper*> registered_class;
  */
/**
 * \brief Deprecated! this class is no longer used
 * */
class GlobalCommit {
    private:
        static uint64_t now_commit;
        static map<uint64_t, pthread_cond_t*> conditionVariables;
        //static deque<uint64_t> ticketQueue;
        //std::multimap<uint64_t, int, void *> eventQueue;
        //static std::multimap<uint64_t, void(*)(uint64_t)> onCommitFuncs;
        // ticket num, event type, data structure
  
        static std::set<commit_executor*> registered;
        static std::set<CommitWrapper*> registered_class;


    public:
        static void registerForCommit(commit_executor* ptr) {
          ABORT("DEFUNCT");
            registered.insert(ptr);
        }
        static void registerCommitExecutor(CommitWrapper* commit_executor) {
          ABORT("DEFUNCT");
            registered_class.insert(commit_executor);
        }

        static void executeCommit(uint64_t myTicketNum)
        {
          ABORT("DEFUNCT");
            std::set<CommitWrapper*>::iterator i;
            for (i = registered_class.begin(); i != registered_class.end(); i++) {
                (*i)->commitCallBack(myTicketNum);
            }
            std::set<commit_executor*>::iterator regIter = registered.begin();
            while (regIter != registered.end()) {
                commit_executor* fnt = *regIter;
                (*fnt)(myTicketNum);
                regIter++;
            }
        }

        static void commit( mace::Event & event){
          static uint32_t eventCommitIncrement = params::get("EVENT_COMMIT_INCREMENT", 1);
          if( event.eventId.ticket %eventCommitIncrement ==0){
            Accumulator::Instance(Accumulator::EVENT_COMMIT_COUNT)->accumulate(eventCommitIncrement); // increment committed event number
          }
          event.commit();
        }
        static void commitNoop( ){
            Accumulator::Instance(Accumulator::EVENT_COMMIT_COUNT)->accumulate(1); // increment committed event number
        }
        static void commit(){
          static uint32_t eventCommitIncrement = params::get("EVENT_COMMIT_INCREMENT", 1);
          Event& myEvent = ThreadStructure::myEvent();
          Accumulator::Instance(Accumulator::EVENT_COMMIT_COUNT)->accumulate(1); // increment committed event number

          if( myEvent.eventId.ticket %eventCommitIncrement ==0){
            Accumulator::Instance(Accumulator::EVENT_COMMIT_COUNT)->accumulate(eventCommitIncrement); // increment committed event number
          }
          myEvent.commit();
        }
 
        // issue a new commit order - XXX forgot what for
        /*static void nextTicket(uint64_t myTicketNum) {
            // add it to commit queue - only writers will added here
            ticketQueue.push_back(myTicketNum);
        }*/

};

}
#endif
