/* 
 * SimScheduler.h : part of the Mace toolkit for building distributed systems
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
#ifndef SIM_SCHED_H
#define SIM_SCHED_H

#include "SimSchedulerCommon.h"
#include "SimTimeUtil.h"

class SimScheduler : public SimSchedulerCommon {
  protected:
    static SimScheduler* _sim_inst;
    SimScheduler() : SimSchedulerCommon() { }

  public:
    uint64_t schedule(TimerHandler& timer, uint64_t time, bool abs = false) {
      ADD_FUNC_SELECTORS;
      int current = getCurrentNode();
      if (!abs) {
        time += SimTimeUtil::nodeTimeu(current);
      }
      uint64_t newTime = SimTimeUtil::getNextTime(time);
      Event ev;
      ev.node = current;
      ev.type = Event::SCHEDULER;
      ev.simulatorVector.push_back((intptr_t)(&timer));
      ev.desc = timer.getDescription();
      maceout << "Scheduling timer " << timer.getDescription() << " for node " << current << Log::endl;
      SimTimeUtil::addEvent(newTime, ev);
      return time;
    }

    void cancel(TimerHandler& timer) {
      ADD_FUNC_SELECTORS;
      //       maceout << "Cancelling timer " << &timer << " for node " << current << Log::endl;
      
      EventList& events = SimEvent::getEvents();
      EventList::iterator i;
      for (i = events.begin(); i != events.end(); i++) {
        if (i->second.type == Event::SCHEDULER && i->second.simulatorVector[0] == (intptr_t)&timer) {
          break;
        }
      }

      if (i != events.end()) {
        events.erase(i);
      } else {
        maceerr << "Removing unknown timer";
      }
    }

    static SimScheduler& Instance() { 
      if (_sim_inst == NULL) {
        _sim_inst = new SimScheduler();
      }
      if (scheduler == NULL) {
        scheduler = _sim_inst;
      }
      return *_sim_inst; 
    } 
    virtual ~SimScheduler() { }
};

#endif // SIM_SCHED_H
