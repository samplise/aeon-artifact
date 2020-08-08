/* 
 * SimEventWeighted.h : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, Karthik Nagaraj
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
#ifndef SIM_EVENT_WEIGHTED_H
#define SIM_EVENT_WEIGHTED_H

#include "SimEvent.h"
#include "SimulatorCommon.h"

/**
 * Modelchecker specific class for holding pending events, picking the next
 * event to execute, etc.
 */
class SimEventWeighted : public mace::PrintPrintable {
  private:
    static SimEventWeighted *_inst;
    int totalWeight; // current sum of all weights. Avoids runtime overhead
    SimEventWeighted() : totalWeight(0) { }

  protected:
    Event getNextEventInternal() {
      EventList& pendingEvents = SimEvent::getEvents();

      ASSERT(pendingEvents.size());

      EventList::iterator nextEventIterator =
          macesim::__SimulatorCommon__::randomUtil->randEvent(pendingEvents, totalWeight);

      Event nextEvent = nextEventIterator->second;
      totalWeight -= nextEventIterator->first;
      
      pendingEvents.erase(nextEventIterator); 
      
      return nextEvent;
    }

    void addEventInternal(int weight, const Event& ev) {
      EventList& pendingEvents = SimEvent::getEvents();
      pendingEvents.insert(std::make_pair(weight, ev));
      totalWeight += weight;
    }

    void removeEventInternal(EventList::iterator ev) {
      totalWeight -= ev->first;
      SimEvent::getEvents().erase(ev);
    }

    void print(std::ostream& out) const {
      EventList& pendingEvents = SimEvent::getEvents();
      out << "events(size=" << pendingEvents.size() << ", ";
      out << "totalWeight=" << totalWeight << ", " << std::endl;
      EventList::const_reverse_iterator e = pendingEvents.rbegin();
      //for (int i = 0; i < 30 && e != pendingEvents.end(); i++, e++) {
      for (; e != pendingEvents.rend(); ++e) {
        out << "( " << e->first << " -> " << e->second << " )" << std::endl;
      }
      out << ")";
    }
    
  public:
    /// Pick the next event to simulate in a Weighted-Random manner
    static Event getNextEvent() {
      return _inst->getNextEventInternal();
    }

    /// Add an event to the list
    static void addEvent(int weight, const Event& ev) {
      _inst->addEventInternal(weight, ev);
    }

    static void removeEvent(EventList::iterator ev) {
      _inst->removeEventInternal(ev);
    }
    
    static mace::Printable& PrintableInstance() {
      return *_inst;
    }
    
    static void SetInstance() {
      if (_inst != NULL) { delete _inst; }
      _inst = new SimEventWeighted();
    }
};

#endif // SIM_EVENT_WEIGHTED_H
