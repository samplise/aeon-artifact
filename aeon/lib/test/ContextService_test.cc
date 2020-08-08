#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE libmace
#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>
#include "ContextService.h"
#include "AccessLine.h"
#include "MaceKey.h"
#include "HeadEventDispatch.h"

namespace mace{
  class __ServiceStackEvent__;
  class __ScopedTransition__;
  class __ScopedRoutine__;
};
// LocalService is for non-distributed service.
using mace::__CheckTransition__;
class __LocalTransition__;
class LocalService: public ContextService {
friend class __LocalTransition__;
public:
  LocalService():  ContextService()
  {
    mace::map<mace::MaceAddr ,mace::list<mace::string > > servContext;
    loadContextMapping( servContext);
  }
protected:
  virtual void dispatchDeferredMessages(MaceKey const& dest, mace::string const& message,  registration_uid_t const rid ) {}// no messages
  virtual void executeDeferredUpcall(  mace::ApplicationUpcall_Message* const upcall, mace::string& returnValue ) {}
};
class __LocalTransition__{
public:
    __LocalTransition__( LocalService* service, int8_t const eventType, mace::string const& targetContextName = "", mace::vector< mace::string > const& snapshotContextNames = mace::vector< mace::string >() ) {
      mace::vector< uint32_t > snapshotContextIDs;
      mace::__ServiceStackEvent__ sse( eventType, service, targetContextName );
      const mace::ContextMapping& currentMapping = service->contextMapping.getSnapshot();
      const uint32_t targetContextID = currentMapping.findIDByName( targetContextName );
      for_each( snapshotContextNames.begin(), snapshotContextNames.end(), mace::addSnapshotContextID( currentMapping, snapshotContextIDs ) );
      mace::AccessLine al( service->instanceUniqueID, targetContextID, currentMapping );

      p = new mace::__ScopedTransition__( service, targetContextID );
    }
    ~__LocalTransition__(){
      delete p;
    }

private:
  mace::__ScopedTransition__* p;
};

// InContextService is similar to mace-incontext system
template< class GlobalContextType >
class InContextService: public LocalService {
friend class mace::__ServiceStackEvent__;
public:
  InContextService():  LocalService(), globalContext(NULL) { }
private:
  GlobalContextType* globalContext;
  mace::ContextBaseClass* createContextObject( mace::string const& contextTypeName ){
    ASSERT( contextTypeName.empty() );
    ASSERT( globalContext == NULL );

    globalContext = new GlobalContextType();
    return globalContext;

  }
};
namespace mace {
  // a specialized message type. This message is used for storing information and passed around between threads, therefore it will not do serialization
  class LocalMessage: public mace::AsyncEvent_Message, public mace::PrintPrintable{
    virtual void print( std::ostream& __out ) const {
      __out << "LocalMessage()";
    }
    virtual void serialize( std::string& str ) const{ } // empty
    virtual int deserialize( std::istream& __in ) throw (mace::SerializationException){ return 0;}
  };

}
struct __async_req: public mace::LocalMessage{
  static const uint8_t messageType =12;
  uint8_t getType() const{ return messageType; }
  __asyncExtraField& getExtra() { return extra; }
  mace::Event& getEvent() { return event; }
  mace::Event event;
  mace::__asyncExtraField extra;
};
template< class GlobalContextType >
class Test1Service: public InContextService< GlobalContextType > {
public:
  Test1Service():  InContextService< GlobalContextType >() { }
  void maceInit(){ // access the global context
    this->registerInstanceID();
    //__LocalTransition__ lt( this, mace::Event::STARTEVENT );
    __CheckTransition__ cm( this, mace::Event::STARTEVENT, "" );
    __real_maceInit();

  }
  void maceExit(){ // access the global context
    //__LocalTransition__ lt( this, mace::Event::ENDEVENT );
    __CheckTransition__ cm( this, mace::Event::ENDEVENT, "" );
    __real_maceExit();
  }
private:
  void __real_maceInit(){
    async_test();
  }
  void __real_maceExit(){

  }
  void async_test(){

    __async_req* req = new __async_req;
    this->addEventRequest( req );
  }
  void test( __async_req* msg){

    async_test();
   
  }
  int deserializeMethod( std::istream& is, mace::Message*& eventObject   )  {
    uint8_t msgNum_s = static_cast<uint8_t>(is.peek() ) ;
    switch( msgNum_s  ){
      case __async_req::messageType: {
        eventObject = new __async_req;
        return mace::deserialize( is, eventObject );
        break;
      }
      default:
        ABORT("message type not found");
    }
  };
  void executeEvent( mace::AsyncEvent_Message* __param ){ 
    this->__beginRemoteMethod( __param->getEvent() );
    mace::__ScopedTransition__ st(this, __param->getExtra() );

    mace::Message *msg = static_cast< mace::Message* >( __param ) ;
    switch( msg->getType()  ){
      case __async_req::messageType: {
        __async_req* __msg = static_cast< __async_req *>( msg ) ;
        test( __msg );
        break;
      }
    }
    delete __param;
  }
  void executeRoutine(mace::Routine_Message* __param, mace::MaceAddr const & source){

  }

};
class GlobalContext: public mace::ContextBaseClass{
public:
  GlobalContext( const mace::string& contextName="", const uint64_t eventID=0, const uint8_t instanceUniqueID=0, const uint32_t contextID=0 ):
    mace::ContextBaseClass( contextName, eventID, instanceUniqueID, contextID ){
  }
private:
  // declare context variables here
};

int main(int argc, char* argv[]){
  mace::Init( argc, argv );
  const char *_argv[] = {""};
  int _argc = 1;
  ::boost::unit_test::unit_test_main( &init_unit_test, _argc, const_cast<char**>(_argv) );

  mace::Shutdown();
}

BOOST_AUTO_TEST_SUITE( lib_ContextService )

BOOST_AUTO_TEST_CASE( Case1 )
{ // test single services
  // test: create start event, async event, timer event, message delivery event, downcall event, upcall event,
  //       test routines (local)
  BOOST_TEST_CHECKPOINT("Constructor");
  Test1Service<GlobalContext> service1;
  BOOST_TEST_CHECKPOINT("maceInit");
  service1.maceInit();
  SysUtil::sleep( 10 );
  BOOST_TEST_CHECKPOINT("maceExit");
  service1.maceExit();
}
BOOST_AUTO_TEST_SUITE_END()
