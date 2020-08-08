/* 
 * mace.cc : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, Charles Killian, Dejan Kostic, Ryan Braud, James W. Anderson, John Fisher-Ogden, Calvin Hubble, Duy Nguyen, Justin Burke, David Oppenheimer, Amin Vahdat, Adolfo Rodriguez, Sooraj Bhat
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the names of the contributors, nor their associated universities 
 *      or organizations may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * ----END-OF-LEGAL-STUFF---- */
#include <errno.h>
#include "mace.h"
#include "ScopedStackExecution.h"
#include "Event.h"
#include "AsyncDispatch.h"
#include "HeadEventDispatch.h"
#include "HierarchicalContextLock.h"
#include "ContextLock.h"
#include "NumberGen.h"

int32_t __eventContextType = 0;    // used in simulator to determine the context type of the event.

static pthread_mutex_t getRecursiveMutex() {
  pthread_mutex_t mutex;
  pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);
	pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&mutex, &mutexattr); 
	return mutex;
}

pthread_mutex_t BaseMaceService::synclock = getRecursiveMutex();
bool BaseMaceService::_printLower = false;

std::deque<BaseMaceService*> BaseMaceService::instances;
uint64_t BaseMaceService::lastSnapshot = 0;
uint64_t BaseMaceService::lastSnapshotReleased = 0;

std::vector<BaseMaceService*> BaseMaceService::instanceID;

void BaseMaceService::registerInstanceID(){
  instanceUniqueID = static_cast<uint8_t>(NumberGen::Instance(NumberGen::SERVICE_INSTANCE_UID)->GetVal());
  ASSERT( instanceUniqueID == instanceID.size() );
  instanceID.push_back( this );
}
BaseMaceService* BaseMaceService::getInstance( const uint8_t sid ){
  ASSERT( sid < instanceID.size() );
  return instanceID[ static_cast<std::vector<BaseMaceService*>::size_type>(sid) ];
}

//chuangw: Obsoleted....
void BaseMaceService::globalSnapshot(const uint64_t& ver) {
  ADD_SELECTORS("BaseMaceService::globalSnapshot");
  macedbg(1) << "Global Snapshot Requested for Version " << ver << " lastSnapshot " << lastSnapshot << " lastSnapshotReleased " << lastSnapshotReleased << Log::endl;
  if (ver > lastSnapshot) {
    macedbg(1) << "Performing global snapshot." << Log::endl;
    for (std::deque<BaseMaceService*>::const_iterator i = instances.begin(); i != instances.end(); i++) {
      (*i)->snapshot(ver);
    }
    lastSnapshot = ver;
  }
  else {
    // Possibly log here?
  }
}
//chuangw: Obsoleted....
void BaseMaceService::globalSnapshotRelease(const uint64_t& ver) {
  ADD_SELECTORS("BaseMaceService::globalSnapshotRelease");
  macedbg(1) << "Global Snapshot Release Requested for Version " << ver << " lastSnapshot " << lastSnapshot << " lastSnapshotReleased " << lastSnapshotReleased << Log::endl;
  if (lastSnapshot >= lastSnapshotReleased && lastSnapshotReleased < ver) {
    macedbg(1) << "Performing global snapshot release." << Log::endl;
    for (std::deque<BaseMaceService*>::const_iterator i = instances.begin(); i != instances.end(); i++) {
      (*i)->snapshotRelease(ver);
    }
    lastSnapshotReleased = ver;
  }
}
void BaseMaceService::globalNotifyNewContext( mace::Event & event, const uint8_t serviceID ) {
  ADD_SELECTORS("BaseMaceService::globalNewContext");
  macedbg(1) << "Globally propogate new context event to all contexts" << Log::endl;
  // When a service decides to create a new context, it should also tell other services in the composition to create snapshot. To other services, since their context doesn't change, the snapshot is the same as before

  for (std::deque<BaseMaceService*>::const_iterator i = instances.begin(); i != instances.end(); i++) {
    (*i)->notifyNewContext( event, serviceID );
  }
}

void BaseMaceService::globalNotifyNewEvent(  mace::Event & event, const uint8_t serviceID ) {
  ADD_SELECTORS("BaseMaceService::globalNewEvent");
  macedbg(1) << "An event was created in service "<< (uint16_t)serviceID << Log::endl;

  for (std::deque<BaseMaceService*>::const_iterator i = instances.begin(); i != instances.end(); i++) {
    (*i)->notifyNewEvent( event, serviceID );
  }
}

void BaseMaceService::globalDowngradeEventContext( ) {
  ADD_SELECTORS("BaseMaceService::globalDowngradeEventContext");
  macedbg(1) << "The event "<< ThreadStructure::myEvent().eventId <<" is downgrading all contexts " << Log::endl;

  for (std::deque<BaseMaceService*>::const_iterator i = instances.begin(); i != instances.end(); i++) {
    (*i)->downgradeEventContext( );
  }
}
#include "ContextBaseClass.h"
// chuangw: TODO: check if the downgrade is valid: i.e. the current context is the top-most possessed context.
void BaseMaceService::downgradeCurrentContext() const{
  ADD_SELECTORS("BaseMaceService::downgradeCurrentContext");
  // Simpler and presumably more efficient than the more general downgradeContext()
  mace::string snapshot;
  //mace::serialize( snapshot, ThreadStructure::myContext() );
  ThreadStructure::insertSnapshotContext( ThreadStructure::getCurrentContext(), snapshot );
  if( ThreadStructure::getCurrentContext() != ThreadStructure::myContext()->getID() ){
    maceerr<<"ThreadStructure::getCurrentContext() = "<< ThreadStructure::getCurrentContext()
           <<"ThreadStructure::myContext()->contextID = "<< ThreadStructure::myContext()->contextId<<Log::endl;
    ABORT("The current context id doesn't match the id of the current context object");
  }
  mace::ContextLock cl( *(ThreadStructure::myContext()), ThreadStructure::myEventID(), true, mace::ContextLock::NONE_MODE );
  ThreadStructure::removeEventContext( ThreadStructure::getCurrentContext() );
          
}


mace::AgentLock::RPCWaitType mace::AgentLock::rpcWaitingEvents;
pthread_mutex_t mace::AgentLock::_agent_ticketbooth = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mace::AgentLock::_agent_commitbooth = PTHREAD_MUTEX_INITIALIZER;
uint64_t mace::AgentLock::now_serving = 1; // First ticket has number 1.
//uint64_t mace::AgentLock::lastWrite = 1; // First ticket has number 1.
//int mace::AgentLock::numReaders = 0;
int mace::AgentLock::numWriters = 0;
mace::AgentLock::CondQueue mace::AgentLock::conditionVariables;



mace::OrderID mace::AgentLock::now_committing; // First ticket has number 1.
mace::AgentLock::CondQueue mace::AgentLock::commitConditionVariables;



mace::AgentLock::BypassTicketType mace::AgentLock::bypassTickets;
mace::AgentLock::BypassTicketType mace::AgentLock::bypassCommits;


pthread_key_t mace::AgentLock::ThreadSpecific::pkey;
pthread_once_t mace::AgentLock::ThreadSpecific::keyOnce = PTHREAD_ONCE_INIT;

mace::AgentLock::ThreadSpecific::ThreadSpecific() : /*currentMode(mace::AgentLock::NONE_MODE),*/ myEventId(), 
  snapshotVersion(0)
{
  pthread_cond_init(&threadCond, 0);
} // ThreadSpecific

mace::AgentLock::ThreadSpecific::~ThreadSpecific() {
  pthread_cond_destroy(&threadCond);
} // ~ThreadSpecific

mace::AgentLock::ThreadSpecific* mace::AgentLock::ThreadSpecific::init() {
  pthread_once(&keyOnce, mace::AgentLock::ThreadSpecific::initKey);
  ThreadSpecific* t = (ThreadSpecific*)pthread_getspecific(pkey);
  if (t == 0) {
    t = new ThreadSpecific();
    assert(pthread_setspecific(pkey, t) == 0);
  }

  return t;
} // init

void mace::AgentLock::ThreadSpecific::initKey() {
  assert(pthread_key_create(&pkey, NULL) == 0);
} // initKey

void mace::AgentLock::ThreadSpecific::releaseThreadSpecificMemory(){
  pthread_once(&keyOnce, initKey);
  ThreadSpecific* t = (ThreadSpecific*)pthread_getspecific(pkey);
  if (t != 0) {
    delete t;
  }
}

#include "HeadEventDispatch.h"
namespace HeadEventDispatch{
  extern pthread_mutex_t eventQueueMutex;
}
pthread_mutex_t mace::AgentLockNB::_agent_ticketbooth = PTHREAD_MUTEX_INITIALIZER;
uint64_t mace::AgentLockNB::now_serving = 1; // First ticket has number 1.
mace::OrderID mace::AgentLockNB::lastWrite; // First ticket has number 1.
int mace::AgentLockNB::numReaders = 0;
int mace::AgentLockNB::numWriters = 0;
mace::AgentLockNB::CondQueue mace::AgentLockNB::conditionVariables;



mace::OrderID mace::AgentLockNB::now_committing; // First ticket has number 1.
mace::AgentLockNB::CondQueue mace::AgentLockNB::commitConditionVariables;

pthread_mutex_t mace::AgentLockNB::ticketMutex = PTHREAD_MUTEX_INITIALIZER;
uint64_t mace::AgentLockNB::nextTicketNumber = 1;

pthread_key_t mace::AgentLockNB::ThreadSpecific::pkey;
pthread_once_t mace::AgentLockNB::ThreadSpecific::keyOnce = PTHREAD_ONCE_INIT;

mace::AgentLockNB::ThreadSpecific::ThreadSpecific() : currentMode(-1), myEventId(), 
  snapshotVersion(0)
{
  pthread_cond_init(&threadCond, 0);
} // ThreadSpecific

mace::AgentLockNB::ThreadSpecific::~ThreadSpecific() {
  pthread_cond_destroy(&threadCond);
} // ~ThreadSpecific

mace::AgentLockNB::ThreadSpecific* mace::AgentLockNB::ThreadSpecific::init() {
  pthread_once(&keyOnce, mace::AgentLockNB::ThreadSpecific::initKey);
  ThreadSpecific* t = (ThreadSpecific*)pthread_getspecific(pkey);
  if (t == 0) {
    t = new ThreadSpecific();
    assert(pthread_setspecific(pkey, t) == 0);
  }

  return t;
} // init

void mace::AgentLockNB::ThreadSpecific::initKey() {
  assert(pthread_key_create(&pkey, NULL) == 0);
} // initKey

void mace::AgentLockNB::ThreadSpecific::releaseThreadSpecificMemory(){
  pthread_once(&keyOnce, initKey);
  ThreadSpecific* t = (ThreadSpecific*)pthread_getspecific(pkey);
  if (t != 0) {
    delete t;
  }
}

/*
std::set<mace::commit_executor*> mace::registered;
std::set<mace::CommitWrapper*> mace::registered_class;
*/
void mace::Init(int argc, char** argv) {
  params::loadparams(argc, argv);
  Init();
}

void mace::Init() {
  Log::configure(); //This should probably be first

  ADD_SELECTORS("mace::Init");
  macedbg(1) << "Enter mace::Init" << Log::endl;
  AsyncDispatch::init();
  HeadEventDispatch::init();
}

void mace::Shutdown() {
  AsyncDispatch::haltAndWait();

  HeadEventDispatch::haltAndWait();
  HeadEventDispatch::haltAndWaitCommit();
  mace::AgentLock::cleanup();

  Scheduler::haltScheduler(); //Keep this last!
}
