/* 
 * BufferedBlockManager.h : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, Charles Killian, Dejan Kostic, Ryan Braud, James W. Anderson, John Fisher-Ogden, Calvin Hubble, Duy Nguyen, Justin Burke, David Oppenheimer, Amin Vahdat, Adolfo Rodriguez, Sooraj Bhat
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
#include <pthread.h>
#include <deque>
#include "LRUCache.h"
#include "BlockManager.h"
#include "FileBlockManager.h"

#ifndef BUFFERED_BLOCK_MANAGER_H
#define BUFFERED_BLOCK_MANAGER_H

typedef std::deque<int> BlockQueue; ///< type for storage of block prefetch requests

/**
 * \file BufferedBlockManager.h
 * \brief declares the BufferedBlockManager class
 */

/**
 * \addtogroup BlockManagers
 * @{
 */

/// The BufferedBlockManager provides the BlockManager, using a FileBlockManager.  It provides a thread for background loading of file blocks in response to explicit prefetchBlock() requests and in advance of the actual getBlock() requests for block data.
class BufferedBlockManager : public BlockManager {
public:
  static const uint DEFAULT_BUFFER_CAPACITY = 256; ///< buffer size in blocks
  BufferedBlockManager(FileBlockManager& m, uint capacity = DEFAULT_BUFFER_CAPACITY);
  virtual ~BufferedBlockManager();
  virtual void prefetchBlock(uint64_t index); 
  virtual int flush();
  virtual std::string getBlock(uint64_t index);
  virtual size_t setBlock(uint64_t index, const std::string& buffer);
  virtual int open(const std::string& p, const char* mode);
  virtual int close();
  virtual bool isOpen() const;
  virtual const std::string& getPath() const;
  virtual std::string getFileName() const;
  virtual size_t getBlockSize() const;
  virtual void setBlockSize(size_t s);
  virtual uint64_t getBlockCount() const {
    assert(isOpen());
    return fbm->getBlockCount();
  }
  virtual uint64_t getSize() const {
    assert(isOpen());
    return fbm->getSize();
  }
  void processQueues();

protected:
  static void* startQueueThread(void* t);


private:
  void writeLastDirty();
  void readIntoCache(uint index);
  void acquireQueueLock();
  void releaseQueueLock();
  void signalQueueData();
  void waitForQueueData();

private:
  FileBlockManager* fbm;
  BlockQueue readQueue;
  mace::LRUCache<uint, std::string> bufferCache;
  bool isOpenFlag;
  bool processQueuesFlag;
  pthread_t queueThread;
  pthread_mutex_t queueLock;
  pthread_cond_t queueCond;

  
}; // BlockManager

/** @} */

#endif // BUFFERED_BLOCK_MANAGER_H
