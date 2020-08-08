/* 
 * HashUtil.cc : part of the Mace toolkit for building distributed systems
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
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/md5.h>

#include "HashUtil.h"
#include "hash_string.h"
#include "Util.h"
#include "FileUtil.h"
#include "mace-macros.h"

using namespace std;

const sha1& HashUtil::NULL_HASH = HashUtil::nullHash();
const size_t HashUtil::BLOCK_SIZE = 8192;
const size_t HashUtil::MAX_RETRY_COUNT = 7;

sha1& HashUtil::nullHash() {
  static sha1 nullHashVal;
  char c = 0;
  computeSha1(&c, 0, nullHashVal);

  return nullHashVal;
} // nullHash

void HashUtil::computeSha1(const string& buf, sha1& r) {
  computeSha1((void*)buf.data(), buf.size(), r);
} // computeSha1

void HashUtil::computeSha1(int buf, sha1& r) {
  computeSha1(&buf, sizeof(buf), r);
} // computeSha1

void HashUtil::computeSha1(const void* buf, size_t size, sha1& r) {
  char sha[SHA_DIGEST_LENGTH];
  SHA1((const unsigned char*)buf, size, (unsigned char*)sha);
  r.clear();
  r.append(sha, SHA_DIGEST_LENGTH);
} // computeSha1

void HashUtil::computeMD5(const void* buf, size_t size, md5& r) {
  char md[MD5_DIGEST_LENGTH];
  MD5((const unsigned char*)buf, size, (unsigned char*)md);
  r.clear();
  r.append(md, MD5_DIGEST_LENGTH);
} // copmuteMD5

void HashUtil::computeFileSha1(const std::string& filePath, sha1& h, struct stat& sbuf,
			       bool dostat, bool followlink) throw (FileException) {
  computeFileHash(filePath, h, sbuf, dostat, followlink, EVP_sha1());
} // computeFileSha1

void HashUtil::computeFileMD5(const std::string& filePath, md5& h, struct stat& sbuf,
			      bool dostat, bool followlink) throw (FileException) {
  computeFileHash(filePath, h, sbuf, dostat, followlink, EVP_md5());
} // computeFileMD5

EVP_MD_CTX* HashUtil::initCTX(const EVP_MD* md) {
  EVP_MD_CTX* mdctx = new EVP_MD_CTX();
  EVP_MD_CTX_init(mdctx);
  EVP_DigestInit_ex(mdctx, md, NULL);
  return mdctx;
} // initCTX

void HashUtil::finalizeCTX(EVP_MD_CTX* mdctx, std::string& r) {
  unsigned char rbuf[EVP_MAX_MD_SIZE];
  unsigned int rlen;
  EVP_DigestFinal_ex(mdctx, rbuf, &rlen);
  freeCTX(mdctx);

  r.clear();
  r.append((const char*)rbuf, rlen);
} // finalizeCTX

void HashUtil::freeCTX(EVP_MD_CTX* mdctx) {
  EVP_MD_CTX_cleanup(mdctx);
  delete mdctx;
} // freeCTX

void HashUtil::computeHash(const void* buf, size_t size, std::string& r,
			   const EVP_MD* md) {
  EVP_MD_CTX* c = initCTX(md);
  EVP_DigestUpdate(c, buf, size);
  finalizeCTX(c, r);
} // computeHash

HashUtil::Context::Context(const EVP_MD* m) : md(m), ctx(0), finalized(false) {
  ctx = initCTX(md);
} // Context::Context

HashUtil::Context::~Context() {
  if (ctx) {
    freeCTX(ctx);
  }
} // Context::~Context

const mace::string& HashUtil::Context::getHash() {
  ASSERT(finalized);
  return hash;
} // Context::getHash

void HashUtil::Context::update(const std::string& buf) {
  EVP_DigestUpdate(ctx, buf.data(), buf.size());
} // Context::update

void HashUtil::Context::finalize() {
  finalizeCTX(ctx, hash);
  ctx = 0;
  finalized = true;
} // Context::finalize

HashUtil::MD5Context::MD5Context() : Context(EVP_md5()) {
} // MD5Context::MD5Context

HashUtil::Sha1Context::Sha1Context() : Context(EVP_sha1()) {
} // Sha1Context::Sha1Context

void HashUtil::computeFileHash(const std::string& path, std::string& h,
			       struct stat& sbuf, bool dostat, bool followlink,
			       const EVP_MD* md) throw(FileException) {
  ADD_SELECTORS("HashUtil::computeFileHash");
  assert(md);

  if (dostat) {
    FileUtil::lstatWithErr(path, sbuf);
  }

  if (S_ISDIR(sbuf.st_mode)) {
    throw BadFileTypeException("cannot compute hash on directory " + path);
  }

#ifdef S_ISLNK
  if (S_ISLNK(sbuf.st_mode)) {
    if (followlink) {
      if (dostat) {
	FileUtil::statWithErr(path, sbuf);
      }
    }
    else {
      std::string s = FileUtil::readlink(path);
      computeHash(s.data(), s.size(), h, md);
      return;
    }
  }
#endif

  bool retry = false;
  size_t count = 0;
  int fd = 0;
  char buf[BLOCK_SIZE];
  size_t len = 0;
  EVP_MD_CTX* mdctx = 0;
  do {
    count++;
    if (count > MAX_RETRY_COUNT) {
      throw FileException("could not hash file " + path + ": too many errors");
    }
    try {
      fd = FileUtil::open(path);
    }
    catch (const FileException& e) {
      Log::err() << e << Log::endl;
      retry = true;
      FileUtil::lstatWithErr(path, sbuf);
      continue;
    }

    mdctx = initCTX(md);

    int r = FileUtil::read(fd, buf, BLOCK_SIZE);
    while (r != 0) {
      if (r < 0) {
	Log::err() << "HashUtil::computeFileHash: error reading block from "
		   << path << ": " << Util::getErrorString(errno) << Log::endl;
	close(fd);
	freeCTX(mdctx);
	retry = true;
      }
      len += r;
      EVP_DigestUpdate(mdctx, buf, r);
      r = FileUtil::read(fd, buf, BLOCK_SIZE);
    }
    retry = false;
  } while (retry);

  close(fd);
  finalizeCTX(mdctx, h);
} // computeFileHash
