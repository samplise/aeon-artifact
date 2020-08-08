#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE libmace
#include <boost/test/unit_test.hpp>

#include "mvector.h"
#include "MaceKey.h"
#include "Util.h"
#include "ContextMapping.h"
#include "params.h"
using namespace boost::unit_test;

BOOST_AUTO_TEST_CASE(TestVector)
{
  mace::vector<int> v;
  v.push_back(1);
  v.push_back(2);
  BOOST_REQUIRE_EQUAL(v.size(), (size_t)2);

  std::string s = v.serializeStr();
  std::string t = s;

  mace::vector<int> w;
  w.deserializeStr(t);

  BOOST_REQUIRE_EQUAL(v.toString(), w.toString());
  BOOST_REQUIRE(v == w);
}

BOOST_AUTO_TEST_SUITE( TestMaceKey )
BOOST_AUTO_TEST_CASE(BasicMaceKeyTest)
{
  Log::disableDefaultWarning();
  params::set("MACE_ADDRESS_ALLOW_LOOPBACK", "1");
  mace::MaceKey j = mace::MaceKey(mace::ipv4, "23.24.25.26");
  BOOST_REQUIRE(! j.isNullAddress());

  BOOST_TEST_CHECKPOINT("The following tests if DNS is working in Mace");
  mace::MaceKey k = mace::MaceKey(mace::ipv4, "www.google.com");
  BOOST_WARN(! k.isNullAddress());

  BOOST_TEST_CHECKPOINT("Now testing serialization of MaceKey");
  mace::MaceKey l = mace::MaceKey(mace::ipv4, "127.0.0.1:1024");
  std::string l_s = l.serializeStr();
  uint8_t expected_s[] = {1 /* IPV4 */, 127, 0, 0, 1, 4, 0, 255, 255, 255, 255, 0, 0};
  for (size_t i = 0; i < sizeof(expected_s); i++) {
    BOOST_REQUIRE_EQUAL( static_cast<uint8_t>(l_s[i]), expected_s[i]);
  }
}
BOOST_AUTO_TEST_CASE(MaceKey_vnode)
{ 
  mace::MaceAddr addr1 = Util::getMaceAddr("cloud01.cs.purdue.edu:5000");
  mace::MaceAddr addr2 = Util::getMaceAddr("cloud02.cs.purdue.edu:6000");
  mace::MaceAddr addr3 = Util::getMaceAddr("cloud03.cs.purdue.edu:7000");
  mace::ContextMapping::updateVirtualNodes( 1, addr1 );
  mace::ContextMapping::updateVirtualNodes( 2, addr2 );
  mace::ContextMapping::updateVirtualNodes( 3, addr3 );

  mace::MaceKey vnode1( mace::vnode, 1 );
  mace::MaceKey vnode2( mace::vnode, 2 );
  mace::MaceKey vnode3( mace::vnode, 3 );

  BOOST_REQUIRE(! vnode1.isNullAddress());
  BOOST_REQUIRE(! vnode2.isNullAddress());
  BOOST_REQUIRE(! vnode3.isNullAddress());

  BOOST_REQUIRE( vnode1.getMaceAddr() == addr1 );
  BOOST_REQUIRE( vnode2.getMaceAddr() == addr2 );
  BOOST_REQUIRE( vnode3.getMaceAddr() == addr3 );

  BOOST_REQUIRE( vnode1.getMaceAddr() != addr2 );
  BOOST_REQUIRE( vnode1.getMaceAddr() != addr3 );

  BOOST_REQUIRE( vnode2.getMaceAddr() != addr1 );
  BOOST_REQUIRE( vnode2.getMaceAddr() != addr3 );

  BOOST_REQUIRE( vnode3.getMaceAddr() != addr1 );
  BOOST_REQUIRE( vnode3.getMaceAddr() != addr2 );

  mace::MaceAddr addr4 = Util::getMaceAddr("cloud04.cs.purdue.edu:8000");
  mace::ContextMapping::updateVirtualNodes( 1, addr4 );
  BOOST_REQUIRE(! vnode1.isNullAddress());
  BOOST_REQUIRE( vnode1.getMaceAddr() != addr1 );
  BOOST_REQUIRE( vnode1.getMaceAddr() == addr4 );


  mace::string s;
  mace::serialize( s, &vnode1 );
  mace::MaceKey k;
  mace::deserialize( s, &k );
  BOOST_REQUIRE( k == vnode1 );
}
BOOST_AUTO_TEST_CASE(MaceKey_ctxnode)
{
  mace::MaceAddr addr1 = Util::getMaceAddr("cloud01.cs.purdue.edu:5000");
  mace::MaceAddr addr2 = Util::getMaceAddr("cloud02.cs.purdue.edu:6000");
  mace::MaceAddr addr3 = Util::getMaceAddr("cloud03.cs.purdue.edu:7000");

  mace::MaceKey node1( mace::ctxnode, addr1 );
  mace::MaceKey node2( mace::ctxnode, addr2 );
  mace::MaceKey node3( mace::ctxnode, addr3 );

  BOOST_REQUIRE(! node1.isNullAddress());
  BOOST_REQUIRE(! node2.isNullAddress());
  BOOST_REQUIRE(! node3.isNullAddress());

  BOOST_REQUIRE( node1.getMaceAddr() == addr1 );
  BOOST_REQUIRE( node2.getMaceAddr() == addr2 );
  BOOST_REQUIRE( node3.getMaceAddr() == addr3 );

  BOOST_REQUIRE( node1.getMaceAddr() != addr2 );
  BOOST_REQUIRE( node1.getMaceAddr() != addr3 );

  BOOST_REQUIRE( node2.getMaceAddr() != addr1 );
  BOOST_REQUIRE( node2.getMaceAddr() != addr3 );

  BOOST_REQUIRE( node3.getMaceAddr() != addr1 );
  BOOST_REQUIRE( node3.getMaceAddr() != addr2 );

  mace::string s;
  mace::serialize( s, &node1 );
  mace::MaceKey k;
  mace::deserialize( s, &k );
  BOOST_REQUIRE( k == node1 );
}
BOOST_AUTO_TEST_SUITE_END()
#include <iostream>
BOOST_AUTO_TEST_SUITE( s1 )
BOOST_AUTO_TEST_CASE(XTest)
{
    //std::cout<< boost::unit_test::framework::master_test_suite().argc << std::endl;
    int x = 1;
    BOOST_TEST_MESSAGE("message");
    BOOST_WARN( x == 2 );
    //BOOST_TEST_CHECKPOINT("The following tests if DNS is working in Mace");
    //BOOST_REQUIRE_EQUAL( x , 2 );

}
BOOST_AUTO_TEST_SUITE_END()

/*test_suite*
init_unit_test_suite( int argc, char* argv[] ) 
{
    framework::master_test_suite().p_name.value = "my master test suite name";

    return 0;
}*/
