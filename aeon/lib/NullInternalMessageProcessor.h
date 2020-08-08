#ifndef _NULL_INTERNAL_MESSAGE_H_
#define _NULL_INTERNAL_MESSAGE_H_

#include "InternalMessageInterface.h"
namespace mace{
class NullInternalMessageProcessor: public InternalMessageSender{
public:
  NullInternalMessageProcessor( InternalMessageReceiver* receiver =NULL){}
  void sendInternalMessage( mace::MaceAddr const& dest, mace::Message const& message ){
    ABORT("Null internal message processor does not send message!");
  }
  void initChannel(){}
  void exitChannel(){}
};
}
#endif
