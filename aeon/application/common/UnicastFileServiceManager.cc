/* 
 * UnicastFileServiceManager.cc : part of the Mace toolkit for building distributed systems
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
#include <sched.h>
#include <boost/format.hpp>

#include "Accumulator.h"
#include "Log.h"
// #include "params.h"
#include "UnicastFileServiceManager.h"

using namespace std;

UnicastFileServiceManager::UnicastFileServiceManager(BlockManager& bm,
						     BufferedTransportServiceClass& router,
						     bool rts) :
  blockManager(bm),
  router(router),
  syncTransport(router, (ConnectionStatusHandler&)*this),
  rts(rts),
  numBlocks(0),
  blocksReceived(0),
  currentBlock(0),
  diskc(0),
  routec(0),
  ctscount(0)
{
  router.maceInit();
  router.registerUniqueHandler((ReceiveDataHandler&)*this);
  router.registerUniqueHandler((ConnectionStatusHandler&)*this);

  pthread_mutex_init(&lock, 0);
  pthread_cond_init(&cond, 0);
} // UnicastFileServiceManager

UnicastFileServiceManager::~UnicastFileServiceManager() {
  pthread_cond_destroy(&cond);
  pthread_mutex_destroy(&lock);
} // ~UnicastFileServiceManager

void UnicastFileServiceManager::joinOverlay() {
} // joinOverlay

void UnicastFileServiceManager::sendFile(const MaceKey& dest) {
  ADD_SELECTORS("UnicastFileServiceManager::sendFile");
  Accumulator::Instance(Accumulator::NETWORK_WRITE)->startTimer();

//   maceout << "sending file to " << dest << Log::endl;

  // prefetch some blocks
//   uint prefetchCount = 128;
//   if (prefetchCount > bm.getBlockCount()) {
//     prefetchCount = bm.getBlockCount();
//   }
//   Log::logf("UnicastFileServiceManager::sendFile",
// 	    "prefetchCount=%d blockCount=%d\n",
// 	    prefetchCount, bm.getBlockCount());
//   for (uint i = 0; i < prefetchCount; i++) {
//     Log::logf("UnicastFileServiceManager::sendFile", 2,
// 	      "prefetching block %d\n", i);
//     bm.prefetchBlock(i);
//   }

  unicast_file_control_header controlHeader;
  controlHeader.block_count = blockManager.getBlockCount();
  numBlocks = controlHeader.block_count;
  data.clear();
  data.append((char*)&controlHeader, sizeof(unicast_file_control_header));
  maceLog(1, "sending control header with block_count=%zu, data.size=%zu\n",
	  controlHeader.block_count, data.size());

  while (!router.route(dest, data));

  data.clear();
  if (rts) {
    currentBlock = 0;
    ASSERT(pthread_mutex_lock(&lock) == 0);
    loadData();
    router.requestToSend(dest);
    ASSERT(pthread_cond_wait(&cond, &lock) == 0);
    ASSERT(pthread_mutex_unlock(&lock) == 0);
  }
  else {
    while (currentBlock < numBlocks) {
      loadData();
      maceLog(2, "sending sequence %zu of with %zu bytes of data, length=%zu\n",
	      currentBlock, blockManager.getBlockSize(), data.size());
      while (!router.route(dest, data));
    }
  }
  
  blockManager.close();

  maceout << "wrote all blocks, flushing..." << endl;

  syncTransport.flush();

  maceout << "flushed, exiting..." << endl;
  router.maceExit();

  uint64_t start = Accumulator::Instance(Accumulator::NETWORK_WRITE)->getStartTime();
  uint64_t end = Accumulator::Instance(Accumulator::NETWORK_WRITE)->getLastTime();
  uint64_t byteCount = Accumulator::Instance(Accumulator::NETWORK_WRITE)->getAmount();
  uint64_t t = end - start;
  double bw = 0;
  string unit = "B";
  if (t > 0) {
    bw = ((double)byteCount / t) * 1000 * 1000;
    if (bw > 1024) {
      unit = "KB";
      bw /= 1024;
    }
    if (bw > 1024) {
      unit = "MB";
      bw /= 1024;
    }
  }

  maceout
    << "connection open for " << (t / 1000) << " ms "
    << byteCount << " bytes " << boost::format("%.3f") % bw << " " << unit << "/sec"
    << " cts count=" << ctscount << " diskc=" << diskc << " routec=" << routec
    << Log::endl;
} // sendFile

void UnicastFileServiceManager::sendFile() {
  MaceKey dest(ipv4, params::get<string>("dest"));

  sendFile(dest);
} // sendFile

void UnicastFileServiceManager::downloadFile() {
//   cerr << "downloadFile waiting for signal" << endl;
  ASSERT(pthread_mutex_lock(&lock) == 0);
  ASSERT(pthread_cond_wait(&cond, &lock) == 0);
  ASSERT(pthread_mutex_unlock(&lock) == 0);

//   cerr << "downloadFile received signal" << endl;

  blockManager.close();
  router.maceExit();
} // downloadFile

void UnicastFileServiceManager::deliver(const MaceKey& source, const MaceKey& dest,
					const string &s, int serviceUid) {
  ADD_SELECTORS("UnicastFileServiceManager::receiveData");
  if (numBlocks == 0) {
    unicast_file_control_header* h = (unicast_file_control_header*)s.data();
    numBlocks = h->block_count;
    maceLog(2, "received control header, numBlocks=%zu\n", numBlocks);
    return;
  }
  
  unicast_file_header* h = (unicast_file_header*)s.data();
  data.clear();
  data.append(s.data() + sizeof(unicast_file_header),
      s.size() - sizeof(unicast_file_header));
  maceLog(2, "received block %zu of size %zu bytes\n", h->sequence, data.size());
  blockManager.setBlock(h->sequence, data);
  blocksReceived++;

  if (blocksReceived == numBlocks) {
    ASSERT(pthread_mutex_lock(&lock) == 0);
    ASSERT(pthread_cond_signal(&cond) == 0);
    ASSERT(pthread_mutex_unlock(&lock) == 0);
  }
} // receiveData

void UnicastFileServiceManager::clearToSend(const MaceKey& dest, registration_uid_t rid) {
  ADD_SELECTORS("UnicastFileServiceManager::clearToSend");
  uint64_t now = TimeUtil::timeu();
  ctscount++;
//   maceout << "count=" << ctscount << " currentBlock=" << currentBlock << Log::endl;
  while (router.routeRTS(dest, data, rid)) {
    uint64_t tmp = TimeUtil::timeu();
    routec += tmp - now;
    now = tmp;
    if (currentBlock == numBlocks) {
      ASSERT(pthread_mutex_lock(&lock) == 0);
      ASSERT(pthread_cond_signal(&cond) == 0);
      ASSERT(pthread_mutex_unlock(&lock) == 0);
      maceout << ctscount << " callbacks" << Log::endl;
      return;
    }
    loadData();
  } 
} // clearToSend

void UnicastFileServiceManager::loadData() {
  uint64_t now = TimeUtil::timeu();
  data.clear();
  data.reserve(blockManager.getBlockSize() + sizeof(unicast_file_header));
  unicast_file_header h;
  h.sequence = currentBlock;
  data.append((char*)&h, sizeof(unicast_file_header));
  string block = blockManager.getBlock(currentBlock);
  data.append(block);
  diskc += (TimeUtil::timeu() - now);
  currentBlock++;
} // loadData
