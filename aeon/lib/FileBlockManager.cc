/* 
 * FileBlockManager.cc : part of the Mace toolkit for building distributed systems
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
#include <sys/types.h>
#include <sys/stat.h>
#include <inttypes.h>
// #include <unistd.h>
// #include <stdio.h>
#include <assert.h>

#include "Log.h"
#include "FileBlockManager.h"

using namespace std;

FileBlockManager::FileBlockManager() :
  isOpenFlag(false), path("(null)"), fp(NULL) {
} // FileBlockManager

FileBlockManager::~FileBlockManager() {
  close();
} // ~FileBlockManager

const string& FileBlockManager::getPath() const {
  return path;
}

string FileBlockManager::getFileName() const {
  if(path.find("/") != string::npos) {
    return path.substr(path.rfind("/")+1);
  }
  else {
    return path;
  }
}

string FileBlockManager::getBlock(uint64_t index) {
  assert(isOpen());
  if (fp == 0) {
    fprintf(stderr,
	    "FileBlockManager::getBlock: attempt to read block for null file pointer\n");
    exit(-1);
  }

//   size_t offset = index * blockSize;
  char buf[blockSize];
  
//   if (fseek(fp, offset, SEEK_SET) < 0) {
//     perror("FileBlockManager::getBlock: fseek");
//     exit(-1);
//   }

//   int read = fread(buf, 1, blockSize, fp);
//   if (ferror(fp)) {
//     perror("FileBlockManager::getBlock: fread");
//     ABORT("fread");
//   }
//   if ((read == 0) && (feof(fp))) {
//     clearerr(fp);
//     fprintf(stderr, "FileBlockManager::getBlock: read 0 sized block for EOF\n");
//     exit(-1);
// //     r = new string;
// //     return r;
//   }
//   else if (read <= 0) {
//     perror("FileBlockManager::getBlock: fread");
//     ABORT("fread2");
//   }

//   return string(buf, read);
  return string(buf, blockSize);
} // getBlock

size_t FileBlockManager::setBlock(uint64_t index, const string& buf) {
  assert(isOpen());
  if (fp == 0) {
    fprintf(stderr,
	    "FileBlockManager::setBlock: attempt to write block for null file pointer\n");
    exit(-1);
  }

//   size_t offset = index * blockSize;
//   if (fseek(fp, offset, SEEK_SET) < 0) {
//     perror("FileBlockManager::setBlock: fseek");
//     exit(-1);
//   }
//   size_t r = fwrite(buf.data(), 1, buf.size(), fp);
//   if (r != buf.size()) {
//     perror("FileBlockManager::setBlock: fwrite");
//     exit(-1);
//   }
//   return r;
  return buf.size();
} // setBlock

int FileBlockManager::open(const std::string& p, const char* mode) {
  if (isOpen()) {
    return 0;
  }
  Log::logf("FileBlockManager::open", "path=%s mode=%s", p.c_str(), mode);
    
  fp = fopen(p.c_str(), mode);
  if (fp == 0) {
    perror("FileBlockManager: fopen");
    exit(-1);
//     return -1;
  }

  isOpenFlag = true;
  path = p;

  // do an fstat on the file
  struct stat sbuf;
  if (fstat(fileno(fp), &sbuf) != 0) {
    perror("FileBlockManager::open: fstat");
    exit(-1);
//     return -1;
  }

  size = sbuf.st_size;
  blockCount = size / blockSize;
  if ((size % blockSize) != 0) {
    blockCount++;
  }

  Log::logf("FileBlockManager::open",
	    "opened file %d size=%" PRIu64 " blockSize=%zu blockCount=%" 
	    PRIu64 , fileno(fp), size, blockSize, blockCount);

  return 0;
} // open

bool FileBlockManager::isOpen() const {
  return isOpenFlag;
} // isOpen

int FileBlockManager::close() {
  if (!isOpen()) {
    return 0;
  }
  isOpenFlag = false;
  Log::logf("FileBlockManager::close", "closing file %d", fileno(fp));
  int r = fclose(fp);
  return r;
} // close

int FileBlockManager::flush() {
  if (!isOpen()) {
    return 0;
  }

  int r = fflush(fp);
  return r;
} // flush
