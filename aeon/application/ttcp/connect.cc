/* 
 * connect.cc : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, James W. Anderson
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the names of the contributors, nor their associated universities 
 *      or organizations may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * ----END-OF-LEGAL-STUFF---- */
#include "params.h"
#include "TcpTransport.h"
#include "TcpTransport-init.h"
#include "SysUtil.h"
#include "TimeUtil.h"

using namespace std;

BufferedTransportServiceClass* transport;

class TransportCallback : public ReceiveDataHandler, public NetworkErrorHandler {
public:
  void error(const MaceKey& id, TransportError::type code, const string& message,
	     registration_uid_t rid) {
    cout << "received error " << message << endl;
  }

  void deliver(const MaceKey& source, const MaceKey& destination,
	       const std::string& s, registration_uid_t rid) {
    cout << "read " << s.size() << endl;
  }
  
}; // TransportCallback

TransportCallback tcb;

int main(int argc, char** argv) {
//   params::set("MACE_LOCAL_ADDRESS", macelocal);
  params::set("MACE_BIND_LOCAL_ADDRESS", "1");
  params::set("MACE_ADDRESS_ALLOW_LOOPBACK", "1");
  params::set("MACE_WARN_LOOPBACK", "0");

  Log::autoAddAll();
  Log::setLevel(1);
  params::loadparams(argc, argv);

  transport = &TcpTransport_namespace::new_TcpTransport_BufferedTransport();
  transport->maceInit();
  transport->registerUniqueHandler((ReceiveDataHandler&)tcb);
  transport->registerUniqueHandler((NetworkErrorHandler&)tcb);

  int port = 10000;
  int count = 1026;
//   int count = 1;
  for (int i = port; i < port + count; i++) {
    MaceKey dest(ipv4, "127.0.0.1:" + StrUtil::toString(i));
    cout << "sending to " << dest << endl;
    transport->send(dest, string());
  }

  SysUtil::sleep();

  return 0;
}
