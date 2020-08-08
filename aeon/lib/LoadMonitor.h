/* 
 * LoadMonitor.h : part of the Mace toolkit for building distributed systems
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
#ifndef __LOAD_MONITOR__H
#define __LOAD_MONITOR__H
// #include <pthread.h>
#include <fstream>
#include <cassert>
#include "Scheduler.h"

/**
 * \file LoadMonitor.h
 * \brief defines LoadMonitor and HiResLoadMonitor
 */

/**
 * \addtogroup Monitors
 * @{
 */

/// For monitoring CPU usage
class HiResLoadMonitor : public TimerHandler {
public:
  class ProcStat {
  public:
    ProcStat() : utime(0), stime(0), vsize(0), rss(0) { }
    uint utime;
    uint stime;
    uint vsize;
    int rss;
  }; // ProcStat

  void expire();
  static void start(uint64_t freq = DEFAULT_FREQUENCY); ///< start the CPU monitor logger with the given frequency in microseconds
  static void stop(); ///< stop the CPU monitor 

public:
  static const uint64_t DEFAULT_FREQUENCY = 1*1000*1000; ///< default CPU monitor frequency

private:
  HiResLoadMonitor(uint64_t freq);
  void readProcStat(ProcStat& stat);

private:
  static HiResLoadMonitor* instance;
  bool halt;
  uint64_t frequency;
  uint64_t prevTime;
  pid_t pid;
  ProcStat stat;
  std::ifstream statfile;
  std::string path;
}; // HiResLoadMonitor

/// For monitoring uptime
class LoadMonitor : public TimerHandler {
public:
  LoadMonitor();
  void expire(); 
  static void runLoadMonitor(); ///< start the load monitor logger
  static void stopLoadMonitor(); ///< stop the load monitor
  static double getLoad() { return instance ? instance->curLoad[0] : 0; } ///< get the last load measured

public:
  static const uint64_t DEFAULT_FREQUENCY = 5*1000*1000; ///< default timer period for the load monitor in microseconds

private:
  static LoadMonitor* instance;
  bool halt;
  uint64_t frequency;
  std::ifstream loadfile;
  std::ifstream cpufile;
  double load;
  double curLoad[3];
  uint32_t lastCPU[4];
  uint32_t curCPU[4];
  double peakload[3];
  uint32_t lastCPU_sum;

  
};

class HiResLoadMonitorLogObject : public mace::BinaryLogObject, public mace::PrintPrintable {
public:
  static const std::string type;
  int user;
  int system;
  int total;
  int vsize;
  int rss;

  HiResLoadMonitorLogObject() : user(0), system(0), total(0), vsize(0), rss(0) { }
  HiResLoadMonitorLogObject(int u, int s, int t, int v, int r) :
    user(u), system(s), total(t), vsize(v), rss(r) { }

protected:
  static mace::LogNode* rootLogNode;

public:
  void serialize(std::string& __str) const {
  }
    
  int deserialize(std::istream& in) throw(mace::SerializationException) {
    return 0;
  }

  void sqlize(mace::LogNode* node) const {
  }

  void print(std::ostream& __out) const {
  }

  const std::string& getLogType() const {
    return type;
  }

  const std::string& getLogDescription() const {
    static const std::string desc = "HiResLoadMonitor";
    return desc;
  }

  LogClass getLogClass() const {
    return OTHER;
  }
  
  mace::LogNode*& getChildRoot() const {
    return rootLogNode;
  }
};

/** @} */

#endif
