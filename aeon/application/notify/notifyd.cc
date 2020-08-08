/* 
 * notifyd.cc : part of the Mace toolkit for building distributed systems
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

/**
 * Comment added by Chip Killian:
 *
 * Simple distributed notification daemon written by James Anderson.  The
 * client connects to the daemon, causing a multicast to be sent to all
 * participants, performing the action.  
 */
#include <cstring>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>

#include "params.h"
#include "SysUtil.h"

#include "BaseTransport.h"
#include "XmlRpcServer.h"
#include "XmlRpcUrlHandler.h"
#include "MulticastServiceClass.h"
#include "MulticastServiceClassXmlRpcHandler.h"
#include "GenericTreeMulticast-init.h"
#include "TcpTransport-init.h"
#include "RouteTransportWrapper-init.h"
#include "RandTree-init.h"

using namespace std;
using namespace GenericTreeMulticast_namespace;
using namespace TcpTransport_namespace;
using namespace RouteTransportWrapper_namespace;
using namespace RandTree_namespace;

class NotifyHandler : public ReceiveDataHandler {
public:
  void deliver(const MaceKey& src, const MaceKey& dest, const std::string& s,
	       registration_uid_t rid) {
    ADD_SELECTORS("NotifyHandler::deliver");

    if (params::containsKey(dest.addressString(false))) {
      play(params::get<string>(dest.addressString(false)));
    }
    else {
      macewarn << "received unknown dest " << dest
	       << " addr=" << dest.addressString(false) << Log::endl;
    }
  }

protected:
  void play(const string& path) {
    ADD_SELECTORS("notifyd::play");
    static string playcmd = params::get<string>("player");
    int pid = 0;
    if ((pid = fork()) < 0) {
      Log::perror("fork");
      assert(0);
    }

    if (pid == 0) {
      close(fileno(stdin));
      close(fileno(stdout));
      close(fileno(stderr));
      // child
      if (execl(playcmd.c_str(), playcmd.c_str(), path.c_str(), NULL) < 0) {
	Log::err() << "could not play: " << playcmd << " " << path << Log::endl;
	Log::perror("execl");
	assert(0);
      }
    }
    int status;
    int r = waitpid(pid, &status, 0);
    if (r != pid) {
      Log::perror("waitpid");
      assert(0);
    }
    if (WEXITSTATUS(status) != 0) {
      Log::err() << "child exited with " << WEXITSTATUS(status) << Log::endl;
      assert(0);
    }
    maceout << "joined " << pid << " which exited with " << WEXITSTATUS(status)
	    << Log::endl;
  } // play

}; // NotifyHandler

XmlRpcServer* server;
MulticastServiceClass* ms;

void shutdownHandler(int sig) {
  ms->maceExit();
  server->shutdown();
  Scheduler::haltScheduler();
  exit(0);
} // shutdownHandler

int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf("usage: %s config\n", argv[0]);
    exit(1);
  }

  params::addRequired("MACE_AUTO_BOOTSTRAP_PEERS", "hosts to peer with");
  params::addRequired("MACE_PRIVATE_KEY_FILE", "private key file");
  params::addRequired("MACE_CERT_FILE", "certificate file");
  params::addRequired("MACE_CA_FILE", "CA file");
  params::addRequired("listen", "port to listen for connections");
  params::addRequired("player", "command to play sounds");
  params::addRequired("pri1", "sound for priority 1 notification");
  params::addRequired("pri2", "sound for priority 2 notification");
//   params::loadfile(argv[1], true);
  params::loadparams(argc, argv, true);

  Log::configure();

  if (params::get("daemon", false)) {
    if (SysUtil::daemon(0, 0) < 0) {
      Log::perror("daemon");
      exit(-1);
    }
  }

  server = new XmlRpcServer("/xmlrpc", params::get<uint16_t>("listen"));

  SysUtil::signal(SIGINT, &shutdownHandler);
  SysUtil::signal(SIGTERM, &shutdownHandler);

  TransportServiceClass& transport = new_TcpTransport_Transport(
    TransportCryptoServiceClass::TLS);
  RouteServiceClass& router = new_RouteTransportWrapper_Route(transport);
  TreeServiceClass& tree = new_RandTree_Tree(transport);
  ms = &new_GenericTreeMulticast_Multicast(router, tree);
  try {
    ms->maceInit();
  }
  catch (const BaseTransport::SocketException& e) {
    cerr << "SocketException " << e << endl;
    exit(-1);
  }

  NotifyHandler nh;
  ms->registerUniqueHandler(nh);
  server->registerHandler("MulticastServiceClass",
			  new MulticastServiceClassXmlRpcHandler<MulticastServiceClass>(ms));

  server->run();

  SysUtil::select();
  return 0;
} // main
