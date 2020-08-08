/* 
 * FileUtil.h : part of the Mace toolkit for building distributed systems
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
#ifndef _FILE_UTIL_H
#define _FILE_UTIL_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>

/**
 * \file FileUtil.h
 * \brief defines the FileUtil utilities
 */

#ifndef S_IRGRP
// Set of macro definitions for windows compatability
#define S_IRGRP S_IRUSR
#define S_IWGRP S_IWUSR
#define S_IXGRP S_IXUSR
#define S_IROTH S_IRUSR
#define S_IWOTH S_IWUSR
#define S_IXOTH S_IXUSR
#endif

#include "Collections.h"

typedef int pipefdpair_t[2];

/**
 * \addtogroup Utils
 * @{
 */

/**
 * \brief helper methods for accessing the filesystem
 *
 * Use these instead of the basic ones for portability and to use other Mace profiling, tracing, etc.
 *
 * \todo JWA, can you document?
 */
class FileUtil {
public:
  class AccessResult {
  public:
    AccessResult() : access(false), exists(false) { }
    AccessResult(bool access, bool exists, const std::string& error) :
      access(access), exists(exists), error(error) { }
    bool access;
    bool exists;
    std::string error;
  };

public:  
  static int open(const std::string& path, int flags, mode_t mode) throw(FileException); 
  static int open(const std::string& path, int flags = O_RDONLY) throw(FileException); 
  static int create(const std::string& path, mode_t mode) throw(FileException) {
    return open(path, O_WRONLY|O_TRUNC|O_CREAT, mode);
  }
  static void ftruncate(int fd, off_t length) throw(FileException);
  static void mkdir(const std::string& path, mode_t mode, bool createParents = false)
    throw(FileException);
  static void chdir(const std::string& path) throw(FileException);
  static int mktemp(std::string& name) throw(FileException);
  static void rename(const std::string& oldpath, const std::string& newpath) throw(FileException);
  static void symlink(const std::string& oldpath, const std::string& newpath) throw(FileException);
  static void unlink(const std::string& path) throw(FileException);
  static void rmdir(const std::string& path) throw(FileException);
  static void chmod(const std::string& path, mode_t mode) throw(FileException);
  static void fchmod(int fd, mode_t mode) throw(FileException);
  static void utime(const std::string& path, time_t mtime)
    throw(FileException);
  static void utime(const std::string& path, time_t mtime, time_t atime)
    throw(FileException);
  static int close(int fd);
  static bool fileExists(const std::string& path, bool doLstat = false);
  static bool fileIsReg(const std::string& path, bool doLstat = false);
  static bool fileIsLink(const std::string& path);
  static bool fileIsDir(const std::string& path, bool doLstat = false);
  // first - accessible, second - exists
  static AccessResult canAccess(const std::string& path, int mode)
    throw (FileException);
  static void fstatWithErr(int fd, struct stat& buf) throw (FileException);
  static void statWithErr(const mace::string& path, struct stat& buf)
    throw (FileException);
  static void lstatWithErr(const mace::string& path, struct stat& buf)
    throw (FileException);
  static void fstat(int fd, struct stat& buf) throw (FileException) {
    fstatWithErr(fd, buf);
  }
  static void stat(const mace::string& path, struct stat& buf)
    throw (FileException) {
    statWithErr(path, buf);
  }
  static void lstat(const mace::string& path, struct stat& buf)
    throw (FileException) {
    lstatWithErr(path, buf);
  }

//   static void realpathWithErr(const std::string& path, char* buf) throw (FileException);
  static off_t seek(int fd, off_t offset, int whence = SEEK_SET)
    throw (FileException);
  static void write(int fd, const char* buf, size_t size, bool log = false)
    throw (WriteException) {
    fullWrite(fd, buf, size, log);
  }
  static void write(int fd, const std::string& buf, bool log = false)
    throw (WriteException);
  static std::string read(int s) throw (ReadException);
  static int read(int fd, char* buf, size_t count);
  static size_t read(int s, mace::string& sr) throw (ReadException);
  static std::string bufferedRead(int s, size_t len) throw (ReadException);
  static size_t bufferedRead(int s, mace::string& sr, size_t len, bool clear = true) throw (ReadException);
  static void saveFile(const std::string& path, const std::string& data,
		       const std::string& backup = "", bool removeBackup = false)
    throw (FileException);
  static std::string loadFile(const std::string& path) throw (FileException);
  static void remapPipeStdinStdout(pipefdpair_t p2c, pipefdpair_t c2p)
    throw (IOException);
  static void remapPipeStdinStdout(int rpipe, int wpipe) throw (IOException);
  static void dup2close(int oldfd, int newfd) throw (IOException);
  static void readDir(const std::string& path, StringList& dirlist, bool sort = false)
    throw (FileException);
  static std::string readlink(const std::string& path) throw (FileException);
  static std::string basename(const std::string& buf);
  static std::string dirname(const std::string& buf);
  static std::string realpath(const std::string& path) throw (FileException);


public:
  static const int STDIN = 0;
  static const int STDOUT = 1;
  static const int STDERR = 2;
  static const int READ_INDEX = 0;
  static const int WRITE_INDEX = 1;

private:
  static void throwFileException(const std::string& prefix, int err, const std::string& path) throw (FileException);
  static std::string getErrorString(int err, const std::string& n, int fd);
  static std::string getErrorMessage(int err, const std::string& syscall,
				     const std::string& path);

  static void fullWrite(int fd, const char* ptr, size_t len, bool log = false)
    throw (WriteException);
//   static size_t safeRead(int fd, char* ptr, size_t len);
//   static std::string safeRead(int fd, size_t len);

};

/** @} */

#endif // _FILE_UTIL_H
