/* 
 * filecopy.cc : part of the Mace toolkit for building distributed systems
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
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <iostream>
#include <string>

#include "Accumulator.h"
#include "LoadMonitor.h"
#include "lib/Scheduler.h"
#include "lib/params.h"
#include "lib/Log.h"
#include "application/common/UnicastFileServiceManager.h"
#include "BufferedTransportServiceClass.h"
#include "services/Transport/TcpTransport-init.h"
#include "Accumulator.h"

static const size_t NULL_SIZE = 256*1024*1024 / BlockManager::DEFAULT_BLOCK_SIZE;
// static const size_t NULL_SIZE = 4*512*1024*1024 / BlockManager::DEFAULT_BLOCK_SIZE;
static const size_t MAX_QUEUE_SIZE = 16*1024*1024;
static const bool RTS = false;

using namespace std;

int main(int argc, char* argv[]) {
  Log::autoAddAll();
  params::set("MACE_ADDRESS_ALLOW_LOOPBACK", "1");
//   HiResLoadMonitor::start();
//   Log::setLevel(3);

//   params::addRequired("port", "int");
//   params::addRequired("blocksize", "int");
  params::addRequired("file", "path");
  params::loadparams(argc, argv);

  string path = params::get<std::string>("file");

  if (params::containsKey("dest")) {
    struct stat sbuf;
    if (stat(path.c_str(), &sbuf) != 0) {
      perror("stat");
      return -1;
    }
  }

  Log::configure();
  Log::add("UnicastFileServiceManager::sendFile", stdout, LOG_TIMESTAMP_HUMAN);
//   Log::add("Accumulator::dumpAll");
//   Log::logToFile("/tmp/tcpwrite.log", Accumulator::TCP_WRITE_SELECTOR);
//   Log::logToFile("/tmp/send.log", Accumulator::TRANSPORT_SEND_SELECTOR);
//   Log::logToFile("/tmp/times.log", "times");
//   Accumulator::startLogging(100*1000);
  Accumulator::startLogging();

  FileBlockManager bm;
//   NullBlockManager bm(NULL_SIZE);

  BufferedTransportServiceClass& router =
    TcpTransport_namespace::new_TcpTransport_BufferedTransport(TransportCryptoServiceClass::NONE, false,
							   MAX_QUEUE_SIZE);

  UnicastFileServiceManager fsm(bm, router, RTS);
  if (params::containsKey("dest")) {
    bm.open(path, "rb");
    fsm.sendFile();
  }
  else {
    bm.open(path, "wb");
    fsm.downloadFile();
  }
  Scheduler::haltScheduler();
  Accumulator::dumpAll();
  return 0;
} // main
