#include "TransportServiceClass.h"
#include "InternalMessageInterface.h"
#include "NumberGen.h"
#include "TcpTransport-init.h"

namespace mace{
class InternalMessageProcessor: public InternalMessageSender, public virtual ReceiveDataHandler, public  virtual NetworkErrorHandler{
private:
  InternalMessageReceiver* receiver;
  TransportServiceClass& transport;
  registration_uid_t __ctx;  
public:
  InternalMessageProcessor( 
    InternalMessageReceiver* receiver, 
    TransportServiceClass& transport  = TcpTransport_namespace::new_TcpTransport_Transport()
  ): receiver( receiver ), transport( transport )
  {
  }

  void initChannel(){
    transport.maceInit();

    if (__ctx == -1) {
      __ctx  = NumberGen::Instance(NumberGen::HANDLER_UID)->GetVal();
    }

    transport.registerHandler((ReceiveDataHandler&)*this, __ctx, false);
    transport.registerHandler((NetworkErrorHandler&)*this, __ctx, false);
  }

  void exitChannel(){
    transport.unregisterHandler((ReceiveDataHandler&)*this, __ctx);
    transport.unregisterHandler((NetworkErrorHandler&)*this, __ctx);
    transport.maceExit();
  }

  void deliver( const MaceKey& source, const MaceKey& destination, const std::string& s, registration_uid_t rid ){
    ADD_SELECTORS("InternalMessageProcessor::deliver");

    mace::InternalMessage s_deserialized;
    s_deserialized.deserializeStr(s);

    macedbg(1)<<"source="<< source <<", destination="<< destination << ", message="<< s_deserialized << ", rid="<< rid << Log::endl;

    receiver->handleInternalMessages( s_deserialized, source.getMaceAddr(), s.length() );
  }
  
  void messageError( const MaceKey& dest, TransportError::type errorCode, const std::string& message, registration_uid_t rid = -1 ){
    ADD_SELECTORS("InternalMessageProcessor::messageError");
    macedbg(1)<<"dest="<< dest << ", errorCode="<< errorCode << ", message="<< message << ", rid="<< rid << Log::endl;
  }

  void error( const MaceKey& nodeId, TransportError::type errorCode, const std::string& message = "", registration_uid_t rid = -1 ){
    ADD_SELECTORS("InternalMessageProcessor::error");
    macedbg(1)<<"nodeId="<< nodeId << ", errorCode="<< errorCode << ", message="<< message << ", rid="<< rid << Log::endl;
  }

  void sendInternalMessage( mace::MaceAddr const& destNode, Message const& message ){
    ADD_SELECTORS("InternalMessageProcessor::sendInternalMessage");
    macedbg(1)<< "destNode="<<destNode<<",message="<< message << Log::endl;

    const MaceKey dest( mace::ctxnode, destNode  );
    std::string s;
    ScopedSerialize< std::string, mace::Message > ss( s, message );
    transport.route( dest, s, __ctx );
  }

};
}
