#include "SysUtil.h"
#include "Util.h"
#include "ServiceFactory.h"
#include "PingServiceClass.h"

#include "load_protocols.h"


using namespace std;

class PingResponseHandler : public PingDataHandler {

  void hostResponseReceived(const MaceKey& host, uint64_t timeSent, uint64_t timeReceived,
			    uint64_t remoteTime, registration_uid_t rid) {
    cout << host << " responded in " << (timeReceived - timeSent) << " usec" << endl;
    exit(0);
  } // hostResponseReceived

  void hostResponseMissed(const MaceKey& host, uint64_t timeSent, registration_uid_t rid) {
    const time_t t = timeSent / 1000000;
    cout << "did not receive response from " << host << " pinged at " << ctime(&t);
    exit(0);
  } // hostResponseMissed

}; // PingResponseHandler

int main(int argc, char* argv[]) {
//   Log::autoAdd("Ping::");

  params::loadparams(argc, argv);
  Log::configure();
  load_protocols();

  PingResponseHandler prh;

  std::string pingService = params::get<std::string>("ping_service", "FirstPing");

  mace::ServiceFactory<PingServiceClass>::print(stdout);
  PingServiceClass& ping = mace::ServiceFactory<PingServiceClass>::create(pingService, true);

  ping.maceInit();
  ping.registerUniqueHandler(prh);
  if (argc > 1) {
    ping.monitor(MaceKey(ipv4, argv[1]));
  }

  SysUtil::sleep();

  return 0;
} // main
