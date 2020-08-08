/* 
 * SimTimeUtil.h : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, Charles Killian, James W. Anderson
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
#ifndef SIM_TIME_UTIL_H
#define SIM_TIME_UTIL_H

#include <sstream>

#include "Printable.h"
#include "Sim.h"
#include "SimEvent.h"
#include "TimeUtil.h"
#include "params.h"

static const uint64_t SIM_START_TIME = 1026583200000000ll; //date -d "July 13 14:00:00 EDT 2002" "+%s000000"

typedef mace::vector<uint64_t> UInt64List;

class SimTimeUtil : public TimeUtil, public mace::PrintPrintable {
  protected:
    static SimTimeUtil* realTimeUtil;

    UInt64List nodeTimes;

    uint64_t timeuImpl() {
      static const bool ALLOW_TIMEU = params::get("ALLOW_TIMEU", true);
      ASSERT(ALLOW_TIMEU);
      return nodeTimes[Sim::getCurrentNode()]++; //Ensure time always advances
    }
    uint64_t nodeTimeuImpl(int node) {
      return nodeTimes[node];
    }
    time_t timeImpl() {
      return nodeTimes[Sim::getCurrentNode()]/(1*1000*1000); //convert from microseconds to seconds
    }
    uint64_t realTimeuImpl() {
      return TimeUtil::timeuImpl();
    }

  public:
    static void addTime(int node, uint64_t time) {
      realTimeUtil->nodeTimes[node] += time;
    }
    static void addTime(uint64_t time) {
      addTime(Sim::getCurrentNode(), time);
    }
    static uint64_t realTimeu() {
      return realTimeUtil->realTimeuImpl();
    }
    static uint64_t nodeTimeu(int node) {
      return realTimeUtil->nodeTimeuImpl(node);
    }
    
    /// Add an event to the time-ordered list
    static void addEvent(uint64_t t, const Event& e) {
      EventList& pendingEvents = SimEvent::getEvents();
      ASSERT(pendingEvents.find(t) == pendingEvents.end()); // no two events share the same time
      pendingEvents.insert(std::make_pair(t, e));
    }

    /// Find an unoccupied time greater than t
    static uint64_t getNextTime(uint64_t t) {
      EventList& el = SimEvent::getEvents();
      while (el.containsKey(t)) {
        t++;
      }
      return t;
    }

    /// Find and return the event with smallest time
    static Event beginNextEvent() {
      EventList& pendingEvents = SimEvent::getEvents();
      
      EventList::iterator i = pendingEvents.begin();
      ASSERT(i != pendingEvents.end());
      
      Event e = i->second;
      uint64_t cur = nodeTimeu(e.node);
      if (cur < i->first) {
        addTime(e.node, i->first-cur);
      }
      
      pendingEvents.erase(i);
      return e;
    }

    static void SetInstance() {
      if (_inst != NULL) { delete _inst; }
      _inst = realTimeUtil = new SimTimeUtil();
    }

    static mace::Printable& PrintableInstance() {
      return *realTimeUtil;
    }
  
    static void getTimeStats(uint64_t& min, uint64_t& max, uint64_t& avg) {
      min = std::numeric_limits<uint64_t>::max();
      max = std::numeric_limits<uint64_t>::min();
      avg = 0;

      for (uint i = 0; i < realTimeUtil->nodeTimes.size(); i++) {
	uint64_t nodeTime = realTimeUtil->nodeTimes[i] - SIM_START_TIME;
	if (nodeTime < min) {
	  min = nodeTime;
	}
	if (nodeTime > max) {
	  max = nodeTime;
	}
	avg += nodeTime;
      }
      avg /= realTimeUtil->nodeTimes.size();
    }

    static std::string getTimeStats() {
      uint64_t min, max, avg;
      getTimeStats(min, max, avg);

      std::ostringstream out;
      out << "avg: " << avg << " min: " << min << " max: " << max;
      return out.str();
    }
    
    void print(std::ostream& out) const {
      EventList& pendingEvents = SimEvent::getEvents();

      out << "times(" << nodeTimes << ")" << std::endl;
      out << "events(size=" << pendingEvents.size() << ", ";
      EventList::const_iterator e = pendingEvents.begin();
      for (int i = 0; i < 30 && e != pendingEvents.end(); i++, e++) {
        out << "( " << e->first << " -> " << e->second << " )";
      }
      out << ")";
    }

  private:
    SimTimeUtil() {
      nodeTimes.resize(Sim::getNumNodes(), SIM_START_TIME);
    }    
    
};

#endif // SIM_TIME_UTIL_H
