#include "SysUtil.h"
#include "Util.h"
#include <signal.h>
#include "Scheduler.h"

#include "ServiceFactory.h"
#include "PingServiceClass.h"
#include "load_protocols.h"

using namespace std;

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

static bool shutdownPing = false;

void shutdownHandler(int sig) {
  shutdownPing = true;
} // shutdownHandler

int main(int argc, char* argv[]) {
  Log::autoAdd(".*Ping::.*");
  //   Log::autoAddAll();
  //   Log::setLevel(2);

  SysUtil::signal(SIGINT, &shutdownHandler);
  SysUtil::signal(SIGTERM, &shutdownHandler);

  params::loadparams(argc, argv);
  Log::configure();

  load_protocols();

  PingResponseHandler prh;

  std::string pingService = params::get<std::string>("ping_service", "Ping");

  mace::ServiceFactory<PingServiceClass>::print(stdout);
  PingServiceClass& ping = mace::ServiceFactory<PingServiceClass>::create(pingService, true);

  ping.maceInit();
  registration_uid_t rid = ping.registerHandler(prh);

  for (int i = 1; i < argc; i++) {
    ping.monitor(MaceKey(ipv4, argv[i]), rid);
  }

  while (!shutdownPing) {
    SysUtil::sleepm(500);
  }

  ping.maceExit();
  Scheduler::haltScheduler();
  return 0;
} // main
