#ifndef _MACE_TICKET_H
#define _MACE_TICKET_H

#include <list>
#include <pthread.h>

#include "mhash_map.h"
#include "mvector.h"
#include "mdeque.h"
#include "mhash_set.h"
#include "mace-macros.h"
#include "Log.h"
#include "mset.h"

#include "Event.h"

/**
 * \file ThreadStructure.h
 * \brief declares the ThreadStructure class for storing thread-local information including the current event
 */
namespace mace{
class ContextBaseClass;
}

/**
   * \brief ThreadStructure class is used for storing thread-local information including the current event
   *
   * */
class ThreadStructure {
    class ThreadSpecific;
    public:
    static const uint8_t UNDEFINED_THREAD_TYPE = 0;
    static const uint8_t ASYNC_THREAD_TYPE     = 1;
    static const uint8_t TRANSPORT_THREAD_TYPE = 2;
    static const uint8_t HEAD_THREAD_TYPE      = 3;
  private:
		//Ticket relevant member variables
    static uint64_t nextTicketNumber;
    static uint64_t nextMessageTicketNumber;
    static uint64_t current_valid_ticket;
    static pthread_mutex_t ticketMutex;

	public:
    static void releaseThreadSpecificMemory(){
      ThreadSpecific::releaseThreadSpecificMemory();
    }
    
    /// returns a new ticket which is monotonically increasing
    static mace::OrderID newTicket(mace::ContextBaseClass* contextObj);


    static mace::OrderID newTickets( mace::ContextBaseClass* contextObj, const uint32_t nTickets );

    static uint64_t newMessageTicket( );

    /* set the current event
     * @param event the event object to be set
     * */
    static void setEvent(const mace::Event& event){
        ADD_SELECTORS("ThreadStructure::setEvent");
        //macedbg(1)<<"Set event with id = "<< event.eventId << Log::endl;
      	ThreadSpecific::init()->setEvent(event);
    }
    /// set the current event ID
    static void setEventID(const mace::OrderID& eventId){
        ADD_SELECTORS("ThreadStructure::setEventID");
        macedbg(1)<<"Set event id = "<< eventId << Log::endl;
      	ThreadSpecific::init()->setEventID(eventId);
    }

    /// returns the current ticket number
    static const mace::OrderID& myEventID() {
      	const ThreadSpecific *t = ThreadSpecific::init();
      	return t->myEventID();
    }
    /// returns the current event
    static mace::Event& myEvent() {
      	ThreadSpecific *t = ThreadSpecific::init();
      	return t->myEvent();
    }
    /// returns current event context mapping version
    static const uint64_t getEventContextMappingVersion(){
      	const ThreadSpecific *t = ThreadSpecific::init();
      	return t->getEventContextMappingVersion();
    }
    /// set current event context mapping version
    /*
    static void setEventContextMappingVersion( ){
      	ThreadSpecific *t = ThreadSpecific::init();
      	return t->setEventContextMappingVersion( );
    }
    */
    /** set ver as current event context mapping version 
     * @param ver the context mapping version
     * */
    static void setEventContextMappingVersion(const uint64_t ver ){
      	ThreadSpecific *t = ThreadSpecific::init();
      	return t->setEventContextMappingVersion( ver );
    }

    /// returns the current context object
    static mace::ContextBaseClass* myContext(){
        ADD_SELECTORS("ThreadStructure::myContext");
      	ThreadSpecific *t = ThreadSpecific::init();
        if( t->myContext() == NULL) {
          //macedbg(1) << "Return NULL for myContext!" << Log::endl;
        }
        //ASSERTMSG(t->myContext() != NULL, "ThreadStructure::myContext() object returned NULL!");
      	return t->myContext();
    }
    /// set the current context object
    static void setMyContext(mace::ContextBaseClass* thisContext){
        ADD_SELECTORS("ThreadStructure::setMyContext");
        //ASSERTMSG(thisContext != NULL, "ThreadStructure::setMyContext() received a NULL pointer!");
      	ThreadSpecific *t = ThreadSpecific::init();
        t->setMyContext( thisContext );
    }

    class ScopedContextObject{
        private:
        ThreadSpecific *t;
        mace::ContextBaseClass* origContextObj;
        public: ScopedContextObject(mace::ContextBaseClass* thisContext){
            t = ThreadSpecific::init();
            origContextObj = t->myContext();
            t->setMyContext( thisContext );
        }
        ~ScopedContextObject(){
            t->setMyContext( origContextObj );
        }
    };

    
    static mace::OrderID current_eventId() {
      mace::OrderID currentId(0, current_valid_ticket);
      return currentId;
    }
    

    static uint32_t getCurrentContext(){
        ThreadSpecific *t = ThreadSpecific::init();
        return t->getCurrentContext();
    }

    static void popContext(){
        ThreadSpecific *t = ThreadSpecific::init();
        t->popContext();
    }

    static void pushContext(const uint32_t contextId){
        ADD_SELECTORS("ThreadStructure::pushContext");
        //macedbg(1)<<"Set context ID as "<<contextId<<Log::endl;
        ThreadSpecific *t = ThreadSpecific::init();
        t->pushContext(contextId);
    }
    class ScopedContextID{
        private:
        ThreadSpecific *t;
        public: ScopedContextID(const uint32_t contextID){
            t = ThreadSpecific::init();
            t->pushContext(contextID);
        }
        ~ScopedContextID(){
            t->popContext();
        }
    };
    static void popServiceInstance(){
        ThreadSpecific *t = ThreadSpecific::init();
        t->popServiceInstance();
    }

    static void pushServiceInstance(const uint8_t sid){
        ADD_SELECTORS("ThreadStructure::pushServiceInstance");
        //macedbg(1)<<"Set context ID as "<<contextID<<Log::endl;
        ThreadSpecific *t = ThreadSpecific::init();
        t->pushServiceInstance(sid);
    }
    class ScopedServiceInstance{
        private:
        ThreadSpecific *t;
        public: ScopedServiceInstance(const uint8_t uid){
            t = ThreadSpecific::init();
            t->pushServiceInstance(uid);
        }
        ~ScopedServiceInstance(){
            t->popServiceInstance();
        }
    };

    /**
     * defer an external message until the current event commits
     * 
     * @param dest MaceKey of the external node
     * @param message the serialized message
     * @param rid registration id of the service of the message
     *
     * */
    static bool deferExternalMessage( MaceKey const& dest,  std::string const&  message, registration_uid_t const rid ){
        ThreadSpecific *t = ThreadSpecific::init();
        uint8_t sid = t->getServiceInstance();
        return t->deferExternalMessage(sid, dest, message, rid);
    }
    /**
     * returns the current event service ID
     * */
    static uint8_t getServiceInstance()  {
        ThreadSpecific *t = ThreadSpecific::init();
        return t->getServiceInstance();
    }
    /// This is used in downcall transitions except maceInit and maceExit
    static bool isOuterMostTransition( ){
        ThreadSpecific *t = ThreadSpecific::init();
        return t->isOuterMostTransition();
    }
    /**
     * determine if the transition is called from application
     * */
    static bool isApplicationDowncall( ){
        ThreadSpecific *t = ThreadSpecific::init();
        return t->isApplicationDowncall();
    }
    /// Determine if the current MaceInit is the first to be executed
    static bool isFirstMaceInit( ){
        ThreadSpecific *t = ThreadSpecific::init();
        return t->isFirstMaceInit();
    }
    /// Determine if the current MaceExit is the first to be executed
    static bool isFirstMaceExit( ){
        ThreadSpecific *t = ThreadSpecific::init();
        return t->isFirstMaceExit();
    }

    /**
     * This function returns a set of contexts owned by the event
     * */
    static const mace::Event::EventContextType& getEventContexts(){
        ThreadSpecific *t = ThreadSpecific::init();
        return  t->getEventContexts();
    }
    /**
     * returns the set of contexts owned by the current event in the current service
     * */
    static const mace::Event::EventServiceContextType& getCurrentServiceEventContexts(){
        ThreadSpecific *t = ThreadSpecific::init();
        return  t->getCurrentServiceEventContexts();
    }
    /**
     * returns the set of snapshot contexts owned by the current event in all services
     * */
    static const mace::Event::EventSnapshotContextType& getEventSnapshotContexts(){
        ThreadSpecific *t = ThreadSpecific::init();
        return  t->getEventSnapshotContexts();
    }
    /**
     * returns the set of snapshot contexts owned by the current event in the current service
     * */
    static const mace::Event::EventServiceSnapshotContextType& getCurrentServiceEventSnapshotContexts(){
        ThreadSpecific *t = ThreadSpecific::init();
        return  t->getCurrentServiceEventSnapshotContexts();
    }
    /*
    static const mace::vector<mace::PreEventInfo> getPreEventInfos(const uint8_t serviceID, const mace::string& ctx_name){
        ThreadSpecific *t = ThreadSpecific::init();
        return  t->getPreEventInfos(serviceID, ctx_name);
    }
    */
    /**
     * determine whether or not the current event has ever entered a service
     * @param serviceID the numerical ID of the service
     * @return TRUE if the event has entered the service
     * */
    static const bool  isEventEnteredService(const uint8_t serviceID){
        ThreadSpecific *t = ThreadSpecific::init();
        return  t->isEventEnteredService(serviceID);
    }
    /**
     * This function inserts a context id owned by the event
     * @param contextID the numerical ID of the context
     * @return TRUE if successful
     * */
    static const bool insertEventContext(const uint32_t contextID){
        ThreadSpecific *t = ThreadSpecific::init();
        return  t->insertEventContext(contextID);
    }
    /**
     * This function removes a context id (because event downgrades)
     * */
    static const bool removeEventContext(const uint32_t contextID){
        ThreadSpecific *t = ThreadSpecific::init();
        return  t->removeEventContext(contextID);
    }

    static void insertSnapshotContext(const uint32_t contextID, const mace::string& snapshot){
        ThreadSpecific *t = ThreadSpecific::init();
        t->insertSnapshotContext(contextID, snapshot);
    }
    /**
     * This function erases all context IDs and resets message counter
     * */
    static void initializeEventStack(){
        ThreadSpecific *t = ThreadSpecific::init();
        t->initializeEventStack();
    }
    /*static void setLastWriteContextMapping(){
      	ThreadSpecific *t = ThreadSpecific::init();
      	return t->setLastWriteContextMapping( );
    }*/

    /*static const uint64_t getCurrentServiceEventSkipID(const mace::string& contextID){
        ThreadSpecific *t = ThreadSpecific::init();
        return  t->getCurrentServiceEventSkipID(contextID);
    }*/
    /**
     * This function resets the contexts of an event (when returning from an sync call)
     * */
    /*static void setEventContexts(const mace::map<uint8_t, mace::set<mace::string> >& contextIDs){
        ThreadSpecific *t = ThreadSpecific::init();
        t->setEventContexts(contextIDs);
    }*/
    /**
     * This function returns a set of child-contexts of a context owned by the event
     * */
    /*static mace::set<mace::string>& getEventChildContexts(const mace::string& contextID){
        //ThreadSpecific *t = ThreadSpecific::init();
        //return  t->getEventChildContexts( contextID );


        // TODO: use versionlized context map snapshot
    }*/

    static void setThreadType( const uint8_t type ){
        ThreadSpecific *t = ThreadSpecific::init();
        t->setThreadType( type );
    }
    static uint8_t getThreadType(){
        ThreadSpecific *t = ThreadSpecific::init();
        return  t->getThreadType();
    }

    /// \todo not implemented yet
    static bool isNoneContext(){
        return false; // TODO: not completed
    }
    /// chuangw: XXX: not used currently
    static uint64_t getHighestTicketNumber(){
      // TODO: lock
        return nextTicketNumber;
    }
    /// chuangw: XXX: not used currently
    static void markTicketServed() {
      ThreadSpecific *t = ThreadSpecific::init();
      t->markTicketServed();
      return;
    }
/* 
    /// chuangw: XXX: not used currently
    static void releaseTicket() {// chuangw: XXX: not used currently
      ThreadSpecific *t = ThreadSpecific::init();
      current_valid_ticket = t->myTicket() + 1;
      return;
    }

    /// chuangw: XXX: not used currently
    bool checkTicket() { // chuangw: XXX: not used currently
      ThreadSpecific *t = ThreadSpecific::init();
      if (current_valid_ticket == t->myTicket()) return true;
      else return false;
    }
*/
    /*static uint32_t incrementEventMessageCount(){
        ThreadSpecific *t = ThreadSpecific::init();
        return  t->incrementEventMessageCount();
    }*/
    /*static const uint32_t& getEventMessageCount(){
        ThreadSpecific *t = ThreadSpecific::init();
        return  t->getEventMessageCount();
    }
    static void setEventMessageCount(const uint32_t count){
        ThreadSpecific *t = ThreadSpecific::init();
        return  t->setEventMessageCount(count);
    }*/
  private:
    class ThreadSpecific {
      public:
        ThreadSpecific();
        ~ThreadSpecific();
        static ThreadSpecific* init();
        static void releaseThreadSpecificMemory();
        const mace::OrderID& myEventID() const;
        mace::Event& myEvent();
        const uint64_t getEventContextMappingVersion() const;
        void setEventContextMappingVersion( );
        void setEventContextMappingVersion( const uint64_t ver );
        mace::ContextBaseClass* myContext() const;
        void setMyContext(mace::ContextBaseClass* thisContext);
        void setEvent(const mace::Event& _event);
        void setEventID(const mace::OrderID& eventId);
        void setMessageTicket( const uint64_t msgTicket );
        void markTicketServed() { ticketIsServed = true; }

        const uint32_t getCurrentContext() const;
        void pushContext(const uint32_t contextID);
        void popContext();
        void pushServiceInstance(const uint8_t uid);
        void popServiceInstance();
        bool deferExternalMessage( uint8_t sid, MaceKey const& dest,  std::string const&  message, registration_uid_t const rid );
        const uint8_t getServiceInstance() const;
        bool isOuterMostTransition( ) const;
        bool isApplicationDowncall( ) const;
        bool isFirstMaceInit( ) const;
        bool isFirstMaceExit( ) const;

        const mace::Event::EventContextType& getEventContexts() const;
        const mace::Event::EventServiceContextType & getCurrentServiceEventContexts() ;
        const mace::Event::EventSnapshotContextType & getEventSnapshotContexts() ;
        const mace::Event::EventServiceSnapshotContextType & getCurrentServiceEventSnapshotContexts() ;
        //const mace::vector<mace::PreEventInfo> getPreEventInfos(const uint8_t serviceID, const mace::string& ctx_name) const ;
        const bool isEventEnteredService(const uint8_t serviceID) const;
        const bool insertEventContext(const uint32_t contextID);
        const bool removeEventContext(const uint32_t contextID);
        const void insertSnapshotContext(const uint32_t contextID, const mace::string& snapshot);
        void setEventContexts(const mace::Event::EventContextType& contextIDs);
        void initializeEventStack();
        void setThreadType( const uint8_t type );
        uint8_t getThreadType();

      private:
        static void initKey();

      private:
        static pthread_key_t pkey;
        static pthread_once_t keyOnce;
        static unsigned int count;

        mace::Event event;
        
        bool ticketIsServed;

        mace::ContextBaseClass* thisContext;
        mace::deque< uint32_t > contextStack;

        mace::deque< uint8_t > serviceStack;
        uint8_t threadType; ///< thread type is defined when the thread is start/created
    }; // ThreadSpecific
};
#endif
