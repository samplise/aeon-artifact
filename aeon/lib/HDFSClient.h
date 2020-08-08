/* 
 * HDFSClient.h : part of the Mace toolkit for building distributed systems
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <hdfs.h>
#include "Collections.h"
#include "Exception.h"
#include "mvector.h"

#ifndef _HDFS_CLIENT_H
#define _HDFS_CLIENT_H

/**
 * \file HDFSClient.h
 * \brief Hadoop Distributed FileSystem Client library wrapper
 */

typedef mace::vector<StringSet> DFSBlocksLocations;

/** 
 * hdfsFileInfo - Information about a file/directory.
 */
typedef struct  {
  bool isDir;          /* file or directory */
  std::string name;    /* the name of the file */
  time_t last_mod;      /* the last modification time for the file in seconds */
  off_t size;       /* the size of the file in bytes */
  short replication;    /* the count of replicas */
  off_t blockSize;  /* the block size for the file */
//  std::string owner;        /* the owner of the file */
//  std::string group;        /* the group associated with the file */
//  short permissions;  /* the permissions associated with the file */
//  time_t lastAccess;    /* the last access time for the file in seconds */
} DFSFileProperties;
typedef std::vector<DFSFileProperties> DFSDirList;

#define HDFS_PERM_FILE_DEFAULT (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH) // 644 rw-r--r--
#define HDFS_PERM_FOLDER_DEFAULT (S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH) // 755 rwxr-xr-x

class HDFSClient {
 public:
  /// Returns a static object reference. Also establishes HDFS connections.
  static HDFSClient& Instance();
  
  /**
   * \brief Open a file for reading/writing/appending
   *
   * The replication and blockSize parameters are used when creating new files
   * Use open(path) for reading
   */
  int open(const std::string& path, int flags = O_RDONLY, mode_t mode = HDFS_PERM_FILE_DEFAULT,
           uint32_t bufferSize = 0, short replication = 0,
           uint32_t blockSize = 0)
    throw(FileException);
  /// Create a new file and open for writing (overwrite existing)
  int create(const std::string& path, mode_t mode = HDFS_PERM_FILE_DEFAULT,
             uint32_t bufferSize = 0, short replication = 0,
             uint32_t blockSize = 0) throw(FileException) {
    return open(path, O_WRONLY|O_TRUNC|O_CREAT, mode,
                bufferSize, replication, blockSize);
  }
  /// Append to an existing file
  int append(const std::string& path, uint32_t bufferSize = 0) throw(FileException) {
    return open(path, O_WRONLY|O_APPEND, HDFS_PERM_FILE_DEFAULT, bufferSize);
  }
  int close(const int fd) throw(FileException); ///< Close the file handle
  
  bool fileExists(const std::string& path) throw(FileException); ///< Return true if file exists
  bool fileIsReg(const std::string& path) throw(FileException) { ///< Return true if path is file
    return !stat(path).isDir;
  }
  bool fileIsDir(const std::string& path) throw(FileException) { ///< Return true if path is folder
    return stat(path).isDir;
  }

  void rename(const std::string& oldpath, const std::string& newpath) throw(FileException);
  void rm(const std::string& path, bool recursive = false) throw(FileException);
  void mkdir(const std::string& path, mode_t mode = HDFS_PERM_FOLDER_DEFAULT,
             bool createParents = true)
    throw(FileException);
  void rmdir(const std::string& path) throw(FileException) { rm(path); }
  void chdir(const std::string& path) throw(FileException);
  std::string pwd() throw(FileException);
  
  void chmod(const std::string& path, mode_t mode) throw(FileException);
  void utime(const std::string& path, time_t mtime) throw(FileException) { utime(path, mtime, 0); }
  void utime(const std::string& path, time_t mtime, time_t atime)
    throw(FileException);
 
  /// Seek to the specified location from the start of the file
  off_t seek(const int fd, off_t offset) throw(FileException);
  off_t tell(const int fd) throw(FileException); ///< Returns the current position in file
  size_t write(const int fd, const char* buf, const size_t size, bool log = false) throw(WriteException);
  size_t write(const int fd, const std::string& buf, bool log = false) throw(WriteException) {
    return write(fd, buf.data(), buf.size(), log);
  }
  int read(const int fd, char* buf, const size_t size) throw(ReadException);
  int pread(const int fd, const off_t offset, char* buf, const size_t size) throw(ReadException);
  void flush(const int fd) throw(WriteException);
 
  /// Returns a struct of all file properties in DFSFileProperties
  DFSFileProperties stat(const std::string& path) throw(FileException);
  /// Inserts a list of DFSFileProperties into dirlist in given path
  void readDir(const std::string& path, DFSDirList& dirlist, bool sort = false) throw(FileException);

  /// Set a new value of replication for file
  void setReplication(const std::string& path, const short replication) throw(FileException);
  /**
   * Find the locations of all blocks in given range
   * @param path The path of the file
   * @param start Start byte position
   * @param size Length of region to consider
   * @return A set of hostnames for each block that overlaps in the given range
   */
  DFSBlocksLocations getBlocksLocations(const std::string& path, const off_t start, const size_t size) throw (FileException);

 private:
  static DFSFileProperties getFileProperties(hdfsFileInfo *info);
  
  int fd_next; ///< Unique integer file-descriptor for I/O
  hdfsFS fs; ///< HDFS FileSystem connection
  std::map<int, hdfsFile> fd_hdfs; ///< Map from integer file-desc to hdfs file
  
  pthread_mutex_t ulock; ///< Lock on fd_next and fd_hdfs

  HDFSClient();
  ~HDFSClient();
  static HDFSClient* _inst;
};

#endif // _HDFS_CLIENT_H
