/* 
 * UnicastFileServiceManager.h : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, James W. Anderson, Ryan Braud, Charles Killian, John Fisher-Ogden, Calvin Hubble
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
#ifndef UNICAST_FILE_SERVICE_MANAGER_H
#define UNICAST_FILE_SERVICE_MANAGER_H

#include <boost/shared_ptr.hpp>
#include <pthread.h>
#include "ServiceManager.h"
#include "ReceiveDataHandler.h"
#include "TransportServiceClass.h"
#include "BufferedTransportServiceClass.h"
#include "BufferedBlockManager.h"
#include "NullBlockManager.h"
#include "services/Transport/TcpTransport-init.h"

class UnicastFileServiceManager : public ServiceManager, public ReceiveDataHandler,
				  public ConnectionStatusHandler {

  class unicast_file_control_header {
  public:
    size_t block_count;
  };

  class unicast_file_header {
  public:
    size_t sequence;
  };

public:
  UnicastFileServiceManager(BlockManager& bm,
			    BufferedTransportServiceClass& router = TcpTransport_namespace::new_TcpTransport_BufferedTransport(),
			    bool rts = false);
  virtual ~UnicastFileServiceManager();
  virtual void downloadFile();
  virtual void sendFile(const MaceKey& dest);
  virtual void sendFile();

  virtual void deliver(const MaceKey& source, const MaceKey& dest, const std::string &s, int serviceUid);
  virtual void joinOverlay();
  virtual void clearToSend(const MaceKey& dest, registration_uid_t rid);

private:
  void loadData();

// private:
//   void send(int dest, const std::string& msg);

private:
  BlockManager& blockManager;
  BufferedTransportServiceClass& router;
  SynchronousBufferedTransport syncTransport;
  bool rts;
  size_t numBlocks;
  size_t blocksReceived;
  size_t currentBlock;
  pthread_mutex_t lock;
  pthread_cond_t cond;
  uint64_t diskc;
  uint64_t routec;
  std::string data;
  size_t ctscount;

};

#endif // UNICAST_FILE_SERVICE_MANAGER_H
