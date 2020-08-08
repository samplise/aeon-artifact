/* 
 * SysUtil.h : part of the Mace toolkit for building distributed systems
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
#include "maceConfig.h"
// #include <signal.h>
// #include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include "m_net.h"
// #include <cstring>
// #include <cstdlib>
// #include <cassert>
// #include <cerrno>
// #include <cstdio>

#ifndef SYS_UTIL_H
#define SYS_UTIL_H

/**
 * \file SysUtil.h
 * \brief declares system helper functions for sleeping, selecting, and installing signal handlers
 */

// sighandler_t is a GNU extension, define here for Cygwin
typedef void (*sighandler_t)(int);

/**
 * \addtogroup Utils
 * @{
 */

/// System utility class for sleeping, selecting, and installing signal handlers
class SysUtil {
public:
  /**
   * \brief Call system select with various sets.
   *
   * Note about Windows implementation: in the Winsock API, it is invalid to
   * use select to sleep only (without passing in any file descriptors).  On
   * Windows, if \c max is 0, the implementation calls OS sleep instead.
   *
   * @param max the largest file descriptor (plus one)
   * @param rfs the read fd_set (set of fds to wait for readability)
   * @param wfs the write fd_set (set of fds to wait for writability)
   * @param efs the error fd_set (set of fds to wait for error signals)
   * @param timeout the timeout for select (0 means forever)
   * @param restartOnEINTR whether to restart the sleep if the sleep is interrupted
   * @return the number of file descriptors in the output fd_sets.
   */
  static int select(int max = 0, fd_set* rfs = 0, fd_set* wfs = 0, fd_set* efs = 0,
		    struct timeval* timeout = 0, bool restartOnEINTR = true);
  /// constructs a timeval, and calls select(int, fd_set*, fd_set*, fd_set*, struct timeval*, bool)
  static int select(int max, fd_set* rfs, fd_set* wfs, fd_set* efs, uint64_t timeout, bool restartOnEINTR = true);

  /// Install signal handler \c handler for \c signum, warn on duplicate if \c warn
  static sighandler_t signal(int signum, sighandler_t handler, bool warn = true);

  /// Sleep an optional number of seconds and (< 1000000) microsecods (0 means infinite)
  static void sleep(time_t sec = 0, useconds_t usec = 0);
  /// Sleep a number of microseconds (0 means infinite)
  static void sleepu(uint64_t usec) { if (usec > 1000000) { SysUtil::sleep(usec / 1000000, usec % 1000000); } else { SysUtil::sleep(0, usec); } }
  /// Sleep a number of milliseconds (0 means infinite)
  static void sleepm(useconds_t msec) { if (msec > 1000) { SysUtil::sleep(msec / 1000, (msec % 1000) * 1000); } else { SysUtil::sleep(0, msec * 1000); } }
  static int daemon(int nochdir, int noclose);
}; // SysUtil

/** @} */

#endif // SYS_UTIL_H
