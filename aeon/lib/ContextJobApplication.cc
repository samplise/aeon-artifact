#include "HeadEventDispatch.h"
#include "ContextService.h"


namespace mace{
  void sampleLatency(bool flag){
    HeadEventDispatch::sampleLatency(true);
  }

  double getAverageLatency(){
    return HeadEventDispatch::getAverageLatency();
  }
  void finishHeadThread(bool isHead){
    HeadEventDispatch::haltAndWait();
    if( isHead ){
      // if this is the node, the commit thread must wait until the ENDEVENT commits
      HeadEventDispatch::haltAndWaitCommit();
    }else{
      HeadEventDispatch::haltAndNoWaitCommit();
    }
  }
/*
  void loadOwnerships2( const mace::vector< mace::pair<mace::string, mace::string> >& ownerships) {
    ContextStructure::setInitialContextOwnerships(ownerships);   
  }
*/
}
