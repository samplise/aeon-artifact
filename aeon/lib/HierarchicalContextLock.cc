#include "HierarchicalContextLock.h"
/*pthread_mutex_t mace::DeferredMessages::msgmutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t mace::DeferredMessages::eventCond = PTHREAD_COND_INITIALIZER;;
mace::DeferredMessages::DeferredEventMessageType mace::DeferredMessages::deferredMessages;*/


std::map<uint64_t, pthread_cond_t* >  mace::HierarchicalContextLock::enteringEvents;
std::map<mace::OrderID, pthread_cond_t*> mace::HierarchicalContextLock::commitConditionVariables;
mace::OrderID mace::HierarchicalContextLock::now_serving;
mace::OrderID mace::HierarchicalContextLock::now_committing;
uint32_t mace::HierarchicalContextLock::noLeafContexts = 0;
mace::map<mace::OrderID, mace::set<mace::string> > mace::HierarchicalContextLock::eventSnapshotContextIDs;
pthread_mutex_t mace::HierarchicalContextLock::ticketbooth = PTHREAD_MUTEX_INITIALIZER;
mace::map<mace::OrderID, mace::string> mace::HierarchicalContextLock::eventsQueue;
bool mace::HierarchicalContextLock::endEventCommitted = false;

