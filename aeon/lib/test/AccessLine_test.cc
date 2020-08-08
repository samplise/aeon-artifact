
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE libmace
#include <boost/test/unit_test.hpp>
#include "ThreadStructure.h"
#include "Event.h"
#include "ContextLock.h"
#include "AccessLine.h"
#include "ContextMapping.h"
#include "mace.h"
BOOST_AUTO_TEST_SUITE( lib_AccessLine )

// test 1: only global context exists, and see if an event can enter it without owning any contexts.
BOOST_AUTO_TEST_CASE( Case1 )
{
  const uint32_t serviceID = 0;
  ThreadStructure::ScopedServiceInstance si( serviceID );
  mace::map<uint8_t, mace::set< uint32_t > > contextIDs;
  mace::set< uint32_t > ctxSet;

  contextIDs[ serviceID ] = ctxSet;
  mace::Event currentEvent( mace::Event::STARTEVENT );
  currentEvent.eventContexts = contextIDs;
  ThreadStructure::setEvent( currentEvent );
  mace::ContextMapping contextMapping;

  ThreadStructure::newTicket();
  mace::AgentLock alock( mace::AgentLock::WRITE_MODE );
  contextMapping.setDefaultAddress( Util::getMaceAddr() );

  mace::Event::setLastContextMappingVersion( currentEvent.eventID );
  const std::pair< mace::MaceAddr, uint32_t> nm = contextMapping.newMapping( "" );
  contextMapping.snapshot();

  const mace::ContextMapping& currentMapping = contextMapping.getSnapshot();
  alock.downgrade( mace::AgentLock::READ_MODE );
  

  BOOST_REQUIRE( mace::AccessLine::granted(serviceID, 1, currentMapping) );
}
// test 2: only global context exists. an event who is already in global context and re-enter it
BOOST_AUTO_TEST_CASE( Case2 )
{
  const uint32_t serviceID = 0;
  ThreadStructure::ScopedServiceInstance si( serviceID );
  mace::map<uint8_t, mace::set< uint32_t > > contextIDs;
  mace::set< uint32_t > ctxSet;
  ctxSet.insert( 1 ); // global

  contextIDs[ serviceID ] = ctxSet;
  mace::Event currentEvent( mace::Event::ASYNCEVENT );
  currentEvent.eventContexts = contextIDs;
  ThreadStructure::setEvent( currentEvent );
  mace::ContextMapping contextMapping;

  ThreadStructure::newTicket();
  mace::AgentLock alock( mace::AgentLock::WRITE_MODE );
  contextMapping.setDefaultAddress( Util::getMaceAddr() );

  mace::Event::setLastContextMappingVersion( currentEvent.eventID );
  const std::pair< mace::MaceAddr, uint32_t> nm = contextMapping.newMapping( "" );
  contextMapping.snapshot();

  const mace::ContextMapping& currentMapping = contextMapping.getSnapshot();
  alock.downgrade( mace::AgentLock::READ_MODE );
  

  BOOST_REQUIRE( mace::AccessLine::granted(serviceID, 1, currentMapping) );
}
// test 3: an event at global context can enter its subcontexts
BOOST_AUTO_TEST_CASE( Case3 )
{
  const uint32_t serviceID = 0;
  ThreadStructure::ScopedServiceInstance si( serviceID );
  mace::map<uint8_t, mace::set< uint32_t > > contextIDs;
  mace::set< uint32_t > ctxSet;
  ctxSet.insert( 1 ); // global

  contextIDs[ serviceID ] = ctxSet;
  mace::Event currentEvent( mace::Event::ASYNCEVENT );
  currentEvent.eventContexts = contextIDs;
  ThreadStructure::setEvent( currentEvent );
  mace::ContextMapping contextMapping;

  ThreadStructure::newTicket();
  mace::AgentLock alock( mace::AgentLock::WRITE_MODE );
  contextMapping.setDefaultAddress( Util::getMaceAddr() );

  mace::Event::setLastContextMappingVersion( currentEvent.eventID );
  const std::pair< mace::MaceAddr, uint32_t> nm1 = contextMapping.newMapping( "" );
  const std::pair< mace::MaceAddr, uint32_t> nm2 = contextMapping.newMapping( "A[0]" );
  const std::pair< mace::MaceAddr, uint32_t> nm3 = contextMapping.newMapping( "A[1]" );
  contextMapping.snapshot();

  const mace::ContextMapping& currentMapping = contextMapping.getSnapshot();
  alock.downgrade( mace::AgentLock::READ_MODE );
  

  BOOST_REQUIRE(mace::AccessLine::granted(serviceID, 2, currentMapping) ); // can the event enter A[0]?
  BOOST_REQUIRE(mace::AccessLine::granted(serviceID, 3, currentMapping) ); // can the event enter A[1]?
}
// test4 three contexts: global, A[0] and A[1]. can an event enter subcontexts from global context?
BOOST_AUTO_TEST_CASE( Case4 )
{
  const uint32_t serviceID = 0;
  ThreadStructure::ScopedServiceInstance si( serviceID );
  mace::map<uint8_t, mace::set< uint32_t > > contextIDs;
  mace::set< uint32_t > ctxSet;
  ctxSet.insert( 1 ); // global

  contextIDs[ serviceID ] = ctxSet;
  mace::Event currentEvent( mace::Event::ASYNCEVENT );
  currentEvent.eventContexts = contextIDs;
  ThreadStructure::setEvent( currentEvent );
  mace::ContextMapping contextMapping;

  ThreadStructure::newTicket();
  mace::AgentLock alock( mace::AgentLock::WRITE_MODE );
  contextMapping.setDefaultAddress( Util::getMaceAddr() );

  mace::Event::setLastContextMappingVersion( currentEvent.eventID );
  const std::pair< mace::MaceAddr, uint32_t> nm1 = contextMapping.newMapping( "" );
  const std::pair< mace::MaceAddr, uint32_t> nm2 = contextMapping.newMapping( "A[0]" );
  const std::pair< mace::MaceAddr, uint32_t> nm3 = contextMapping.newMapping( "A[1]" );
  contextMapping.snapshot();

  const mace::ContextMapping& currentMapping = contextMapping.getSnapshot();
  alock.downgrade( mace::AgentLock::READ_MODE );
  

  BOOST_REQUIRE(mace::AccessLine::granted(serviceID, 2, currentMapping) ); // can the event enter A[0]?
  BOOST_REQUIRE(mace::AccessLine::granted(serviceID, 3, currentMapping) ); // can the event enter A[1]?
}
// test 5: an event in context A[0] can not access global and A[1] context
BOOST_AUTO_TEST_CASE( Case5 )
{
  const uint32_t serviceID = 0;
  ThreadStructure::ScopedServiceInstance si( serviceID );
  mace::map<uint8_t, mace::set< uint32_t > > contextIDs;
  mace::set< uint32_t > ctxSet;
  ctxSet.insert( 2 ); // global

  contextIDs[ serviceID ] = ctxSet;
  mace::Event currentEvent( mace::Event::ASYNCEVENT );
  currentEvent.eventContexts = contextIDs;
  ThreadStructure::setEvent( currentEvent );
  mace::ContextMapping contextMapping;

  ThreadStructure::newTicket();
  mace::AgentLock alock( mace::AgentLock::WRITE_MODE );
  contextMapping.setDefaultAddress( Util::getMaceAddr() );

  mace::Event::setLastContextMappingVersion( currentEvent.eventID );
  const std::pair< mace::MaceAddr, uint32_t> nm1 = contextMapping.newMapping( "" );
  const std::pair< mace::MaceAddr, uint32_t> nm2 = contextMapping.newMapping( "A[0]" );
  const std::pair< mace::MaceAddr, uint32_t> nm3 = contextMapping.newMapping( "A[1]" );
  contextMapping.snapshot();

  const mace::ContextMapping& currentMapping = contextMapping.getSnapshot();
  alock.downgrade( mace::AgentLock::READ_MODE );
  
  BOOST_TEST_CHECKPOINT("An event start from context A[0] should not be granted to access global context");
  BOOST_REQUIRE( !mace::AccessLine::granted(serviceID, 1, currentMapping) ); // can the event enter A[0]?
  BOOST_TEST_CHECKPOINT("An event start from context A[0] should not be granted to access the sibling context A[1]");
  BOOST_REQUIRE( !mace::AccessLine::granted(serviceID, 3, currentMapping) ); // can the event enter A[1]?
}
// more tests: service composition, with snapshot contexts, etc
BOOST_AUTO_TEST_SUITE_END()
