#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE cjtest
#include <boost/test/unit_test.hpp>

#include "mace.h"
#include "mace-macros.h"
#include "ContextJobApplication.h"
#include "TcpTransport-init.h"
#include "load_protocols.h"
using namespace boost::unit_test;
BOOST_AUTO_TEST_SUITE( standalone )
BOOST_AUTO_TEST_CASE(VirtualNode)
{
  load_protocols();
  mace::MaceKey vnode( mace::vnode, 1 );
  mace::ContextMapping::setVirtualNodeMaceKey( vnode );
  TransportServiceClass* tcp = &( ::TcpTransport_namespace::new_TcpTransport_Transport() );
  mace::MaceKey localAddress = tcp->localAddress();

  BOOST_REQUIRE( localAddress == vnode );
}
#include <sstream>
BOOST_AUTO_TEST_CASE(PhysicalAddress)
{
  load_protocols();
  mace::MaceKey physAddr(ipv4, Util::getMaceAddr() );
  TransportServiceClass* tcp = &( ::TcpTransport_namespace::new_TcpTransport_Transport() );
  mace::MaceKey localAddress = tcp->localAddress();

  std::ostringstream oss;
  oss<<"Util::getMaceAddr() returns "<< physAddr<<", tcp->localAddress() returns "<< localAddress;

  BOOST_TEST_MESSAGE( oss.str() );

  BOOST_REQUIRE( localAddress== physAddr );
}

BOOST_AUTO_TEST_SUITE_END()
#include "ContextJobApplication.h"
#include "../services/interfaces/NullServiceClass.h"
BOOST_AUTO_TEST_SUITE( scheduled )

BOOST_AUTO_TEST_CASE(ContextJobApplication)
{
  int argc = 3;
  const char* argv[] = {"cjtest", "-socket","cjtest.sock"};
  params::set("NUM_ASYNC_THREADS", "1");
  params::set("NUM_TRANSPORT_THREADS", "1");
  mace::Init(argc, const_cast<char**>(argv));
  load_protocols();
  mace::ContextJobApplication<NullServiceClass> app;

  mace::string service = "Simple";
  uint64_t runtime = 2;
  app.startService( service );
  TransportServiceClass* tcp = &( ::TcpTransport_namespace::new_TcpTransport_Transport() );
  app.waitService( runtime );
  MaceKey vn( mace::vnode, 1 );
  BOOST_REQUIRE( tcp->localAddress() == vn );
}
BOOST_AUTO_TEST_SUITE_END()


// TODO: create one process which serves as the server of the domain socket, and create another process as the client.
