/* 
 * SysUtil.cc : part of the Mace toolkit for building distributed systems
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
#include <set>
#include <signal.h>
#include <unistd.h>
#include "SysUtil.h"
#include "SockUtil.h"
#include "TimeUtil.h"
#include "ScopedLog.h"
#include "Log.h"
#include "maceConfig.h"
#include "m_net.h"
#include "FileUtil.h"

int SysUtil::select(int max, fd_set* rfs, fd_set* wfs, fd_set* efs, uint64_t usec, bool restartOnEINTR) {
  if (usec) {
    struct timeval tv = { (time_t)(usec / 1000000), (suseconds_t)(usec % 1000000) };
    return select(max, rfs, wfs, efs, &tv, restartOnEINTR);
  }
  else {
    return select(max, rfs, wfs, efs, (struct timeval*)0, restartOnEINTR);
  }
}

int SysUtil::select(int max, fd_set* rfs, fd_set* wfs, fd_set* efs,
		    struct timeval* timeout, bool restartOnEINTR) {
  //   static const mace::string str = "SysUtil::select";
  //   static const log_id_t id = Log::getId(str);
  //   ScopedLog log(str, 0, id, (max == 0), (max == 0), (max == 0), false);
  int n = 0;
  uint64_t sleeptime = 0;
  if (timeout) {
    sleeptime = TimeUtil::timeu(*timeout);
  }

  do {
    uint64_t start = TimeUtil::timeu();
#if (defined _WINSOCK2_H || defined _WINSOCK_H)
    if (max == 0) {
      if (sleeptime == 0) {
        Sleep(INFINITE);
      }
      else {
        Sleep(sleeptime / 1000);
      }
    } else {
#endif
      n = ::select(max, rfs, wfs, efs, timeout);
#if (defined _WINSOCK2_H || defined _WINSOCK_H)
    }
#endif
    if (n < 0) {
      int err = SockUtil::getErrno();
      if (SockUtil::errorInterrupted(err)) {
        if (restartOnEINTR) {
          if (sleeptime != 0) {
            uint64_t now = TimeUtil::timeu();
            uint64_t diff = now - start;
            if (diff < sleeptime) {
              sleeptime -= diff;
              TimeUtil::fillTimeval(sleeptime, *timeout);
            }
            else {
              // this really should never happen
              Log::warn() << "WARNING: select was interrupted "
                "after it should have timed-out" << Log::end;
              return 0;
            }
          }
        } else {
          if (rfs) {
            FD_ZERO(rfs);
          }
          if (wfs) {
            FD_ZERO(wfs);
          }
          if (efs) {
            FD_ZERO(efs);
          }
          return 0;
        }
      }
      else if (SockUtil::errorBadF(err)) {
	Log::err() << "select returned EBADF" << Log::end;
	if (rfs) {
	  FD_ZERO(rfs);
	}
	if (wfs) {
	  FD_ZERO(wfs);
	}
	if (efs) {
	  FD_ZERO(efs);
	}
	return 0;
      }
      else {
	Log::perror("select");
	ABORT("select");
      }
    }
    else if (n >= 0) {
      return n;
    }
  } while (true);
} // select

void SysUtil::sleep(time_t sec, useconds_t usec) {
  if (sec > 0 || usec > 0) {
    struct timeval tv = { sec, usec };
    select(0, 0, 0, 0, &tv);
  }
  else {
    select(0, 0, 0, 0, (struct timeval*)0);
  }
} // sleep


sighandler_t SysUtil::signal(int signum, sighandler_t handler, bool warn) {
  static std::set<int> registeredSignals;

  if (registeredSignals.find(signum) != registeredSignals.end()) {
    if (warn) {
      Log::warn() << "WARNING: signal handler already registered for " << signum
		  << Log::endl;
    }
    return handler;
  }
  
  registeredSignals.insert(signum);

#ifdef HAVE_SIGACTION
  struct sigaction act;
  struct sigaction oact;
  memset(&act, 0, sizeof(act));
  memset(&oact, 0, sizeof(oact));

  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_RESTART;
  act.sa_handler = handler;

  assert(sigaction(signum, &act, &oact) == 0);
  return oact.sa_handler;
#else
  return signal(signum, handler);
#endif
} // signal

int SysUtil::daemon(int nochdir, int noclose) {
#ifdef OSX_MACELIB_DAEMON
  // implementation adapted from glibc daemon.c
  switch (fork()) {
  case -1:
    return -1;
  case 0:
    break;
  default:
    exit(0);
  }

  if (setsid() == -1) {
    return -1;
  }

  if (!nochdir) {
    FileUtil::chdir("/");
  }

  if (!noclose) {
    ::close(STDIN_FILENO);
    ::close(STDOUT_FILENO);
    ::close(STDERR_FILENO);
    int fd = FileUtil::open("/dev/null", O_RDWR);
    ASSERT(fd == STDIN_FILENO);
    ::dup2(fd, STDOUT_FILENO);
    ::dup2(fd, STDERR_FILENO);
  }
  return 0;
#else
  return ::daemon(nochdir, noclose);
#endif
}
