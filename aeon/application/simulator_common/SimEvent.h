/* 
 * SimEvent.h : part of the Mace toolkit for building distributed systems
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
#ifndef SIM_EVENT_H
#define SIM_EVENT_H

#include "mmultimap.h"
#include "mvector.h"

/**
 * Defines a simulator event using node, type and optional params.
 */
struct Event : public mace::PrintPrintable {
  public:
    enum EventType { APPLICATION, NETWORK, SCHEDULER };

  public:
    int node;
    EventType type;
    mace::string desc;
  
    mace::vector<intptr_t, mace::SoftState> simulatorVector;
    mace::vector<intptr_t, mace::SoftState> subVector;

    const char* getType(EventType t) const {
      switch (t) {
        case APPLICATION:
          return "APPLICATION";
        case NETWORK:
          return "NETWORK";
        case SCHEDULER:
          return "SCHEDULER";
        default:
          ASSERT(0);
      }
      return NULL;
    }
    void print(std::ostream& out) const {
      out << "Event(node="<<node<<",desc="<<desc<<",type="<<getType(type)<<",simulatorVector="<<simulatorVector<<",subVector="<<subVector<<")";
    }

    bool equals(const int other_node, const EventType other_type,
        const int simVector0, const int simVector1, const int simVector2) const {
      return (node == other_node && type == other_type
          && simulatorVector[0] == simVector0
          && simulatorVector[1] == simVector1
          && simulatorVector[2] == simVector2);
    }

};

//typedef mace::map<uint64_t, Event, mace::SoftState> EventList;

/// Common data structure to hold pending events. Multimap for the modelchecker
/** Modelchecker: weight->Event; Simulator: time->Event; */
typedef mace::multimap<uint64_t, Event, mace::SoftState> EventList;

/**
 * Class that holds the list of pending events
 */
class SimEvent : public mace::PrintPrintable {
  protected:
    static SimEvent* _inst;

    EventList pendingEvents;

  public:
    void print(std::ostream& out) const {
      out << "events(size=" << pendingEvents.size() << ", ";
      EventList::const_iterator e = pendingEvents.begin();
      for (int i = 0; i < 30 && e != pendingEvents.end(); ++i, ++e) {
        out << "( " << e->first << " -> " << e->second << " )";
      }
      out << ")";
    }

    static EventList& getEvents() {
      return _inst->pendingEvents;
    }
    static bool hasMoreEvents() {
      return _inst->pendingEvents.size()-_inst->pendingEvents.count(0) > 0;
      //       return !_inst->pendingEvents.empty();
    }

    static void SetInstance() {
      if (_inst != NULL) { delete _inst; }
      _inst = new SimEvent();
    }
};

#endif // SIM_EVENT_H
