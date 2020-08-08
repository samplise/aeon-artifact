/* 
 * cgid.cc : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, James W. Anderson, Ryan Braud
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
#include "maceConfig.h"
#ifdef INCLUDE_CGI
#include <cstring>

#include "params.h"
#include "SysUtil.h"
#include "Scheduler.h"
#include <signal.h>

#include "CgiUrlHandler.h"
#include "HttpServer.h"

using namespace std;

HttpServer* server; 

void shutdownHandler(int sig) {
  server->shutdown();
  exit(0);
} // shutdownHandler

int main(int argc, char* argv[]) {
  if (argc != 2) {
    printf("usage: %s config\n", argv[0]);
    exit(1);
  }

  params::addRequired("DocumentRoot", "path to document root");
  params::addRequired("listen", "port to listen for connections");
  params::loadfile(argv[1], true);

  Log::configure();

  string root = params::get<std::string>("DocumentRoot");
  uint16_t port = params::get<uint16_t>("listen");

  server = new HttpServer(port);

  SysUtil::signal(SIGINT, &shutdownHandler);
  SysUtil::signal(SIGTERM, &shutdownHandler);

  CgiUrlHandler cgih(root);
  server->registerUrlHandler("/", &cgih);
  try {
    server->run();
  } catch (const Exception& e) {
    cerr << e << endl;
    assert(0);
  }
  SysUtil::select();

  Scheduler::haltScheduler();

  return 0;
} // main
#endif
