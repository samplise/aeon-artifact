#include "SysUtil.h"
#include "Util.h"
#include "PingServiceClass.h"
#include "Ping-init.h"
#include "TcpTransport-init.h"
#include "UdpTransport-init.h"

using namespace std;

class PingLatencyHandler : public PingDataHandler {
  void hostResponseReceived(const MaceKey& host, uint64_t timeSent, uint64_t timeReceived,
			    uint64_t remoteTime, registration_uid_t rid) {
    cout << host << " one-way latency is " << (remoteTime - timeSent) << " usec" << endl;
  } // hostResponseReceived
}; // PingLatencyHandler

class PingResponseHandler : public PingDataHandler {

  void hostResponseReceived(const MaceKey& host, uint64_t timeSent, uint64_t timeReceived,
			    uint64_t remoteTime, registration_uid_t rid) {
    cout << host << " responded in " << (timeReceived - timeSent) << " usec" << endl;
  } // hostResponseReceived

  void hostResponseMissed(const MaceKey& host, uint64_t timeSent, registration_uid_t rid) {
    const time_t t = timeSent / 1000000;
    cout << "did not receive response from " << host << " pinged at " << ctime(&t);
  } // hostResponseMissed

}; // PingResponseHandler

class ErrorHandler : public NetworkErrorHandler {
public:
  void error(const MaceKey& k, TransportError::type error, const std::string& m,
            registration_uid_t rid) {
    cerr << "received error " << error << " for " << k << ": " << m << endl;
  }
}; // ErrorHandler

int main(int argc, char* argv[]) {
//   Log::autoAdd("Ping::");
  Log::autoAddAll();

  if (argc < 2 || !FileUtil::fileExists(argv[1])) {
    cerr << "usage: " << argv[0] << " config-file" << endl;
    exit(-1);
  }
  params::loadfile(argv[1], true);

  PingResponseHandler prh;
  PingLatencyHandler plh;
  ErrorHandler eh;

//   TransportServiceClass& router = UdpTransport_namespace::new_UdpTransport_Transport();
  TransportServiceClass& router = TcpTransport_namespace::new_TcpTransport_Transport(TransportCryptoServiceClass::TLS);
  router.registerHandler(eh);

  PingServiceClass& ping = Ping_namespace::new_Ping_Ping(router, 1*1000*1000);
  ping.maceInit();
  registration_uid_t rid = ping.registerHandler(prh);
  registration_uid_t lrid = ping.registerHandler(plh);

  for (int i = 2; i < argc; i++) {
    ping.monitor(MaceKey(ipv4, argv[i]), rid);
    ping.monitor(MaceKey(ipv4, argv[i]), lrid);
  }

  SysUtil::sleep();

  return 0;
} // main
