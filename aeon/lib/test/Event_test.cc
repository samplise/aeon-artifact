#include "Event.h"
#include "ThreadStructure.h"
using namespace mace;

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE libmace
#include <boost/test/unit_test.hpp>
#include "ThreadStructure.h"
#include "Event.h"
#include "ContextLock.h"
#include "AccessLine.h"
#include "ContextMapping.h"
#include "mace.h"
using namespace mace;
BOOST_AUTO_TEST_SUITE( lib_Event )


BOOST_AUTO_TEST_CASE( Case1 )
{
  ThreadStructure::newTicket();
  mace::Event& newEvent = ThreadStructure::myEvent();
  newEvent.newEventID( Event::STARTEVENT );
  newEvent.initialize(  );

  BOOST_REQUIRE( newEvent.getEventID() == ThreadStructure::myTicket() );
  BOOST_REQUIRE( newEvent.getEventType() == Event::STARTEVENT );

  uint8_t serviceID = 0;
  uint32_t contextID = 1;
  uint64_t skipID = 1;
  //newEvent.setSkipID( serviceID, contextID, skipID );

      mace::Event::EventSkipRecordType & skipIDStorage = newEvent.getSkipIDStorage( serviceID );
      skipIDStorage.set( contextID, skipID );

  mace::vector< uint32_t> parentContextIDs;
  BOOST_REQUIRE( skipID == newEvent.getSkipID( serviceID, contextID, parentContextIDs ) );
}
BOOST_AUTO_TEST_CASE( Case2 )
{
  mace::string s;
  Event e( Event::ASYNCEVENT );
  Event de;
  mace::serialize( s, &e );

  mace::deserialize( s, &de );

  BOOST_REQUIRE( e.eventType == de.eventType );
  BOOST_REQUIRE( e.eventID == de.eventID );
  BOOST_REQUIRE( e.eventContexts == de.eventContexts );
  BOOST_REQUIRE( e.eventSnapshotContexts == de.eventSnapshotContexts );
  BOOST_REQUIRE( e.eventContextMappingVersion == de.eventContextMappingVersion );
  BOOST_REQUIRE( e.eventSkipID == de.eventSkipID );
  BOOST_REQUIRE( e.eventMessages == de.eventMessages );
}

BOOST_AUTO_TEST_SUITE_END()
/*
int main(){

  Event e( Event::STARTEVENT );
  uint8_t sid = 0;

  uint32_t ctxid = 1;

  e.eventContexts[sid].insert( ctxid );
  for( uint32_t count = 0;  count < 1000*1000*10; count++ ){
    ThreadStructure::setEvent( e );
  }


}*/
