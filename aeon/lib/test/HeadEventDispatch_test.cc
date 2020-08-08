#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE libmace
#include <boost/test/unit_test.hpp>
#include "HeadEventDispatch.h"

BOOST_AUTO_TEST_SUITE( lib_HeadEventDispatch )

int x = 0;
class Obj: public AsyncEventReceiver{
public:
  void startEvent(){
    HeadEventDispatch::HeadEventTP::executeEvent( this, (HeadEventDispatch::eventfunc)&Obj::handler, (mace::Message*)&x, false );
  }
  void handler(void* param){
    x++;
    mace::AgentLock al( mace::AgentLock::WRITE_MODE );
    al.downgrade( mace::AgentLock::READ_MODE );
    HeadEventDispatch::HeadEventTP::executeEvent( this, (HeadEventDispatch::eventfunc)&Obj::handler, (mace::Message*)&x, false );
    /*int& val = *((int*)param);
     ASSERT( val == 1 );
      val = 2;*/
  }
};
BOOST_AUTO_TEST_CASE( test1 )
{
  HeadEventDispatch::init();
  Obj o;
  o.startEvent();

  int last_x = 0;
  for(int t=0;t<10;t++){
    SysUtil::sleep(1);

    std::cout<<x<< " " << x - last_x<<std::endl;
    last_x = x;
  }
  HeadEventDispatch::haltAndWait();

  BOOST_REQUIRE_EQUAL( true, true );
}
BOOST_AUTO_TEST_SUITE_END()
