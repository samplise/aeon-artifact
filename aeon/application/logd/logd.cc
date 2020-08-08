/* 
 * logd.cc : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, James W. Anderson
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
#include <signal.h>
#include "CircularBuffer.h"
#include "SysUtil.h"
#include "FileUtil.h"
#include "params.h"

/**
 * Note added by Chip.
 *
 * This application looks to be used to log just the last portion of output to
 * a logfile.  It reads from standard input, so this is intended to be used in
 * a pipe with the preceeding process.  This approach of course still requires
 * the source process to write output to standard output.  Not sure if this is
 * more efficient than putting the circular buffer in the source process.  But:
 * that approach would not work if the source process were to die abruptly
 * (someone needs to write out the log).
 */

using namespace std;

bool halt = false;
int sigcount = 0;

void shutdownHandler(int sig) {
  sigcount++;
  if (sigcount > 2) {
    halt = true;
  }
} // shutdownHandler

int main(int argc, char* argv[]) {
  params::addRequired("logfile", "path for log file");
  params::addRequired("size", "maximum log size in MB");
  params::loadparams(argc, argv);

  SysUtil::signal(SIGINT, &shutdownHandler);
  SysUtil::signal(SIGTERM, &shutdownHandler);
  SysUtil::signal(SIGPIPE, &shutdownHandler);

  uint32_t size = params::get<uint32_t>("size");
  size *= 1024 * 1024;

  CircularBuffer buf(size);

  const uint32_t BUFLEN = 4096;
  char rbuf[BUFLEN];

  int infd = fileno(stdin);

  int r = 0;
  do {
    r = FileUtil::read(infd, rbuf, BUFLEN);
    if (r > 0) {
      buf.write(rbuf, r);
    }
  } while (!halt && r > 0);

  string path = params::get<string>("logfile");
  int fd = FileUtil::create(path, S_IRUSR|S_IWUSR);
  buf.copy(fd);
  FileUtil::close(fd);
}
