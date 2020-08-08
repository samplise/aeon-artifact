/* 
 * LoadMonitor.cc : part of the Mace toolkit for building distributed systems
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
// #include <unistd.h>
// #include <stdio.h>
#include <sys/types.h>
#include <inttypes.h>
// #include <sys/stat.h>
// #include <fcntl.h>

#include <boost/lexical_cast.hpp>

#include "LoadMonitor.h"
// #include "ThreadCreate.h"
#include "Log.h"
#include "TimeUtil.h"
#include "massert.h"
#include "params.h"
// #include "SysUtil.h"


using namespace std;

LoadMonitor* LoadMonitor::instance = 0;
HiResLoadMonitor* HiResLoadMonitor::instance = 0;
const std::string HiResLoadMonitorLogObject::type = "HRLM";
mace::LogNode* HiResLoadMonitorLogObject::rootLogNode = 0;

HiResLoadMonitor::HiResLoadMonitor(uint64_t freq) : halt(false), frequency(freq) {
  pid = getpid();
  path = "/proc/" + boost::lexical_cast<string>(pid) + "/stat";

  statfile.open(path.c_str());
  if (!statfile.good()) {
    Log::perror("open");
    assert(0);
  }
} // HiResLoadMonitor

void HiResLoadMonitor::expire() {
  assert(instance);
  uint64_t now = TimeUtil::timeu();
  ProcStat st;
  readProcStat(st);
  uint64_t t = now - instance->prevTime;
  
  double pu = (st.utime - instance->stat.utime) * 10000.0 / t;
  double ps = (st.stime - instance->stat.stime) * 10000.0 / t;
  int iu = (int)(100 * pu);
  int is = (int)(100 * ps);
  int it = iu + is;
  Log::logf("HiResLoadMonitor", "%d %d %d %u %d", iu, is, it, st.vsize, st.rss);
  
  instance->stat = st;
  instance->prevTime = now;

  if (!halt) {
    schedule(frequency, false, true);
  }
} // expire

void HiResLoadMonitor::readProcStat(ProcStat& st) {
  statfile.seekg(0, ios::beg);
  string tmp;
  int i;
  uint u;
  statfile >> i // pid
	   >> tmp // comm (filename of executable)
	   >> tmp // state
	   >> i // ppid
	   >> i // pgrp
	   >> i // session
	   >> i // tty_nr
	   >> i // tpgid
	   >> i // flags
	   >> i // minflt
	   >> i // cminflt
	   >> i // majflt
	   >> i // camjflt
	   >> st.utime
	   >> st.stime
	   >> i // cutime
	   >> i // cstime
	   >> i // priority
	   >> i // nice
	   >> i // 0
	   >> i // itrealvalue
	   >> u // starttime
	   >> st.vsize
	   >> st.rss
    ;
} // readProcStat

void HiResLoadMonitor::start(uint64_t freq) {
  if (instance == 0) {
    instance = new HiResLoadMonitor(freq);
  }
  else {
    instance->frequency = freq;
  }

  instance->cancel();
  instance->schedule(instance->frequency, false, true);
  instance->prevTime = TimeUtil::timeu();
  instance->readProcStat(instance->stat);
} // start

void HiResLoadMonitor::stop() {
  instance->halt = true;
} // stop

void LoadMonitor::runLoadMonitor() {
  // Start the CPU load and usage monitor
  if (instance == 0) {
    instance = new LoadMonitor();
  }
  if (instance->loadfile.good() && instance->cpufile.good()) {
    instance->cancel();
    instance->schedule(instance->frequency);
  }
} // runLoadMonitor

void LoadMonitor::stopLoadMonitor() {
  instance->halt = true;
} // stopLoadMonitor

LoadMonitor::LoadMonitor() : halt(false), frequency(DEFAULT_FREQUENCY),
			     load(0), lastCPU_sum(0) {

  memset(curLoad, 0, sizeof(curLoad));
  memset(lastCPU, 0, sizeof(lastCPU));
  memset(curCPU, 0, sizeof(curCPU));
  memset(peakload, 0, sizeof(peakload));

  loadfile.open("/proc/loadavg");
  if (!loadfile.good()) {
    perror( "REPLAY error opening /proc/loadavg" );
    return;
  }

  cpufile.open("/proc/stat");
  if (!cpufile.good()) {
    perror( "REPLAY error opening /proc/stat" );
    return;
  }

  if( params::containsKey(params::MACE_LOAD_MONITOR_FREQUENCY_MS))
  {
    frequency = (int)(params::get<int>(params::MACE_LOAD_MONITOR_FREQUENCY_MS) * 1000);
  }

  Log::logf("LoadMonitor", "REPLAY CPU monitoring frequency is %" PRIu64 
	    " usecs.", frequency );

  string tmp;
  cpufile >> tmp >> lastCPU[0] >> lastCPU[1] >> lastCPU[2] >> lastCPU[3];
  assert(tmp == "cpu");
  cpufile.seekg(0, ios::beg);

  lastCPU_sum = lastCPU[0] + lastCPU[1] + lastCPU[2] + lastCPU[3];
} // LoadMonitor

void LoadMonitor::expire() {
  static string tmp;

  /* The buffer will be used to monitor both load and CPU usage. */

  loadfile >> curLoad[0] >> curLoad[1] >> curLoad[2];
  loadfile.seekg(0, ios::beg);

  Log::logf("LoadMonitor", "REPLAY CPU load: %.2f %.2f %.2f",
	    curLoad[0], curLoad[1], curLoad[2]);

  for(int i = 0; i < 3; ++i) {
    if (curLoad[i] > peakload[i]) {
      peakload[i] = curLoad[i];
    }
  }

  Log::logf("LoadMonitor", "Peak CPU load  : %.2f %.2f %.2f",
	    peakload[0], peakload[1], peakload[2]);

  cpufile >> tmp >> curCPU[0] >> curCPU[1] >> curCPU[2] >> curCPU[3];
  assert(tmp == "cpu");
  cpufile.seekg(0, ios::beg);

  uint32_t curCPU_sum = curCPU[0] + curCPU[1] + curCPU[2] + curCPU[3];

  float time_diff = (float) curCPU_sum - (float) lastCPU_sum;
  float user   = ((float) curCPU[0] - lastCPU[0]) / time_diff * 100;
  float nice   = ((float) curCPU[1] - lastCPU[1]) / time_diff * 100;
  float system = ((float) curCPU[2] - lastCPU[2]) / time_diff * 100;
  float idle   = ((float) curCPU[3] - lastCPU[3]) / time_diff * 100;

  //Log::logf("LoadMonitor", "last: %d %d %d %d %d", lastCPU[0], lastCPU[1],lastCPU[2],lastCPU[3],lastCPU_sum);
  //Log::logf("LoadMonitor", "cur: %d %d %d %d %d", curCPU[0], curCPU[1],curCPU[2],curCPU[3],curCPU_sum);

  Log::logf("LoadMonitor", "REPLAY CPU time: u %.2f %%  n %.2f %%  s %.2f %%  i %.2f %%",
	    user, nice, system, idle);

  lastCPU_sum = 0;
  for (size_t i = 0; i < 4; i++) {
    lastCPU[i] = curCPU[i];
    lastCPU_sum += lastCPU[i];
  }

  if (!halt) {
    schedule(frequency);
  }
} // expire

