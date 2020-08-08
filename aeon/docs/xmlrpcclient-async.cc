#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <cassert>
#include <sstream>
#include <fstream>
#include <iostream>

#include <string>

#include "Util.h"
#include "StrUtil.h"
#include "SysUtil.h"

#include "MathServiceXmlRpcClient.h"
#include "HttpClient.h"
#include "params.h"

using namespace std;

string version = "1.1";
bool quiet = false;
bool profile = false;

class MSHandler : public MathServiceXmlRpcCallbackHandler {
public:
  void MathServiceAddResult(const XmlRpcResponseState<int>& ret,
			    void* cbParam) {
    try {
      int result = ret.getResponse();
      cout << result << endl;
    } catch(const XmlRpcClientException& e) {
      cout << "Error: " << e << endl;
    }
  }

  void MathServiceSumAndDifferenceResult(const XmlRpcResponseState<StringIntHMap>& ret,
					 void* cbParam) {
    try {
      StringIntHMap result = ret.getResponse();
      time_t* t = (time_t*)cbParam;
      cout << "request time=" << *t << endl;
      cout << result["sum"] << " " << result["difference"] << endl;
    } catch(const XmlRpcClientException& e) {
      cout << "Error: " << e << endl;
    }
  }
};

int main(int argc, char* argv[]) {
  int port = 6666;
  MSHandler handler;

  params::set("MACE_ADDRESS_ALLOW_LOOPBACK", "1");
  MathServiceXmlRpcClient c("localhost", port, "/xmlrpc");
  try {
    mace::deque<int> addv;
    for (int i = 1; i < argc; i++) {
      addv.push_back(atoi(argv[i]));
    }
    c.add(addv, &handler, (void*)0);

    time_t now = time(0);
    c.sumAndDifference(atoi(argv[1]), atoi(argv[2]), &handler, &now);
  } catch (const Exception& e) {
    cerr << e << endl;
  }

  SysUtil::sleep();
  return 0;
} // main

