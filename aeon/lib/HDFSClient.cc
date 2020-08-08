/* 
 * HDFSClient.cc : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, Karthik Nagaraj
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

#include "HDFSClient.h"
#include "ScopedLock.h"
#include "mace-macros.h"
#include "params.h"

HDFSClient* HDFSClient::_inst = NULL;

HDFSClient::HDFSClient() : fd_next(0) {
  ADD_SELECTORS("HDFSClient::HDFSClient");
  pthread_mutex_init(&ulock, 0);
  
  std::string namenode = params::get<std::string>("HDFS_namenode", "default");
  uint16_t port = params::get<uint16_t>("HDFS_port", 0);
  fs = hdfsConnect(namenode.c_str(), port);
  // Code compatible with 0.21.0 for user
  //const char* userStr = params::containsKey("HDFS_user") ?
  //    params::get<std::string>("HDFS_user").c_str() : NULL;
  //fs = hdfsConnectAsUser(namenode.c_str(), port, userStr);
  if (!fs)
    ABORT("Cannot connect to HDFS Namenode");
}

HDFSClient::~HDFSClient() {
  ADD_SELECTORS("HDFSClient::~HDFSClient");
  // Close files
  for (std::map<int, hdfsFile>::const_iterator fdit = fd_hdfs.begin();
       fdit != fd_hdfs.end();
       ++fdit) {
    hdfsCloseFile(fs, fdit->second);
  }

  hdfsDisconnect(fs); // Disconnect
}

HDFSClient& HDFSClient::Instance() {
  ADD_SELECTORS("HDFSClient::Instance");
  if (_inst == NULL) {
    _inst = new HDFSClient();
  }
  return *_inst;
}

int HDFSClient::open(const std::string& path, int flags, mode_t mode,
                     uint32_t bufferSize, short replication,
                     uint32_t blockSize) throw(FileException) {
  ADD_SELECTORS("HDFSClient::open");
  hdfsFile f = hdfsOpenFile(fs, path.c_str(), flags,
                            bufferSize, replication, blockSize);

  if (!f) {
    throw FileException("HDFS file open failed!");
  }
  
  ScopedLock ul(&ulock);
  int fd = fd_next++;
  fd_hdfs[fd] = f;
  return fd;
}

int HDFSClient::close(const int fd) throw(FileException) {
  ADD_SELECTORS("HDFSClient::close");
  ASSERT (fd_hdfs.find(fd) != fd_hdfs.end());
  ScopedLock ul(&ulock);
  if (hdfsCloseFile(fs, fd_hdfs[fd]) == 0) {
    fd_hdfs.erase(fd);
    return 0;
  } 
  
  throw FileException("HDFS file close failed!");
  return -1;
}

bool HDFSClient::fileExists(const std::string& path) throw(FileException) {
  ADD_SELECTORS("HDFSClient::fileExists");
  return hdfsExists(fs, path.c_str()) == 0;
}

void HDFSClient::rename(const std::string& oldpath, const std::string& newpath)
  throw(FileException) {
  ADD_SELECTORS("HDFSClient::rename");
  if (hdfsRename(fs, oldpath.c_str(), newpath.c_str()) == -1) {
    throw FileException("HDFS file rename failed!");
  }
}

void HDFSClient::rm(const std::string& path, bool recursive) throw(FileException) {
  ADD_SELECTORS("HDFSClient::rm");
  if (hdfsDelete(fs, path.c_str(), recursive) == -1) {
  //if (hdfsDelete(fs, path.c_str()) == -1) {
    throw FileException("HDFS file rm failed!");
  }
}

void HDFSClient::mkdir(const std::string& path, mode_t mode,
                       bool createParents) throw(FileException) {
  ADD_SELECTORS("HDFSClient::mkdir");
  if (hdfsCreateDirectory(fs, path.c_str()) == -1) {
    throw FileException("HDFS mkdir failed!");
  }
}

void HDFSClient::chdir(const std::string& path) throw(FileException) {
  ADD_SELECTORS("HDFSClient::chdir");
  if (hdfsSetWorkingDirectory(fs, path.c_str()) == -1) {
    throw FileException("HDFS chdir failed!");
  }
}

std::string HDFSClient::pwd() throw(FileException) {
  ADD_SELECTORS("HDFSClient::pwd");
  char buffer[8192];
  char* dir = hdfsGetWorkingDirectory(fs, buffer, 8192);
  if (!dir) {
    throw FileException("HDFS pwd failed!");
  }
  return std::string(dir);
}

void HDFSClient::chmod(const std::string& path, mode_t mode)
  throw(FileException) {
  ADD_SELECTORS("HDFSClient::chmod");
  if (hdfsChmod(fs, path.c_str(), mode) == -1) {
    throw FileException("HDFS file chmod failed!");
  }
}

void HDFSClient::utime(const std::string& path, time_t mtime, time_t atime)
  throw(FileException) {
  ADD_SELECTORS("HDFSClient::utime");
  if (hdfsUtime(fs, path.c_str(), mtime, atime) == -1) {
    throw FileException("HDFS file utime failed!");
  }
}

off_t HDFSClient::seek(const int fd, off_t offset)
  throw(FileException) {
  ADD_SELECTORS("HDFSClient::seek");
  ASSERT (fd_hdfs.find(fd) != fd_hdfs.end());
  if (hdfsSeek(fs, fd_hdfs[fd], offset) == -1) {
    throw FileException("HDFS file seek failed!");
  }
  return offset;
}

off_t HDFSClient::tell(const int fd) throw(FileException) {
  ADD_SELECTORS("HDFSClient::tell");
  ASSERT (fd_hdfs.find(fd) != fd_hdfs.end());
  off_t offset = hdfsTell(fs, fd_hdfs[fd]);
  if (offset == -1) {
    throw FileException("HDFS file tell failed!");
  }
  return offset;
}

size_t HDFSClient::write(const int fd, const char* buf, const size_t size,
                         bool log) throw(WriteException) {
  ADD_SELECTORS("HDFSClient::write");
  ASSERT (fd_hdfs.find(fd) != fd_hdfs.end());
  int bytes_written = hdfsWrite(fs, fd_hdfs[fd], buf, size);
  if (bytes_written == -1) {
    throw WriteException("HDFS file write failed!");
  }
  // XXX: log is ignored
  return bytes_written;
}

int HDFSClient::read(const int fd, char* buf, const size_t count)
  throw(ReadException) {
  ADD_SELECTORS("HDFSClient::read");
  ASSERT (fd_hdfs.find(fd) != fd_hdfs.end());
  int bytes_read = hdfsRead(fs, fd_hdfs[fd], buf, count);
  if (bytes_read == -1) {
    throw ReadException("HDFS file read failed!");
  }
  return bytes_read;
}

int HDFSClient::pread(const int fd, const off_t offset, char* buf,
                      const size_t count) throw(ReadException) {
  ADD_SELECTORS("HDFSClient::pread");
  ASSERT (fd_hdfs.find(fd) != fd_hdfs.end());
  int bytes_read = hdfsPread(fs, fd_hdfs[fd], offset, buf, count);
  if (bytes_read == -1) {
    throw ReadException("HDFS file pread failed!");
  }
  return bytes_read;
}

void HDFSClient::flush(const int fd) throw(WriteException) {
  ADD_SELECTORS("HDFSClient::flush");
  ASSERT (fd_hdfs.find(fd) != fd_hdfs.end());
  if (hdfsFlush(fs, fd_hdfs[fd]) == -1) {
    throw WriteException("HDFS file flush failed!");
  }
}

DFSFileProperties HDFSClient::getFileProperties(hdfsFileInfo *info) {
  DFSFileProperties file;
  file.isDir = (info->mKind == kObjectKindDirectory);
  file.name = info->mName;
  file.last_mod = info->mLastMod;
  file.size = info->mSize;
  file.replication = info->mReplication;
  file.blockSize = info->mBlockSize;
  return file;
}

void HDFSClient::readDir(const std::string& path, DFSDirList& dirlist,
                         bool sort) throw(FileException) {
  ADD_SELECTORS("HDFSClient::readDir");
  int numEntries = 0;
  hdfsFileInfo* list = hdfsListDirectory(fs, path.c_str(), &numEntries);
  if (list) {
    for (int i = 0; i < numEntries; ++i) {
      dirlist.push_back(getFileProperties(&list[i]));
    }
    // XXX: sort is ignored
  } else {
    throw FileException("HDFS readDir failed!");
  }
  hdfsFreeFileInfo(list, numEntries);
}

DFSFileProperties HDFSClient::stat(const std::string& path) throw(FileException) {
  ADD_SELECTORS("HDFSClient::stat");
  DFSFileProperties file;
  hdfsFileInfo* info = hdfsGetPathInfo(fs, path.c_str());
  if (info) {
    file = getFileProperties(info);
  } else {
    throw FileException("HDFS file stat failed!");
  }
  return file;
}

void HDFSClient::setReplication(const std::string& path,
                                const short replication) throw(FileException) {
  ADD_SELECTORS("HDFSClient::setReplication");
  if (hdfsSetReplication(fs, path.c_str(), replication) == -1) {
    throw FileException("HDFS file setReplication failed!");
  }
}

DFSBlocksLocations HDFSClient::getBlocksLocations(const std::string& path,
                                                 const off_t start,
                                                 const size_t size)
  throw(FileException) {
  ADD_SELECTORS("HDFSClient::getBlocksLocations");
  DFSBlocksLocations blocks;
  char*** blockHosts = hdfsGetHosts(fs, path.c_str(), start, size);
  if (blockHosts) {
    for (int i = 0; blockHosts[i]; ++i) {
      StringSet locations;
      for (int j = 0; blockHosts[i][j]; ++j) {
        locations.insert(blockHosts[i][j]);
      }
      blocks.push_back(locations);
    }
  } else {
    throw FileException("HDFS file getBlocksLocations failed!");
  }

  hdfsFreeHosts(blockHosts);
  return blocks;
}



