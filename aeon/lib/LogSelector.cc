/* 
 * LogSelector.cc : part of the Mace toolkit for building distributed systems
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
#include <cstring>
#include <sys/time.h>
// #include <pthread.h>
#include "Log.h"
#include "LogSelector.h"
// #include "NumberGen.h"
#include "ScopedTimer.h"
#include "StrUtil.h"

using namespace std;

LogSelector::ThreadIdMap LogSelector::threadIds;
const std::string* LogSelector::prefix = NULL;

LogSelector::LogSelector(const string& n,
			 FILE* f, 
			 LogSelectorTimestamp lts,
			 LogSelectorName ln,
			 LogSelectorThreadId ltid,
                         LogSelectorOutput lso) :
  name(n),
  fp(f),
  fn(fileno(fp)),
  logTimestamp(lts),
  logName(ln),
  logThreadId(ltid), 
  logOutput(lso) {
} // LogSelector

LogSelector::~LogSelector() {
} // ~LogSelector

// string LogSelector::getName() const {
//   return name;
// } // getName

extern uint64_t tidtimer;
extern uint64_t gtdtimer;
extern uint64_t lntimer;

string LogSelector::getSelectorHeader(uint64_t ts, uint32_t vtid) const {
  time_t sec = 0;
  int usec = 0;
  if (ts) {
    sec = ts / 1000000;
    usec = ts % 1000000;
  }
  else {
    struct timeval clock_;
    struct timezone tz_;
    gettimeofday(&clock_, &tz_);
    sec = clock_.tv_sec;
    usec = clock_.tv_usec;
  }
  uint32_t tid = vtid ? vtid : Log::ThreadSpecific::getVtid();
  char tmp[64];
  if (logTimestamp == LOG_TIMESTAMP_HUMAN) {
    string ct = ctime(&sec);
    ct = ct.substr(0, ct.size() - 1);
    if (logThreadId == LOG_THREADID_ENABLED) {
      snprintf(tmp, 63, "[%s] %02d ", ct.c_str(), tid);
    }
    else {
      snprintf(tmp, 63, "[%s] ", ct.c_str());
    }
  }
  else if (logTimestamp == LOG_TIMESTAMP_EPOCH) {
    if (logThreadId == LOG_THREADID_ENABLED) {
      snprintf(tmp, 63, "%d.%06d %02d ", (int)sec, usec, tid);
    }
    else {
      snprintf(tmp, 63, "%d.%06d ", (int)sec, usec);
    }
  }
  else if (logThreadId == LOG_THREADID_ENABLED) {
    snprintf(tmp, 63, "%02d ", tid);
  }
  else {
    tmp[0] = '\0';
  }

  string r(tmp);

  if (prefix &&
      (logTimestamp != LOG_TIMESTAMP_DISABLED ||
       logThreadId == LOG_THREADID_ENABLED ||
       logName == LOG_NAME_ENABLED)) {
    r = *prefix + r;
  }

  if (logName == LOG_NAME_ENABLED) {
    r += "[";
    r += name;
    r += "] ";
  }

  return r;

//   static int count = 0;
//   string r = (prefix &&
// 	      (logTimestamp != LOG_TIMESTAMP_DISABLED ||
// 	       logThreadId == LOG_THREADID_ENABLED ||
// 	       logName == LOG_NAME_ENABLED))?*prefix:"";

//   if (logTimestamp != LOG_TIMESTAMP_DISABLED) {
//     ScopedTimer st(gtdtimer, true);
//     struct timeval clock_;
//     struct timezone tz_;
//     gettimeofday(&clock_, &tz_);

//     if (logTimestamp == LOG_TIMESTAMP_HUMAN) {
//       r += "[";
//       r += ctime((const time_t*)&(clock_.tv_sec));
//       // chomp last character
//       r = r.substr(0, r.size() - 1);
//       r += "] ";
//     }
//     else if (logTimestamp == LOG_TIMESTAMP_EPOCH) {
// //      double now = clock_.tv_sec + (double)clock_.tv_usec/1000000.0;
//       char tmp[32];
//       snprintf(tmp, 31, "%d.%06d ", (int)clock_.tv_sec, (int)clock_.tv_usec);
//       r += tmp;
//     }
//   }


//   if (logThreadId == LOG_THREADID_ENABLED) {
//     ScopedTimer st(tidtimer, true);
//     unsigned int tid = Log::ThreadSpecific::getVtid();
// //     unsigned int tid = Log::ThreadSpecific::getVid();
// //     ThreadIdMap::iterator i = threadIds.find(selftid);
// //     if (i == threadIds.end()) {
// //       // this seems to cause infinite recursion
// // //       threadIds[selftid] = NumberGen::Instance("LOG_SELECTOR_THREAD_ID")->GetVal();
// //       count++;
// //       threadIds[selftid] = count;
// //       tid = count;
// //     } else {
// //       tid = i->second;
// //     }
//     char tmp[8];
//     snprintf(tmp, 7, "%02d ", tid);
//     r += tmp;
//   }
    
//   if (logName == LOG_NAME_ENABLED) {
//     ScopedTimer st(lntimer, true);
//     r += "[";
//     r += name;
//     r += "] ";
//   }
  
//   return r;
} // getSelectorHeader

// return a serialized format of the selector header
string LogSelector::getSerializedSelectorHeader() const {
  string r = "";
  struct timeval clock_;
  struct timezone tz_;
  gettimeofday(&clock_, &tz_);

  double now = clock_.tv_sec + (double)clock_.tv_usec/1000000.0;
  mace::serialize(r, &now);

  unsigned int tid = Log::ThreadSpecific::getVtid();
  mace::serialize(r, &tid);

  return r;
} // getSerializedSelectorHeader


// FILE* LogSelector::getFile() const {
//   return fp;
// } // getFile

// LogSelectorTimestamp LogSelector::getLogTimestamp() const {
//   return logTimestamp;
// } // getLogTimestamp

// LogSelectorName LogSelector::getLogName() const {
//   return logName;
// } // getLogName

// LogSelectorThreadId LogSelector::getLogThreadId() const {
//   return logThreadId;
// } // getLogThreadId

// LogSelectorOutput LogSelector::getLogSelectorOutput() const { 
//   return logOutput; 
// } // getSelectorOutput

