/* 
 * FileUtil.cc : part of the Mace toolkit for building distributed systems
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
#include <dirent.h>
#include <unistd.h>
#include <utime.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <algorithm>
#include "maceConfig.h"
#include "mace-macros.h"
#include "Accumulator.h"
#include "Log.h"
#include "FileUtil.h"
#include "Util.h"
#include "StrUtil.h"
#include "Replay.h"

#ifdef WINDOWS_FILES
#define lstat stat
#define ELOOP ENOENT
#endif

using namespace std;

const int FileUtil::STDIN;
const int FileUtil::STDOUT;
const int FileUtil::STDERR;
const int FileUtil::READ_INDEX;
const int FileUtil::WRITE_INDEX;


int FileUtil::open(const std::string& path, int flags, mode_t mode)
  throw(FileException) {
  ADD_SELECTORS("FileUtil::open");

  int fd = ::open(path.c_str(), flags, mode);
  if (fd < 0) {
    Log::perror("open " + path);
    throwFileException("open", errno, path);
  }

  traceout << fd << Log::end;

  return fd;
} // open

int FileUtil::open(const std::string& path, int flags) throw(FileException) {
  ADD_SELECTORS("FileUtil::open");
  int fd = ::open(path.c_str(), flags);
  if (fd < 0) {
    Log::perror("open " + path);
    throwFileException("open", errno, path);
  }

  traceout << fd << Log::end;

  return fd;
} // open

void FileUtil::ftruncate(int fd, off_t length) throw(FileException) {
  // XXX - add logging
  int r = ::ftruncate(fd, length);
  if (r < 0) {
    Log::perror("ftruncate " + boost::lexical_cast<std::string>(fd));
    throwFileException("ftruncate", errno, boost::lexical_cast<std::string>(fd));
  }
} // ftruncate

void FileUtil::mkdir(const std::string& path, mode_t mode, bool createParents)
  throw(FileException) {
  // XXX - add logging
  ADD_SELECTORS("FileUtil::mkdir");
  //   macedbg(0) << "In mkdir " << path << " mode " << mode << " createParents " << createParents << Log::endl;
  
  if (createParents) {
    struct stat sbuf;
    bool found = true;
    try {
      statWithErr(path, sbuf);
    }
    catch (const FileNotFoundException& e) {
      found = false;
    }

    if (found) {
      if (S_ISDIR(sbuf.st_mode) && ((sbuf.st_mode & mode) == mode)) {
        //         macedbg(0) << "returning, found dir" << Log::endl;
	return;
      }
    }

    mace::string p;
    StringList l = StrUtil::split("/", path, true);
    while (!l.empty() && l.back().empty()) {
      //       macedbg(0) << "empty l" << Log::endl;
      l.pop_back();
    }
    if (!l.empty()) {
      l.pop_back();
    }

    while (!l.empty()) {
      //       macedbg(0) << "l.front: " << l.front() << Log::endl;
      if (l.front().empty() && p[p.size() - 1] != '/') {
	p.append("/");
      }
      else {
	p.append(l.front());
      }
      l.pop_front();
      try {
        //         macedbg(0) << "statWithErr: " << p << Log::endl;
	statWithErr(p, sbuf);
        //         macedbg(0) << "end statWithErr: " << p << Log::endl;
      }
      catch (const FileNotFoundException& e) {
        //         macedbg(0) << "recursive mkdir in FileNotFoundException" << Log::endl;
	mkdir(p, mode, false);
      }
      if (p != "/") {
        p.append("/");
      }
      //       macedbg(0) << "p: " << p << Log::endl;
    }
  }

  //   macedbg(0) << "calling system mkdir with path " << path << Log::endl;
#ifdef WINDOWS_FILES
  int r = ::mkdir(path.c_str());
  if (r >= 0) {
    chmod(path, mode);
  }
#else
  int r = ::mkdir(path.c_str(), mode);
#endif
  if (r < 0) {
    Log::perror("mkdir " + path);
    throwFileException("mkdir", errno, path);
  }
  //   macedbg(0) << "returning, end of fn" << Log::endl;
} // mkdir

void FileUtil::chdir(const std::string& path) throw(FileException) {
  if (::chdir(path.c_str()) != 0) {
    throwFileException("chdir", errno, path);
  }
} // chdir

int FileUtil::mktemp(std::string& name) throw(FileException) {
  // XXX -- add logging
  size_t s = name.size();
  char* buf = new char[s + 1];
  memcpy(buf, name.data(), s);
  buf[s] = 0;
#ifdef WINDOWS_FILES
  int r = -1;
  if (::mktemp(buf) == NULL) {
    r = open(buf, O_RDWR);
  }
#else
  int r = mkstemp(buf);
#endif
  if (r == -1) {
    delete buf;
    throwFileException("mkstemp", errno, name);
  }
  name.clear();
  name.append(buf, s);
  delete buf;
  return r;
} // mktemp

void FileUtil::rename(const std::string& oldpath, const std::string& newpath) throw(FileException) {
  if (::rename(oldpath.c_str(), newpath.c_str()) != 0) {
    throwFileException("rename", errno, oldpath + " -> " + newpath);
  }
} // rename

void FileUtil::symlink(const std::string& oldpath, const std::string& newpath) throw(FileException) {
  #ifdef WINDOWS_FILES
    ABORT("Windows builds don't support symlinking yet.");
  #else
  if (::symlink(oldpath.c_str(), newpath.c_str()) != 0) {
    throwFileException("symlink", errno, oldpath + " -> " + newpath);
  }
  #endif
} // symlink

void FileUtil::unlink(const std::string& path) throw(FileException) {
  // XXX -- add logging
  if (::unlink(path.c_str()) != 0) {
    throwFileException("unlink", errno, path);
  }
} // unlink

void FileUtil::rmdir(const std::string& path) throw(FileException) {
  if (::rmdir(path.c_str()) != 0) {
    throwFileException("rmdir", errno, path);
  }
} // rmdir

void FileUtil::chmod(const std::string& path, mode_t mode) throw(FileException) {
  if (::chmod(path.c_str(), mode) != 0) {
    throwFileException("chmod", errno, path);
  }
} // chmod

void FileUtil::fchmod(int fd, mode_t mode) throw(FileException) {
  #ifdef WINDOWS_FILES
  ABORT("Windows builds do not support fchmod!\n");
  #else
  if (::fchmod(fd, mode) != 0) {
    throwFileException("chmod", errno, boost::lexical_cast<std::string>(fd));
  }
  #endif
} // fchmod

void FileUtil::utime(const std::string& path, time_t mtime) throw(FileException) {
  utime(path, mtime, TimeUtil::time());
} // utime

void FileUtil::utime(const std::string& path, time_t mtime, time_t atime) throw(FileException) {
  struct utimbuf tbuf = { atime, mtime };
  if (::utime(path.c_str(), &tbuf) != 0) {
    throwFileException("utime", errno, path);
  }
} // utime

int FileUtil::close(int fd) {
  ADD_SELECTORS("FileUtil::close");
  int r = ::close(fd);
  traceout << r << Log::end;
  return r;
} // close

void FileUtil::readDir(const std::string& path, StringList& dirlist, bool sort)
  throw (FileException) {
  ADD_SELECTORS("FileUtil::readDir");
  DIR* d = opendir(path.c_str());
  if (d == 0) {
    Log::perror("opendir " + path);
    throw FileException(getErrorMessage(errno, "opendir", path));
  }

  struct dirent* ent;
  while ((ent = ::readdir(d))) {
    if ((strcmp(ent->d_name, ".") == 0) ||
	(strcmp(ent->d_name, "..") == 0)) {
      continue;
    }

    dirlist.push_back(ent->d_name);
  }

  if (sort) {
    stable_sort(dirlist.begin(), dirlist.end());
  }

  closedir(d);

  traceout << dirlist << Log::end;
} // readDir

std::string FileUtil::loadFile(const std::string& path) throw(FileException) {
  ADD_SELECTORS("FileUtil::loadFile");

  struct stat sbuf;
  statWithErr(path, sbuf);
  
  int fd = ::open(path.c_str(), O_RDONLY);
  if (fd < 0) {
    throw FileException(getErrorMessage(errno, "open", path));
  }

  mace::string r = bufferedRead(fd, sbuf.st_size);
  close(fd);

  traceout << r << Log::end;

  return r;
} // loadFile

void FileUtil::saveFile(const std::string& path, const std::string& data,
			const std::string& bak, bool removeBak) throw(FileException) {

  if (fileIsReg(path) && !bak.empty()) {
    rename(path, bak);
  }

  int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
  if (fd < 0) {
    throw FileException(getErrorMessage(errno, "open", path));
  }

  write(fd, data);

  close(fd);

  if (!bak.empty() && removeBak && fileIsReg(bak)) {
    unlink(bak);
  }
} // saveFile

bool FileUtil::fileIsReg(const std::string& path, bool doLstat) {
  ADD_SELECTORS("FileUtil::fileIsReg");
  struct stat buf;
  if (doLstat) {
    if (::lstat(path.c_str(), &buf) < 0) {
      traceout << false << Log::end;
      return false;
    }
  }
  else {
    if (::stat(path.c_str(), &buf) < 0) {
      traceout << false << Log::end;
      return false;
    }
  }

  traceout << S_ISREG(buf.st_mode) << Log::end;

  return S_ISREG(buf.st_mode);
} // fileIsReg

bool FileUtil::fileIsDir(const std::string& path, bool doLstat) {
  ADD_SELECTORS("FileUtil::fileIsDir");
  struct stat buf;
  if (doLstat) {
    if (::lstat(path.c_str(), &buf) < 0) {
      traceout << false << Log::end;
      return false;
    }
  }
  else {
    if (::stat(path.c_str(), &buf) < 0) {
      traceout << false << Log::end;
      return false;
    }
  }
  traceout << S_ISDIR(buf.st_mode) << Log::end;
  return S_ISDIR(buf.st_mode);
} // fileIsDir

bool FileUtil::fileIsLink(const std::string& path) {
#ifdef WINDOWS_FILES
  return false;
#else
  ADD_SELECTORS("FileUtil::fileIsLink");
  struct stat buf;
  if (::lstat(path.c_str(), &buf) < 0) {
    traceout << false << Log::end;
    return false;
  }
  traceout << S_ISLNK(buf.st_mode) << Log::end;
  return S_ISLNK(buf.st_mode);
#endif
} // fileIsDir

bool FileUtil::fileExists(const std::string& path, bool doLstat) {
  ADD_SELECTORS("FileUtil::fileExists");
  struct stat buf;
  if (doLstat) {
    if (::lstat(path.c_str(), &buf) < 0) {
      traceout << false << Log::end;
      return false;
    }
  }
  else {
    if (::stat(path.c_str(), &buf) < 0) {
      traceout << false << Log::end;
      return false;
    }
  }
  traceout << true << Log::end;
  return true;
} // fileExists

FileUtil::AccessResult FileUtil::canAccess(const std::string& path, int mode)
  throw (FileException) {
  ADD_SELECTORS("FileUtil::canAccess");

  int r = access(path.c_str(), mode);
  if (r < 0) {
    int err = errno;
    if ((err == ELOOP) ||
	(err == ENOTDIR) ||
	(err == ENAMETOOLONG) ||
	(err == ENOENT) ||
	(err == EROFS)) {
      traceout << false << false << mace::string(Util::getErrorString(err)) << Log::end;
      return AccessResult(false, false, Util::getErrorString(err));
    }
    else if (err == EACCES) {
      traceout << false << true << mace::string(Util::getErrorString(err)) << Log::end;
      return AccessResult(false, true, Util::getErrorString(err));
    }

    throwFileException("access", err, path);
  }

  traceout << true << true << Log::end;
  return AccessResult(true, true, "");
} // canAccess

string FileUtil::realpath(const string& path) throw (FileException) {
  #ifndef MAX_PATH
  static const int MAX_PATH = 1028;
  #endif
  // XXX add logging
  ADD_SELECTORS("FileUtil::realpath");
  char buf[MAX_PATH + 1];
  #ifdef WINDOWS_FILES
  if ((GetFullPathName(path.c_str(), MAX_PATH, buf, NULL) == 0)) {
    throwFileException("GetFullPathName", errno, path);
  }
  #else
  if ((::realpath(path.c_str(), buf) == 0)) {
    throwFileException("realpath", errno, path);
  }
  #endif
  return buf;
} // realpath

string FileUtil::getErrorMessage(int err, const string& syscall, const string& path) {
  string errstr = syscall + " " + path + ": " + Util::getErrorString(err);
  return errstr;
} // getErrorMessage

string FileUtil::getErrorString(int err, const string& n, int fd) {
  ostringstream errstr;
  errstr << n << ": " << Util::getErrorString(err) << " on " << fd;
  return errstr.str();
} // getErrorString

void FileUtil::throwFileException(const string& prefix,
				  int err, const string& path)
  throw (FileException) {
    string errstr = getErrorMessage(err, prefix, path);

    if (err == ENOENT) {
//       Log::log("ERROR") << "throwing FileNotFoundException: " << errstr << Log::endl;
      throw FileNotFoundException(errstr);
    }
    else if (err == ENOTDIR) {
//       Log::log("ERROR") << "throwing InvalidPathException: " << errstr << Log::endl;
      throw InvalidPathException(errstr);
    }
    else if (err == EACCES) {
//       Log::log("ERROR") << "throwing PermissionAccessException: " << errstr << Log::endl;
      throw PermissionAccessException(errstr);
    }
    else if (err == ELOOP) {
//       Log::log("ERROR") << "throwing LinkLoopException: " << errstr << Log::endl;
      throw LinkLoopException(errstr);
    }
    else{
//       Log::log("ERROR") << "throwing FileException: " << errstr << Log::endl;
      throw FileException(errstr);
    }
} // throwFileException

void FileUtil::lstatWithErr(const mace::string& path, struct stat& buf)
  throw (FileException) {
  ADD_SELECTORS("FileUtil::lstatWithErr");
  if (::lstat(path.c_str(), &buf) < 0) {
    traceout << false << errno << path << Log::end;
    throwFileException("lstat", errno, path);
  }
  //   traceout << true << buf << Log::end; //XXX - James?
} // lstatWithErr

void FileUtil::statWithErr(const mace::string& path, struct stat& buf)
  throw (FileException) {
  ADD_SELECTORS("FileUtil::statWithErr");
  if (::stat(path.c_str(), &buf) < 0) {
    traceout << false << errno << path << Log::end;
    throwFileException("stat", errno, path);
  }
  //   traceout << true << buf << Log::end; //XXX - James?
} // statWithErr

void FileUtil::fstatWithErr(int fd, struct stat& buf)
  throw (FileException) {
  ADD_SELECTORS("FileUtil::fstatWithErr");
  if (::fstat(fd, &buf) < 0) {
    //     traceout << false << errno << fd << Log::end;
    throwFileException("fstat", errno, boost::lexical_cast<std::string>(fd));
  }
  //   traceout << true << buf << Log::end;
} // fstatWithErr

off_t FileUtil::seek(int fd, off_t offset, int whence) throw (FileException) {
  ADD_SELECTORS("FileUtil::seek");
  off_t r = ::lseek(fd, offset, whence);
  if (r == (off_t)-1) {
    //     traceout << false << errno << fd << offset << whence << Log::end;
    throwFileException("lseek", errno, boost::lexical_cast<std::string>(fd));
  }
  //   traceout << true << r << Log::endl; //XXX - James?
  return r;
} // lseep

void FileUtil::write(int fd, const string& buf, bool log) throw (WriteException) {
  fullWrite(fd, buf.data(), buf.size(), log);
} // fullWrite

void FileUtil::fullWrite(int desc, const char *ptr, size_t len, bool log)
  throw (WriteException) {
  while (len > 0) {
    int written = ::write(desc, ptr, len);
    if (written < 0)  {
      int err = errno;
      if (err == EINTR) {
	continue;
      }
      else if (err == EPIPE) {
	ostringstream os;
	os << "EPIPE error writing to " << desc;
	throw PipeClosedException(os.str());
      }
      Log::perror("write");
      throw WriteException(getErrorString(err, "write", desc));
    }
    ptr += written;
    len -= written;
    if (log) {
      Accumulator::Instance(Accumulator::FILE_WRITE)->accumulate(written);
    }
  }
} // fullWrite

string FileUtil::read(int s) throw (ReadException) {
  mace::string sr;
  read(s, sr);
  return sr;
} // read

size_t FileUtil::read(int s, mace::string& sr) throw (ReadException) {
  ADD_SELECTORS("FileUtil::read");
  const size_t BUF_SIZE = 8192;
  char buf[BUF_SIZE];
  int rc = 0;
  do {
    rc = ::read(s, buf, BUF_SIZE);
  } while (rc < 0 && errno == EINTR);

  if (rc < 0) {
    traceout << false << errno << s << sr << Log::end;
    throw ReadException(getErrorString(errno, "read", s));
  }
  sr.append(buf, rc);
  traceout << true << rc << sr << Log::end; 
  return rc;
} // read

size_t FileUtil::bufferedRead(int32_t s, mace::string& sr, size_t len, bool clear) throw (ReadException) {
  ADD_SELECTORS("FileUtil::bufferedRead");
  if (clear) {
    sr.clear();
  }
  if (len == 0) {
    return 0;
  }
  size_t r = 0;
  const size_t BUF_SIZE = 8192;
  char buf[BUF_SIZE];
  while (r < len) {
    size_t rlen = BUF_SIZE;
    if ((len - r) < BUF_SIZE) {
      rlen = len - r;
    }
    ssize_t rc = ::read(s, buf, rlen);
    if (rc < 0 && errno == EINTR) {
      continue;
    }
    if (rc < 0) {
      traceout << false << errno << s << sr << Log::end;
      throw ReadException(getErrorString(errno, "read", s));
    }
    if (rc == 0) {
      traceout << true << ((uint32_t)r) << sr << Log::end;
      return r;
    }
    r += rc;
    sr.append(buf, rc);
  }
  ASSERT(r == len);
  traceout << true << ((uint32_t)r) << sr << Log::end;
  return r;
} // bufferedRead

string FileUtil::bufferedRead(int s, size_t len) throw (ReadException) {
  mace::string sr;
  bufferedRead(s, sr, len);
  return sr;
} // bufferedRead

int FileUtil::read(int fd, char* buf, size_t count) {
  ADD_SELECTORS("FileUtil::read");
  int r = ::read(fd, buf, count);
  if (Replay::trace()) {
    traceout << r;
    if (r > 0) {
      mace::string s(buf, r);
      traceout << s;
    }
    traceout << Log::end;
  }
  return r;
}

// string FileUtil::safeRead(int fd, size_t len) {
//   char buf[len];
//   ASSERT(safeRead(fd, buf, len) == len);
//   string s(buf, len);
//   return s;
// } // safeRead

// size_t FileUtil::safeRead(int desc, char *ptr, size_t len) {
//   int n_chars;

//   if (len == 0) {
//     return len;
//   }

//   do {
//     n_chars = ::read(desc, ptr, len);
//   } while (n_chars < 0 && errno == EINTR);

//   return n_chars;
// } // safeRead

void FileUtil::remapPipeStdinStdout(pipefdpair_t p2c, pipefdpair_t c2p)
  throw (IOException) {

  close(p2c[WRITE_INDEX]);
  close(c2p[READ_INDEX]);

  close(STDIN);
  close(STDOUT);

  remapPipeStdinStdout(p2c[READ_INDEX], c2p[WRITE_INDEX]);
} // remapPipeStdinStdout


// this method is based off of code written by Stephen Friedl
// http://www.unixwiz.net/techtips/remap-pipe-fds.c.txt
void FileUtil::remapPipeStdinStdout(int rpipe, int wpipe) throw (IOException) {

/*
 *      Given two file descriptors that we obtained from the pipe(2) system
 *	call, map them to stdin(0) and stdout(1) of the current process.
 *	The usual approach is:
 *
 *		dup2(rpipe, 0); close(rpipe);
 *		dup2(wpipe, 1); close(wpipe);
 *
 *	and this works most of the time. But what happens if one of these
 *	descriptors is *already* 0 or 1 - then we end up making a mess of
 *	the descriptors.
 *
 *	The thing that makes this so hard to figure out is that when it's
 *	run from a "regular" user process, the pipe descriptors won't *ever*
 *	be in this low range, but the file-descriptor environment of a
 *	>daemon< is entirely different.
 *
 *	So: to account for this properly  we consider a kind of matrix that
 *	shows all the possible combinations of inputs and how we are to
 *	properly shuffle the FDs around. Each FD is either "0", "1", or
 *	">1", the latter representing the usual case of being "out of the
 *	way" and not in any risk of getting stomped onn.
 *
 *	NOTE: in this table we use the same macro that we use in the C code
 *	below -- DUP2CLOSE(oldfd, newfd) -- that performs the obvious
 *	operations.
 *
 *	     RFD  WFD     Required actions
 *	     ---  ---	  -------------------------
 *     	[A]   0    1      *nothing*
 *
 *	[B]  >1   >1 \__/ DUP2CLOSE(rfd, 0); \___ same actions
 *     	[C]   1   >1 /  \ DUP2CLOSE(wfd, 1); /    for both cases
 *			  
 *
 *     	[D]   0   >1      DUP2CLOSE(wfd, 1);
 *        
 *	[E]  >1    1      DUP2CLOSE(rfd, 0);
 *
 *	[F]  >1    0      DUP2CLOSE(wfd, 1);
 *                        DUP2CLOSE(rfd, 0);
 *
 *     	[G]   1    0      tmp = dup(wfd); close(wfd);
 *                        DUP2CLOSE(rfd, 0);
 *                        DUP2CLOSE(tmp, 1);
 *
 *	Return is TRUE if all is well or FALSE on error.
 *

 *------------------------------------------------------------------------

 * Every time we run a dup2(), we always close the old FD, so this macro
 * runs them both together and evaluates to TRUE if it all went OK and 
 * FALSE if not.
 
  #define	DUP2CLOSE(oldfd, newfd)	(dup2(oldfd, newfd) == 0  &&  close(oldfd) == 0)

 */

  /*------------------------------------------------------------------
   * CASE [A]
   *
   * This is the trivial case that probably never happens: the two FDs
   * are already in the right place and we have nothing to do. Though
   * this probably doesn't happen much, it's guaranteed that *doing*
   * any shufflingn would close descriptors that shouldn't have been.
   */

  if ((rpipe == STDIN) && (wpipe == STDOUT)) {
    return;
  }

  /*----------------------------------------------------------------
   * CASE [B] and [C]
   *
   * These two have the same handling but not the same rules. In case
   * [C] where both FDs are "out of the way", it doesn't matter which
   * of the FDs is closed first, but in case [B] it MUST be done in
   * this order.
   */

  if ((rpipe >= 1) && (wpipe > 1)) {
    dup2close(rpipe, STDIN);
    dup2close(wpipe, STDOUT);
    return;
  }

  /*----------------------------------------------------------------
   * CASE [D]
   * CASE [E]
   *
   * In these cases, *one* of the FDs is already correct and the other
   * one can just be dup'd to the right place:
   */

  if ((rpipe == STDIN) && (wpipe >= 1)) {
    dup2close(wpipe, STDOUT);
    return;
  }

  if ((rpipe >= 1) && (wpipe == STDOUT)) {
    dup2close(rpipe, STDIN);
    return;
  }


  /*----------------------------------------------------------------
   * CASE [F]
   *
   * Here we have the write pipe in the read slot, but the read FD
   * is out of the way: this means we can do this in just two steps
   * but they MUST be in this order.
   */

  if ((rpipe >= 1) && (wpipe == STDIN)) {
    dup2close(wpipe, STDOUT);
    dup2close(rpipe, STDIN);
    return;
  }

  /*----------------------------------------------------------------
   * CASE [G]
   *
   * This is the trickiest case because the two file descriptors  are
   * *backwards*, and the only way to make it right is to make a
   * third temporary FD during the swap.
   */
  if ((rpipe == STDOUT) && (wpipe == STDIN)) {
    const int tmp = dup(wpipe);		/* NOTE! this is not dup2() ! */

    if (tmp <= 1) {
      throw IOException(getErrorString(errno, "dup", tmp));
    }

    if (close(wpipe) < 0) {
      throw IOException(getErrorString(errno, "close", wpipe));
    }

    dup2close(rpipe, STDIN);
    dup2close(tmp, STDOUT);

    return;
  }

  /* SHOULD NEVER GET HERE */

  ASSERT(false);
} // remapPipeStdinStdout

void FileUtil::dup2close(int oldfd, int newfd) throw (IOException) {
  if (dup2(oldfd, newfd) < 0) {
    throw IOException(getErrorString(errno, "dup2", oldfd));
  }

  if (close(oldfd) < 0) {
    throw IOException(getErrorString(errno, "close", oldfd));
  }
} // dup2close

std::string FileUtil::basename(const std::string& buf) {
  size_t i = buf.rfind("/");
  if (i != string::npos) {
    return buf.substr(i + 1);
  }
  return buf;
} // basename

std::string FileUtil::dirname(const std::string& buf) {
  size_t i = buf.rfind("/");
  if (i != string::npos) {
    while (i > 0 && buf[i] == '/') {
      i--;
    }
    return buf.substr(0, i + 1);
  }
  return ".";
} // dirname

std::string FileUtil::readlink(const std::string& path) throw (FileException) {
  ADD_SELECTORS("FileUtil::readlink");
  int lbufSize = PATH_MAX;
  char lbuf[lbufSize];
  #ifdef WINDOWS_FILES
  int r = EINVAL;
  #else
  int r = ::readlink(path.c_str(), lbuf, lbufSize);
  #endif
  if (r < 0) {
    throwFileException("readlink", errno, path);
  }
  mace::string s(lbuf, r);
  //   traceout << s << Log::endl; //XXX - James?
  return s;
} // readlink
