/* 
 * dhtd.cc : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, Charles Killian, James W. Anderson
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
#include <stdint.h>
#include "SysUtil.h"
#include <signal.h>
#include "Scheduler.h"
#include "XmlRpcServer.h"
#include "XmlRpcUrlHandler.h"
#include "StringDHT.h"
#include "StringDHTXmlRpcHandler.h"

using namespace std;

XmlRpcServer* server;
StringDHT* dht;
bool halt = false;

void shutdownHandler(int sig) {
  halt = true;
} // shutdownHandler

int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf("usage: %s config\n", argv[0]);
    exit(1);
  }

  params::addRequired("listen", "port to listen for connections");
  params::addRequired("MACE_AUTO_BOOTSTRAP_PEERS", "overlay host(s) to join");
  params::loadparams(argc, argv);

  int port = params::get<int>("listen");

  Log::configure();
  //Log::autoAddAll();
  Log::autoAdd("DHT");

  try {
    server = new XmlRpcServer("/xmlrpc", port);

    SysUtil::signal(SIGINT, &shutdownHandler);
    SysUtil::signal(SIGTERM, &shutdownHandler);

    dht = new StringDHT();
    server->registerHandler("StringDHT", new StringDHTXmlRpcHandler<StringDHT>(dht));

    server->run();
    while (!halt) {
      SysUtil::sleepm(100);
    }
  } catch (const Exception& e) {
    cerr << e << endl;
    assert(0);
  }
  
  server->shutdown();
  dht->shutdown();
  Scheduler::haltScheduler();

  return 0;
} // main
