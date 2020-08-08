/* 
 * SimSchedulerCommon.h : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, Charles Killian, James W. Anderson, Karthik Nagaraj
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
#ifndef SIM_SCHED_COMMON_H
#define SIM_SCHED_COMMON_H

#include "Iterator.h"
#include "MaceTypes.h"
#include "ReceiveDataHandler.h"
#include "Scheduler.h"
#include "SimCommon.h"
#include "SimEvent.h"
#include "SimulatorTransport.h"
#include "Util.h"
#include "m_map.h"
#include "mace-macros.h"
#include "mvector.h"

/**
 * Simulated class that handles timers scheduled from within Mace services. A
 * given timer's handler is stored as a pointer in a pending event list, for
 * later execution when picked.
 */
class SimSchedulerCommon : public SimCommon, public Scheduler {
  protected:
    SimSchedulerCommon() : SimCommon(), Scheduler() { }
    void runSchedulerThread() {}
  public:
    uint32_t hashState() const { return 0; }
    void printState(std::ostream& out) const {
      return;
    }
    void print(std::ostream& out) const {
      //       out << std::endl;
      //       for (TimerList::const_iterator i = scheduledTimers.begin();
      //           i != scheduledTimers.end(); i++) {
      //         const Event& e = *i;
      //         out << e.node << ": " << e.desc << std::endl;
      //       }
      return;
    }
    void reset() { }
    
    /// Schedule the timer using the handler for the future
    uint64_t schedule(TimerHandler& timer, uint64_t time, bool abs = false) = 0;
    
    /// Cancel the timer and remove it from pending events.
    void cancel(TimerHandler& timer) = 0;
    bool isSimulated() { return true; }
    
    bool hasEventsWaiting() const {
      // Check the event queue for any NETWORK event
      const EventList& eventList = SimEvent::getEvents();
      for (EventList::const_iterator ev = eventList.begin();
           ev != eventList.end(); ++ev) {
        if (ev->second.type == Event::SCHEDULER) {
          return true;
        }
      }
      return false;
    }

    /// Execute the given timer event. i.e. Fire in the hole!
    std::string simulateEvent(const Event& e) {
      ADD_SELECTORS("SimScheduler::simulateEvent");
      
      TimerHandler* timer = (TimerHandler*)e.simulatorVector[0];
      maceout << "simulating timer for node " << e.node << " " << timer->getDescription() << Log::endl;
      timer->fire();
      
      std::ostringstream r;
      r << timer->getDescription() << "(" << timer->getId() << ")";
      return r.str();
    }
    virtual ~SimSchedulerCommon() { }
};

#endif // SIM_SCHED_COMMON_H
