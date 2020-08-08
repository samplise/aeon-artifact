
// TODO: test newTicket()
// test setTicket()
// test setEvent()
// test myTicket()
// test myContext()
// test setMyContext()
// test getCurrentContext()
// test pushContext()
// test popContext()
// test getEventContext()
// test getEventChildContexts()
// test isValidContextRequest()

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE libmace
#include <boost/test/unit_test.hpp>
#include "ThreadStructure.h"

#include <typeinfo>
#include <sstream>

BOOST_AUTO_TEST_SUITE( lib_ThreadStructure )

BOOST_AUTO_TEST_SUITE( newTicket )

BOOST_AUTO_TEST_CASE( FirstTicket )
{
  BOOST_TEST_CHECKPOINT("Generates tickets from 1.");
  ThreadStructure::newTicket();
  BOOST_REQUIRE( ThreadStructure::myTicket() >= 1  );
}
BOOST_AUTO_TEST_CASE( TicketType )
{
  BOOST_TEST_CHECKPOINT("Ticket type should be uint64_t");
  BOOST_REQUIRE( typeid( ThreadStructure::myTicket() ) == typeid( uint64_t ) );
}
BOOST_AUTO_TEST_CASE( IncrementTicket )
{
  BOOST_TEST_CHECKPOINT("Tickets increment by 1.");
  for(uint64_t i=0;i< 10000;i++){
    ThreadStructure::newTicket();
    BOOST_REQUIRE_EQUAL( ThreadStructure::myTicket() , i+2  );
  }
}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( ContextID )
BOOST_AUTO_TEST_CASE( PushPop )
{
  for(uint32_t n=1;n<=10000;n++){
    //std::ostringstream oss;
    //oss<<"context"<<n;
    ThreadStructure::pushContext( n );
  }
  BOOST_REQUIRE_EQUAL( ThreadStructure::getCurrentContext() , (uint32_t)10000 );
  for(int n=1;n<=10000;n++){
    ThreadStructure::popContext();
  }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( TransitionValidity )
BOOST_AUTO_TEST_CASE( TestValid )
{

}
BOOST_AUTO_TEST_CASE( TestInValid )
{

}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
