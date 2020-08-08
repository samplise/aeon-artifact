/* 
 * PipedSignal.h : part of the Mace toolkit for building distributed systems
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
#ifndef _PIPED_SIGNAL_H
#define _PIPED_SIGNAL_H

// #include "mace-macros.h"
#include "SockUtil.h"
#include "SysUtil.h"
#include "FileUtil.h"
#include "Log.h"
#include "maceConfig.h"

#ifndef HAVE_PIPE
#include "Util.h"
#include "Exception.h"
#include "ThreadCreate.h"
#include "NumberGen.h"
#include "m_net.h"
#endif

/**
 * \file PipedSignal.h
 * \brief defines the PipedSignal class
 */

/**
 * \brief Used for lock-free signalling of threads, and to allow a select thread to be awoken by signal.
 *
 * Two common uses for PipedSignal.  First, as a basic mutex/condition variable
 * style signal, and later as a second as a way to awaken threads in select.
 *
 * First (condition variable style signal):
 * \code
 * PipedSignal p;
 *
 * // thread 1:
 * p.wait();
 *
 * // thread 2:
 * p.signal();
 * \endcode
 *
 * Second (used for select):
 * \code
 * PipedSignal p;
 *
 * // thread 1:
 * fd_set rset;
 * int selectMax = 0;
 * // add any number of other sockets to select for reading on.
 * p.addToSet(rset, selectMax);
 * SysUtil::select(selectMax, rset);
 * p.clear(rset);
 *
 * // thread 2:
 * p.signal();
 * \endcode
 *
 * PipedSignal provides challenges for portability.  On Linux, you can select
 * on a pipe, allowing a simple version which in the constructor creates a pipe
 * and uses it for signalling and waiting.
 *
 * However, on Windows, you cannot select on a pipe.  To overcome this, the
 * Windows implementation spawns a new thread, waits for it to connect a socket
 * over localhost, and then the new thread dies.  Additionally, a new port
 * number series must be used to allocate the PipedSignal server sockets.
 *
 * A separate implemenation was briefly devised on Windows (kept in a compiled
 * out version here) which supports only the first model.  It does so by
 * actually using a mutex and condition variable with pthread_cond_timed_wait.
 * The primary use of the select version is in the Transport scheduler, to wake
 * the scheduler thread.  This can be worked around with a smaller timeout on
 * select.
 */
class PipedSignal 
#ifdef HAVE_PIPE
{
#else 
: public RunThreadClass {
  friend void _runNewThread(pthread_t* t, func f, void* arg, 
			    pthread_attr_t* attr, const char* fname, bool joinThread);
  friend void _runNewThreadClass(pthread_t* t, RunThreadClass* c, classfunc f,
				 void* arg, pthread_attr_t* attr, 
				 const char* fname, bool joinThread);
  friend void* connectSignal(void* vfa);
private:
  int16_t port;

  void* connectSignal(void* arg) {
    w = ::socket(AF_INET, SOCK_STREAM, 0);
    ASSERT(w >= 0);
    struct sockaddr_in sa;
    uint32_t bindaddr = ntohl(INADDR_LOOPBACK);
    SockUtil::fillSockAddr(bindaddr, port, sa);
    if (::connect(w, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
      Log::perror("connect");
      ABORT("connect");
    }
    return NULL;
  }
#endif

public:
  PipedSignal() {
    #ifdef HAVE_PIPE
    int des[2];
    if(pipe(des) < 0) {
      Log::perror("pipe");
      ASSERT(0);
    }
    r = des[0];
    w = des[1];
    #else
    SockUtil::init();
    socket_t sv = socket(AF_INET, SOCK_STREAM, 0);
    if (sv < 0) {
      Log::perror("socket");
      ABORT("socket");
    }

    int n = 1;
    if (setsockopt(sv, SOL_SOCKET, SO_REUSEADDR, (char*)&n, sizeof(n)) < 0) {
      Log::perror("setsockopt");
      ABORT("setsockopt");
    }

    struct sockaddr_in sa;
    uint32_t bindaddr = ntohl(INADDR_LOOPBACK);
    ASSERT(params::containsKey("PIPED_SIGNAL_SOCKET_PORT"));
    port = NumberGen::Instance("PIPED_SIGNAL_SOCKET_PORT")->GetVal();
    SockUtil::fillSockAddr(bindaddr, port, sa);
    
    if (bind(sv, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
      Log::perror("bind");
      ::close(sv);
    }
    if (listen(sv, 1) < 0) {
      Log::perror("listen");
      ABORT("listen");
    }

    sockaddr_in sin;
    socklen_t sinlen = sizeof(sin);
    memset(&sin, 0, sinlen); 
    pthread_t pt;
    _runNewThreadClass(&pt, this, (classfunc)&PipedSignal::connectSignal, 0, 0, "PipedSignal::constructor", false);
    r = ::accept(sv, (struct sockaddr*)&sin, &sinlen);
    ASSERT(r > 0);
    pthread_join(pt, NULL);
    ::close(sv);

    #endif
    SockUtil::setNonblock(r);
    SockUtil::setNonblock(w);
    FD_ZERO(&rset);
  }
  
  virtual ~PipedSignal() {
    close();
  }

  /// adds the PipedSignal read fd to the fd_set \c s, and updates selectMax if needed.
  void addToSet(fd_set& s, socket_t& selectMax) const {
    //     Log::log("PipedSignal::addToSet") << "Adding socket " << r << " to fdset" << Log::endl;
    FD_SET(r, &s);
    selectMax = std::max(selectMax, r);
  }

  /// if using addToSet, after select call clear with the resulting read set to read bytes from it.
  bool clear(fd_set& s) {
    bool ret = FD_ISSET(r, &s);
    if (ret) {
      read(s);
    }
    FD_CLR(r, &s);
    return ret;
  }

  /// used to awaken a piped signal thread
  /**
   * an interesting side effect of using PipedSignal is that the signal() can
   * be called before wait(), and then wait() will return immediately as there
   * should be bytes to read.
   */
  void signal() {
    #ifdef HAVE_PIPE
    if (::write(w, " ", 1) < 0) {
      if (!SockUtil::errorWouldBlock()) {
        Log::perror("PipedSignal::signal");
      }
    }
    #else
    ::send(w, " ", 1, 0);
    #endif
  }

  /// wait for the read fd to be signalled, with an optional timeout (0 means sleep forever)
  int wait(uint64_t usec = 0) {
    //     Log::log("PipedSignal::wait") << "Waiting for " << usec << Log::endl;
    FD_SET(r, &rset);
    int n = SysUtil::select(r + 1, &rset, 0, 0, usec);
    if (n) {
      read(rset);
    }
    //     Log::log("PipedSignal::wait") << "Done waiting for " << usec << Log::endl;
    return n;
  }

  /// cleans up the state from the PipedSignal
  void close() {
    ::close(r);
    ::close(w);
  }

private:
  inline void read(fd_set& rs) {
    #ifdef HAVE_PIPE
    ASSERT(FD_ISSET(r, &rs));
    FileUtil::read(r);
    #else
    int ret = 0;
    char buf[1024];
    do {
      ret = ::recv(r, buf, 1024, 0);
    } while (ret > 0);
    #endif
  } 

private:
  socket_t r;
  socket_t w;
  fd_set rset;
}; // PipedSignal

#if 0 

// Using Windows CreatePipe and WaitForObject
#include "ScopedLock.h"
#include "TimeUtil.h"

class PipedSignal {
public:
  PipedSignal() : waiting(false), signalled(false), mutex(PTHREAD_MUTEX_INITIALIZER), cond(PTHREAD_COND_INITIALIZER) {
   
  }
  
  virtual ~PipedSignal() {
  }

  void signal() {
    ScopedLock sl(mutex);
    if (!signalled) {
      signalled = true;
      if (waiting) {
        pthread_cond_signal(&cond);
      }
    }
  }

  int wait(uint64_t usec = 0) {
    struct timeval t;
    struct timespec ts;
    uint64_t fintime = TimeUtil::timeu() + usec;
    TimeUtil::fillTimeval(fintime, t);
    ts.tv_sec = t.tv_sec;
    ts.tv_nsec = t.tv_usec * 1000;
    ScopedLock sl(mutex);
    if (!signalled ) {
      waiting = true;
      pthread_cond_timedwait(&cond, &mutex, &ts);
    }
    waiting = false;
    signalled = false;
    return 0;
  }

private:
  bool waiting;
  bool signalled;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
}; // PipedSignal


#endif

#endif // _PIPED_SIGNAL_H
