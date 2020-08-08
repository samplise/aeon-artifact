#include "ThreadStructure.h"
#include "ContextBaseClass.h"
#include "ContextMapping.h"

void printThreadStructure(){
  ADD_SELECTORS ("printThreadStructure");

  maceout<< ThreadStructure::myEvent() << Log::endl;
  //std::cout<< ThreadStructure::myEvent() << std::endl;
}

uint64_t ThreadStructure::newMessageTicket() {
  ADD_SELECTORS("ThreadStructure::newMessageTicket");
  ScopedLock sl(ticketMutex);
  ThreadSpecific* t = ThreadSpecific::init();
  t->setMessageTicket(nextMessageTicketNumber);
  macedbg(1) << "External Message Ticket " << nextMessageTicketNumber << " is sold!" << Log::endl;
  return nextMessageTicketNumber ++;
}

mace::OrderID ThreadStructure::newTicket(mace::ContextBaseClass* contextObj) {
  ADD_SELECTORS("ThreadStructure::newTicket");
  ASSERTMSG(contextObj != NULL, "It's a null context object!");
  mace::OrderID eventId;
  ScopedLock sl(contextObj->createEventTicketMutex);
  eventId.ctxId = contextObj->contextId;
  eventId.ticket = contextObj->createTicketNumber ++;
    
  ThreadSpecific *t = ThreadSpecific::init();
  t->setEventID(eventId);
  macedbg(1) << "create eventId " << eventId << " sold!" << Log::endl;
  return eventId;
}

mace::OrderID ThreadStructure::newTickets( mace::ContextBaseClass* contextObj, const uint32_t nTickets ){
  mace::OrderID id;
  if( contextObj == NULL ) return id;
  ADD_SELECTORS("ThreadStructure::newTicket");
  ASSERT( nTickets > 0 );
            
  ScopedLock sl(contextObj->createEventTicketMutex);

  macedbg(1) << "Tickets " << contextObj->createTicketNumber << " to " << contextObj->createTicketNumber+nTickets-1 <<  " sold!" << Log::endl;
  uint64_t firstTicket = contextObj->createTicketNumber;
  contextObj->createTicketNumber += nTickets;
  id.ctxId = contextObj->contextId;
  id.ticket = firstTicket;
  return id;
}

pthread_key_t ThreadStructure::ThreadSpecific::pkey;
pthread_once_t ThreadStructure::ThreadSpecific::keyOnce = PTHREAD_ONCE_INIT;
uint64_t ThreadStructure::nextTicketNumber = 1;
uint64_t ThreadStructure::nextMessageTicketNumber = 1;
uint64_t ThreadStructure::current_valid_ticket = 1;
pthread_mutex_t ThreadStructure::ticketMutex = PTHREAD_MUTEX_INITIALIZER;

ThreadStructure::ThreadSpecific::ThreadSpecific() :
  event( ),
  ticketIsServed( true ),
  thisContext( NULL ),
  contextStack( ),
  serviceStack( )
{

} // ThreadSpecific

ThreadStructure::ThreadSpecific::~ThreadSpecific() {

} // ~ThreadSpecific

ThreadStructure::ThreadSpecific* ThreadStructure::ThreadSpecific::init() {
		pthread_once(&keyOnce, ThreadStructure::ThreadSpecific::initKey);
  	ThreadSpecific* t = (ThreadSpecific*)pthread_getspecific(pkey);
  	if (t == 0) {
    		t = new ThreadSpecific();
    		assert(pthread_setspecific(pkey, t) == 0);
  	}
  	return t;
} // init

void ThreadStructure::ThreadSpecific::releaseThreadSpecificMemory(){
  pthread_once(&keyOnce, initKey);
  ThreadSpecific* t = (ThreadSpecific*)pthread_getspecific(pkey);
  if (t != 0) {
    delete t;
  }
}

void ThreadStructure::ThreadSpecific::initKey() {
		assert(pthread_key_create(&pkey, NULL) == 0);
} // initKey

const mace::OrderID& ThreadStructure::ThreadSpecific::myEventID() const {
  	return this->event.eventId;
} 

mace::Event& ThreadStructure::ThreadSpecific::myEvent() {
  	return this->event;
}  

/* make a copy of event object. store it in the thread-specific heap.*/
void ThreadStructure::ThreadSpecific::setEvent(const mace::Event& _event) {
  event = _event; 
}

/* a fast way to set the current event ID*/
void ThreadStructure::ThreadSpecific::setEventID(const mace::OrderID& eventId) {
  event.eventId = eventId; 
}

void ThreadStructure::ThreadSpecific::setMessageTicket( const uint64_t messageTicket ) {
  event.externalMessageTicket = messageTicket;
}

const uint64_t ThreadStructure::ThreadSpecific::getEventContextMappingVersion() const {
  	return this->event.eventContextMappingVersion;
} 

void ThreadStructure::ThreadSpecific::setEventContextMappingVersion( const uint64_t ver )  {
  	this->event.eventContextMappingVersion = ver;
} 

void ThreadStructure::ThreadSpecific::popContext(){
    ASSERT( !contextStack.empty() );
    contextStack.pop_back();
}

void ThreadStructure::ThreadSpecific::pushContext(const uint32_t contextID){
    contextStack.push_back( contextID );
}

const uint32_t ThreadStructure::ThreadSpecific::getCurrentContext() const{
    ASSERT( !contextStack.empty() );
    return contextStack.back();
}

const mace::Event::EventContextType& ThreadStructure::ThreadSpecific::getEventContexts()const {
    return  event.eventContexts;
}
const mace::Event::EventServiceContextType & ThreadStructure::ThreadSpecific::getCurrentServiceEventContexts() {
    return  event.eventContexts[ getServiceInstance() ];
}
const mace::Event::EventSnapshotContextType & ThreadStructure::ThreadSpecific::getEventSnapshotContexts() {
    return  event.eventSnapshotContexts;
}
const mace::Event::EventServiceSnapshotContextType & ThreadStructure::ThreadSpecific::getCurrentServiceEventSnapshotContexts() {
    return  event.eventSnapshotContexts[ getServiceInstance() ];
}
/*
const mace::vector<mace::PreEventInfo> ThreadStructure::ThreadSpecific::getPreEventInfos(const uint8_t serviceID, const mace::string& ctx_name) const {
    return  event.getPreEventInfos( serviceID, ctx_name );
}
*/
const bool ThreadStructure::ThreadSpecific::isEventEnteredService(const uint8_t serviceID) const {
    return  (event.eventContexts.find( serviceID ) != event.eventContexts.end() );
}
const bool ThreadStructure::ThreadSpecific::insertEventContext(const uint32_t contextID){
    uint8_t serviceUID = getServiceInstance();
    std::pair< mace::Event::EventServiceContextType::iterator, bool> result = event.eventContexts[serviceUID].insert(contextID);
    return result.second;
}
const bool ThreadStructure::ThreadSpecific::removeEventContext(const uint32_t contextID){
    uint8_t serviceUID = getServiceInstance();
    const mace::set< uint32_t >::size_type removedContexts = event.eventContexts[serviceUID].erase(contextID);
    ASSERTMSG( removedContexts == 1 , "Context not found! Can't remove the context id.");
    return static_cast<const bool>(removedContexts);
}
const void ThreadStructure::ThreadSpecific::insertSnapshotContext(const uint32_t contextID, const mace::string& snapshot){
  uint8_t serviceUID = getServiceInstance();
  event.eventSnapshotContexts[serviceUID][ contextID ] = snapshot;
}
// obsolete
void ThreadStructure::ThreadSpecific::initializeEventStack(){
//    event.eventContexts.clear();
//    event.eventMessageCount = 0;
}
void ThreadStructure::ThreadSpecific::setEventContexts(const mace::Event::EventContextType & contextIDs){
    event.eventContexts = contextIDs;
}
mace::ContextBaseClass* ThreadStructure::ThreadSpecific::myContext()const{
    return this->thisContext;
}
void ThreadStructure::ThreadSpecific::setMyContext(mace::ContextBaseClass* thisContext){
    this->thisContext = thisContext;
}
void ThreadStructure::ThreadSpecific::pushServiceInstance(const uint8_t uid){
    serviceStack.push_back( uid );
}
void ThreadStructure::ThreadSpecific::popServiceInstance(){
    ASSERT( !serviceStack.empty() );
    serviceStack.pop_back();
}
bool ThreadStructure::ThreadSpecific::isOuterMostTransition() const{
    return serviceStack.size()==1;
}
bool ThreadStructure::ThreadSpecific::isApplicationDowncall() const{
    return serviceStack.size()==0;
    //return ( eventID == 0 )? true : false;
}
bool ThreadStructure::ThreadSpecific::isFirstMaceInit() const{
    //return ( event.eventID == 0 && event.eventType == mace::Event::UNDEFEVENT )? true : false;
    return ( event.eventType == mace::Event::STARTEVENT )? false : true;
}
bool ThreadStructure::ThreadSpecific::isFirstMaceExit() const{
    //return ( event.eventID != 0 && event.eventType != mace::Event::ENDEVENT )? true : false;
    return ( event.eventType == mace::Event::ENDEVENT )? false : true;
}
bool ThreadStructure::ThreadSpecific::deferExternalMessage( uint8_t sid, MaceKey const& dest,  std::string const&  message, registration_uid_t const rid ){
  return thisContext->deferExternalMessage(event.eventOpInfo, event.target_ctx_name, sid, dest, message, rid);
  //return this->event.deferExternalMessage( sid, dest, message, rid );
}
const uint8_t ThreadStructure::ThreadSpecific::getServiceInstance() const{
    ASSERT( !serviceStack.empty() );
    return serviceStack.back();
}
void ThreadStructure::ThreadSpecific::setThreadType( const uint8_t type ){
    threadType = type;
}
uint8_t ThreadStructure::ThreadSpecific::getThreadType(){
    return threadType;
}



