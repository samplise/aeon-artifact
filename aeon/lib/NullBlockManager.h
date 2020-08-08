/* 
 * NullBlockManager.h : part of the Mace toolkit for building distributed systems
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
#ifndef NULL_BLOCK_MANAGER_H
#define NULL_BLOCK_MANAGER_H

#include "FileBlockManager.h"

/**
 * \file NullBlockManager.h
 * \brief declares the NullBlockManager class
 */

/**
 * \addtogroup BlockManagers
 * @{
 */

/// The NullBlockManager by implementation does nothing.  Used for performance testing.
class NullBlockManager : public FileBlockManager {
public:
//   NullBlockManager(size_t blockCount, size_t blockSize = DEFAULT_BLOCK_SIZE) :
//     blockSize(blockSize), blockCount(blockCount), size(blockSize * blockCount) { }
  NullBlockManager(uint64_t blockCount, size_t blockSize = DEFAULT_BLOCK_SIZE) {
    this->blockSize = blockSize;
    this->blockCount = blockCount;
    this->size = blockSize * blockCount;
  }
  virtual ~NullBlockManager() { }
  std::string getBlock(uint64_t index) {
    static std::string r(blockSize, 0);
    static bool init = false;
    if (!init) {
      init = true;
      for (size_t i = 0; i < blockSize; i++) {
	r[i] = (i&0x7F);
      }
    }

    return r;
  }
//   const std::string& getBlockRef(uint index) {
//     static std::string r(blockSize, 0);
//     return r;
//   }
  size_t setBlock(uint64_t index, const std::string& buffer) { return buffer.size(); }
  int open(const std::string& path, const char* mode) { return 0; }
  bool isOpen() const { return true; }
  int close() { return 0; }
}; // NullBlockManager

/** @} */

#endif // NULL_BLOCK_MANAGER_H
