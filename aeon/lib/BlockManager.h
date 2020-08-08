/* 
 * BlockManager.h : part of the Mace toolkit for building distributed systems
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
#include <stdint.h>
#include <sys/types.h>
#include <string>

/**
 * \file BlockManager.h
 * \brief Declares the BlockManager class
 */

#ifndef BLOCK_MANAGER_H
#define BLOCK_MANAGER_H

/**
 * \addtogroup BlockManagers
 * \brief The block managers provide a simple block-index based put/get interface for accessing files, memory segments, and the like
 * @{
 */

/**
 * \brief The base class (pure virtual) of a block manager interface for
 * referencing files or segments as a set of blocks.
 *
 * Subclasses may provide additional functionality, such as filesystem access,
 * encryption, etc.
 */
class BlockManager {
public:
  BlockManager();
  virtual const std::string& getPath() const = 0; ///< return the path, if any, of the storage backing
  virtual std::string getFileName() const = 0; ///< return the filename (without path) 
  virtual std::string getBlock(uint64_t index) = 0; ///< get the block at the given index
  virtual size_t setBlock(uint64_t index, const std::string& buffer) = 0; ///< set the block at the given index
  virtual size_t getBlockSize() const { return blockSize; } ///< return the block size
  virtual void setBlockSize(size_t s) { blockSize = s; } ///< set the block size
  virtual void prefetchBlock(uint64_t index) { } ///< suggestion, request the block to be prepared for future getting
  /** \todo Ryan, what are getMinimumInterested and getMaximumInterested */
  virtual uint32_t getMinimumInterested() const { return minimumInterested; } ///< Huh?
  virtual uint32_t getMaximumInterested() const { return maximumInterested; } ///< Huh?
  virtual uint64_t getBlockCount() const { return blockCount; } ///< return the number of blocks
  virtual uint64_t getSize() const { return size; } ///< return the size in bytes
  virtual int open(const std::string& path, const char* mode) = 0; ///< open the block manager.  The path may be used by some to open a file with a given mode
  virtual bool isOpen() const = 0; ///< return whether the block manager is open
  virtual int close() = 0; ///< close the block manager, flushing to disk and cleaning up state
  virtual int flush() = 0; ///< flush storage, e.g. to disk
  virtual ~BlockManager() {}

public:
  static const size_t DEFAULT_BLOCK_SIZE; ///< the default block size

protected:
  size_t blockSize;
  uint64_t blockCount;
  uint64_t size;
public:
  uint32_t minimumInterested;
  uint32_t maximumInterested;
}; // BlockManager

/** @} */

#endif // BLOCK_MANAGER_H
