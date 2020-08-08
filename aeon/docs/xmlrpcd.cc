#include <cstring>

#include "params.h"
#include "SysUtil.h"

#include "XmlRpcServer.h"
#include "XmlRpcUrlHandler.h"
#include "MathService.h"
#include "MathServiceXmlRpcHandler.h"

using namespace std;

XmlRpcServer* server;

void shutdownHandler(int sig) {
  server->shutdown();
  exit(0);
} // shutdownHandler

int main(int argc, char* argv[]) {
  if (argc != 2) {
    printf("usage: %s config\n", argv[0]);
    exit(1);
  }

  params::addRequired("listen", "port to listen for connections");
  params::loadfile(argv[1], true);

  int port = params::get<int>("listen");

  server = new XmlRpcServer("/xmlrpc", port);

  SysUtil::signal(SIGINT, &shutdownHandler);
  SysUtil::signal(SIGTERM, &shutdownHandler);

  MathService ms;
  server->registerHandler("MathService", new MathServiceXmlRpcHandler<MathService>(&ms));

  server->run();
  SysUtil::select();

  return 0;
} // main
