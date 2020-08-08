/* 
 * FileBlockManager.h : part of the Mace toolkit for building distributed systems
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
#include <string>

#include "BlockManager.h"

#ifndef FILE_BLOCK_MANAGER_H
#define FILE_BLOCK_MANAGER_H

/**
 * \file FileBlockManager.h
 * \brief declares the BufferedBlockManager class
 */

/**
 * \addtogroup BlockManagers
 * @{
 */

/// The FileBlockManager provides a BlockManager interface by interfacing directly with the filesystem.  Often used below a BufferedBlockManager.
class FileBlockManager : public BlockManager {
public:
  FileBlockManager();
  virtual ~FileBlockManager();
  virtual const std::string& getPath() const;
  virtual std::string getFileName() const;
  virtual std::string getBlock(uint64_t index);
  virtual size_t setBlock(uint64_t index, const std::string& buffer);
  virtual int open(const std::string& path, const char* mode);
  virtual bool isOpen() const;
  virtual int close();
  virtual int flush();

private:
  bool isOpenFlag;
  std::string path;
  FILE* fp;

}; // FileBlockManager

/** @} */

#endif // FILE_BLOCK_MANAGER_H
