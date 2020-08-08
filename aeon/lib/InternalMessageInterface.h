#ifndef _INTERNAL_MESSAGE_INTERFACE_H_
#define _INTERNAL_MESSAGE_INTERFACE_H_

#include "MaceKey.h"
#include "Message.h"

namespace mace{
class InternalMessageSender {
public:
  virtual void sendInternalMessage( mace::MaceAddr const& dest, mace::Message const& message ) = 0;
};

class InternalMessageReceiver {
public:
  virtual void handleInternalMessages( mace::InternalMessage const& message, MaceAddr const& src, uint64_t size  ) = 0;
};

}
#endif
