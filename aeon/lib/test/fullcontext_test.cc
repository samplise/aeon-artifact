#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE libmace
#include <boost/test/unit_test.hpp>
#include "ContextMapping.h"
#include "Event.h"
#include "ThreadStructure.h"
#include "mace.h"


BOOST_AUTO_TEST_SUITE( lib_ContextMapping )

BOOST_AUTO_TEST_CASE( DefaultAddress )
{
    mace::ContextMapping cm;
    BOOST_TEST_CHECKPOINT("Without setting default address, getNodeByContext() should return SockUtil::NULL_MACEADDR");
    ThreadStructure::newTicket();
    mace::AgentLock alock( mace::AgentLock::WRITE_MODE );
    uint64_t eventID =1;
    mace::Event he( mace::Event::ASYNCEVENT );
    cm.setDefaultAddress( Util::getMaceAddr() );
    ThreadStructure::setEvent( he );
    ThreadStructure::setEventContextMappingVersion( );

    //mace::ContextLock c_lock( mace::ContextBaseClass::headContext, mace::ContextLock::WRITE_MODE );
    cm.snapshot( eventID );
    BOOST_REQUIRE( !cm.hasContext("")  );
    //mace::MaceKey j = mace::MaceKey(mace::ipv4, "cloud01.cs.purdue.edu");

    BOOST_TEST_CHECKPOINT("If no mapped data, getNodeByContext() returns defaultAddress");
    BOOST_REQUIRE( cm.getHead() == Util::getMaceAddr() );

    //c_lock.downgrade( mace::ContextLock::NONE_MODE );
    alock.downgrade( mace::AgentLock::NONE_MODE );
}

BOOST_AUTO_TEST_CASE(ConstructorWithParameter)
{
    ThreadStructure::newTicket();
    mace::AgentLock alock( mace::AgentLock::WRITE_MODE );
    BOOST_TEST_CHECKPOINT("Test getNodeByContext() if mapping is supplied in constructor");
    mace::MaceKey vheadNode( mace::ipv4, "google.com" );
    mace::MaceAddr vhead = vheadNode.getMaceAddr();
    mace::MaceAddr node1 = mace::MaceKey( mace::ipv4, "cloud01.cs.purdue.edu" ).getMaceAddr();
    mace::MaceAddr node2 = mace::MaceKey( mace::ipv4, "cloud02.cs.purdue.edu" ).getMaceAddr();
    const char* contexts1[] = {"", "R[0]", "R[1]", "R[2]", "T", "R[0].C[0]", "R[0].C[1]", "M[0,0]" };
    const char* contexts2[] = {"R[0].C[2]", "M[0,1]"};
    mace::list< mace::string > ctxlist0;
    ctxlist0.push_back(  mace::ContextMapping::getHeadContext() );
    mace::list< mace::string > ctxlist1;
    for( uint32_t c = 0; c< 8; c++ ) ctxlist1.push_back( contexts1[c] );
    mace::list< mace::string > ctxlist2;
    for( uint32_t c = 0; c< 2; c++ ) ctxlist2.push_back( contexts2[c] );

    mace::map< mace::MaceAddr, mace::list<mace::string> > ctxmap;
    ctxmap[ vhead ] = ctxlist1;
    ctxmap[ node1 ] = ctxlist1;
    ctxmap[ node2 ] = ctxlist2;

    mace::ContextMapping cm2;
    cm2.loadMapping( ctxmap );


    uint64_t eventID =2;


    mace::Event he( mace::Event::ASYNCEVENT );
    ThreadStructure::setEvent( he );
    ThreadStructure::setEventContextMappingVersion(  );

    //mace::ContextLock c_lock( mace::ContextBaseClass::headContext, mace::ContextLock::WRITE_MODE );
    cm2.newMapping( "" );
    cm2.snapshot( eventID );
    //c_lock.downgrade( mace::ContextLock::NONE_MODE );
    alock.downgrade( mace::AgentLock::NONE_MODE );

    BOOST_REQUIRE( cm2.getNodeByContext("") == node1 );
}

BOOST_AUTO_TEST_CASE(LoadMapping)
{
    ThreadStructure::newTicket();
    mace::AgentLock alock( mace::AgentLock::WRITE_MODE );
    BOOST_TEST_CHECKPOINT("Test getNodeByContext() if mapping is supplied in loadMapping()");
    mace::MaceKey vheadNode( mace::ipv4, "google.com" );
    mace::MaceAddr vhead = vheadNode.getMaceAddr();
    mace::MaceAddr node1 = mace::MaceKey( mace::ipv4, "cloud01.cs.purdue.edu" ).getMaceAddr();
    mace::MaceAddr node2 = mace::MaceKey( mace::ipv4, "cloud02.cs.purdue.edu" ).getMaceAddr();
    const char* contexts1[] = {"", "R[0]", "R[1]", "R[2]", "T", "R[0].C[0]", "R[0].C[1]", "M[0,0]" };
    const char* contexts2[] = {"R[0].C[2]", "M[0,1]"};
    mace::list< mace::string > ctxlist1;
    for( uint32_t c = 0; c< 8; c++ ) ctxlist1.push_back( contexts1[c] );
    mace::list< mace::string > ctxlist2;
    for( uint32_t c = 0; c< 2; c++ ) ctxlist2.push_back( contexts2[c] );
    mace::list< mace::string > ctxlisthead;
    ctxlisthead.push_back( "__head" );

    mace::map< mace::MaceAddr, mace::list<mace::string> > ctxmap;
    ctxmap[ node1 ] = ctxlist1;
    ctxmap[ node2 ] = ctxlist2;
    ctxmap[ vhead ] = ctxlisthead;

    mace::ContextMapping cm2;
    cm2.loadMapping( ctxmap );

    uint64_t eventID =3;

    mace::Event he( mace::Event::ASYNCEVENT );
    ThreadStructure::setEvent( he );
    ThreadStructure::setEventContextMappingVersion(  );

    //mace::ContextLock c_lock( mace::ContextBaseClass::headContext, mace::ContextLock::WRITE_MODE );
    cm2.newMapping( "" );
    cm2.snapshot( eventID );
    //c_lock.downgrade( mace::ContextLock::NONE_MODE );

    alock.downgrade( mace::AgentLock::NONE_MODE );
    BOOST_REQUIRE( cm2.getNodeByContext("") == node1 );

    BOOST_REQUIRE( cm2.getNodeByContext("") == node1 );
}
BOOST_AUTO_TEST_CASE(AccessedContext)
{
    ThreadStructure::newTicket();
    mace::AgentLock alock( mace::AgentLock::WRITE_MODE );
    BOOST_TEST_CHECKPOINT("Test accessedContext() returned false after the same context name is called the first time.");
    mace::MaceKey test( mace::ctxnode, "google.com" );
    mace::MaceKey vheadNode( mace::ipv4, "google.com" );
    mace::MaceAddr vhead = vheadNode.getMaceAddr();
    mace::MaceAddr node1 = mace::MaceKey( mace::ipv4, "cloud01.cs.purdue.edu" ).getMaceAddr();
    mace::MaceAddr node2 = mace::MaceKey( mace::ipv4, "cloud02.cs.purdue.edu" ).getMaceAddr();
    const char* contexts1[] = {"", "R[0]", "R[1]", "R[2]", "T", "R[0].C[0]", "R[0].C[1]", "M[0,0]" };
    const char* contexts2[] = {"R[0].C[2]", "M[0,1]"};
    mace::list< mace::string > ctxlist1;
    for( uint32_t c = 0; c< 8; c++ ) ctxlist1.push_back( contexts1[c] );
    mace::list< mace::string > ctxlist2;
    for( uint32_t c = 0; c< 2; c++ ) ctxlist2.push_back( contexts2[c] );
    mace::list< mace::string > ctxlisthead;
    ctxlisthead.push_back( "__head" );

    mace::map< mace::MaceAddr, mace::list<mace::string> > ctxmap;
    ctxmap[ node1 ] = ctxlist1;
    ctxmap[ node2 ] = ctxlist2;
    ctxmap[ vhead ] = ctxlisthead;

    mace::ContextMapping cm2;
    cm2.loadMapping( ctxmap );
    uint64_t eventID =4;
    mace::Event he( mace::Event::ASYNCEVENT );

    ThreadStructure::setEvent( he );
    ThreadStructure::setEventContextMappingVersion(  );
    //mace::ContextLock c_lock( mace::ContextBaseClass::headContext, mace::ContextLock::WRITE_MODE );
    cm2.snapshot( eventID );
    alock.downgrade( mace::AgentLock::NONE_MODE );
    //c_lock.downgrade( mace::ContextLock::NONE_MODE );

    //BOOST_REQUIRE( cm2.accessedContext("") == false );
    //BOOST_TEST_CHECKPOINT("Test accessedContext() returned true after the same context name is called the second time.");
    //BOOST_REQUIRE( cm2.accessedContext("") == true );
}
#include "mace-macros.h"
#include "inttypes.h"
#include "CollectionSerializers.h"
//#include "mmultimap.h"
/*#include "TcpTransport-init.h"
BOOST_AUTO_TEST_CASE(VirtualNode)
{
    mace::MaceKey vnode( mace::vnode, 1 );
    mace::ContextMapping::setVirtualNodeMaceKey( vnode );
    TransportServiceClass* tcp = &( TcpTransport_namespace::new_TcpTransport_Transport() );
    mace::MaceKey localAddress = tcp->localAddress();

    BOOST_REQUIRE_EQUAL( localAddress , vnode );
}*/


BOOST_AUTO_TEST_SUITE_END()
