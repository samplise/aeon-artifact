/* 
 * ScopedStackExecution.h : part of the Mace toolkit for building distributed systems
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
#ifndef _SCOPED_STACK_EXECUTION_H
#define _SCOPED_STACK_EXECUTION_H

#include "mset.h"
#include "mdeque.h"
#include "Log.h"
#include "LogIdSet.h"
#include "mace.h"
#include "ScopedLogReader.h"
#include "MaceTime.h"
#include "BinaryLogObject.h"

/**
 * \file ScopedStackExecution.h
 * \brief Defines mace::ScopedStackExecution
 */

namespace mace {

/// \todo CK - Document!
class SSEReader : public BinaryLogObject, public PrintPrintable {
public:
  bool begin;
  LogicalClock::lclock_t step;
  LogicalClock::lclockpath_t path;
  uint64_t messageId;
  LogicalClock::lclock_t rclock;
  uint64_t stcount;
  static const std::string type;
  
  SSEReader() {}
  SSEReader(bool b, LogicalClock::lclock_t val, LogicalClock::lclockpath_t p,
	    uint64_t id, LogicalClock::lclock_t rcl) :
    begin(b), step(val), path(p), messageId(id), rclock(rcl), stcount(0)
  {
    static const bool LOG_SCOPED_TIMES = params::get(params::MACE_LOG_SCOPED_TIMES, false);
    if (LOG_SCOPED_TIMES) {
      stcount = Log::ScopedTimeLog::getAdvanceCounter();
    }
  }

protected:
  static LogNode* rootLogNode;
  
public:
  void serialize(std::string& __str) const {
    __str += begin ? '1' : '0';
    mace::serialize(__str, &step);
  }
    
  int deserialize(std::istream& is) throw(mace::SerializationException) {
    char c;
      
    c = is.get();
    if (c == '1') {
      begin = true;
    }
    else if (c == '0') {
      begin = false;
    }
    else {
      std::cerr << "Invalid ScopedStackExecution token " << (int)c << " found" << std::endl;
      throw mace::SerializationException("Invalid ScopedStackExecution token found");
    }

    mace::deserialize(is, &step);

    return 1+sizeof(step);
  }
    
  void print(std::ostream& __out) const {
    static const bool LOG_SCOPED_TIMES = params::get(params::MACE_LOG_SCOPED_TIMES, false);
    if (begin) {
      __out << "STARTING (Step " << step << ") " << path
	    << " " << rclock << " " << messageId;
    }
    else {
      __out << "ENDING (Step " << step << ") " << path
	    << " " << rclock << " " << messageId;
    }
    if (LOG_SCOPED_TIMES) {
      __out << " " << stcount;
    }
  }
  
  void sqlize(mace::LogNode* node) const {
    int index = node->nextIndex();
    
    if (index == 0) {
      std::ostringstream out;
      out << "CREATE TABLE " << node->logName
	  << " (_id INT PRIMARY KEY, step INT, begin INT, path BIGINT, messageId NUMERIC(20, 0));" 
	  << std::endl;
      Log::logSqlCreate(out.str(), node);
    }
    std::ostringstream out;
    out << node->logId << "\t" << index << "\t" << step << "\t" 
	<< begin << "\t" << path << "\t" << messageId << std::endl;
    ASSERT(fwrite(out.str().c_str(), out.str().size(), 1, node->file) > 0);
  }
  
  const std::string& getLogType() const {
    return type;
  }

  const std::string& getLogDescription() const {
    static const std::string desc = "ScopedStackExecution";
    return desc;
  }
  
  LogClass getLogClass() const {
    return STRUCTURE;
  }
  
  LogNode*& getChildRoot() const { 
    return rootLogNode;
  }
  
  static void init();
};

/**
 * \addtogroup Scoped
 * \brief The scoped tools all take advantage of begin and end behaviors in the constructor and destructor of objects.
 * @{
 */

/**
 * \brief Supports deferring actions and identifying event boundaries.
 *
 * A ScopedStackExecution object is placed after the agent lock is acquired, in
 * each service transition and entry point.  A static variable is used to track
 * the call stack depth, incremented in each constructor, and decremented in
 * each destructor.
 *
 * In the constructor, a log message is printed if the stack level is 0,
 * because a new event is beginning.  In the destructor, a log message is
 * printed if the stack level is 0, because the event is ending.  Also, any
 * deferred actions are executed at this point.
 *
 * Finally, the scope level is exposed through a static method, so it may be
 * inspected elsewhere.
 */
class ScopedStackExecution {
  private:
    typedef set<BaseMaceService*, SoftState> ServiceList; ///< A type to store pointers to Mace generated services.
    
    static const bool logStart = true; ///< Whether to print a message at the beginning of a logical time step
    static const bool logEnd = true; ///< Whether to print a message at the end of a logical time step
    int here;                       // current number of stack.
    bool logThis;
  public:
    /**
     * \brief Place this object after lock acquiring to mark the beginning of a transition or service entry point.
     */
    ScopedStackExecution(bool _logThis = true) {
      static const bool LOG_SCOPED_STACK = params::get("LOG_SCOPED_STACK", true);
      static const log_id_t logid = Log::getId("ServiceStack");
      logThis = _logThis;

      ThreadSpecific *t = ThreadSpecific::init();
      here = t->getStackValue();
      t->increaseStackValue();

      //std::cerr << "[" << t->getVtid() << "] stack++ (here = " << here << ")\n";
      // here = stack++;
      if (here == 0) {
	LogicalClock& cl = LogicalClock::instance();
        cl.tick();
        if (LOG_SCOPED_STACK && logStart && logThis && cl.shouldLogPath()) {
          Log::binaryLog(logid, new SSEReader(true, cl.getClock(), cl.getPath(),
					      cl.getMessageId(),
					      cl.getRemoteClock()));
        }
      }
    }
    ~ScopedStackExecution() {
      static const bool LOG_SCOPED_STACK = params::get("LOG_SCOPED_STACK", true);
      static const log_id_t logid = Log::getId("ServiceStack");
      if (here == 0) {
        runDefer();
        if (LOG_SCOPED_STACK && logEnd && logThis) {
	  LogicalClock& cl = LogicalClock::instance();
	  if (cl.shouldLogPath()) {
	    Log::binaryLog(logid, new SSEReader(false, cl.getClock(), cl.getPath(),
						cl.getMessageId(),
						cl.getRemoteClock()));
	  }
        }
      }
      ThreadSpecific *t = ThreadSpecific::init();
      t->decreaseStackValue();
      //std::cerr << "** [" << t->getVtid() << "] stack-- (here = " << here << ")\n";
      //stack--;
    }

    /// A service calls addDefer to indicate that when the call stack returns to 0, the \c service should have processDefered() called on it.
    static void addDefer(BaseMaceService* service) { ThreadSpecific *t = ThreadSpecific::init(); t->insertService(service);}
    /// called when stack returns to 0 to execute deferred actions.  Recurses in case deferred actions defer more actions.
    static void runDefer() {
      ADD_SELECTORS("ScopedStackExecution::runDefer");
      ServiceList sl;
      ThreadSpecific *t = ThreadSpecific::init();
      t->getServiceList(sl);
      
      for (ServiceList::iterator i = sl.begin(); i != sl.end(); i++) {
        (*i)->processDeferred();
      }
      if (!t->needDeferEmpty()) {
	runDefer();
      }
    }
    
    /// returns the current stack depth.  When called from a service \e executing an event, will return "here + 1", so 1 means top-level.
    static int getStackDepth() {
      ThreadSpecific *t = ThreadSpecific::init();
      return t->getStackValue();
    }

  private:
    class ThreadSpecific {

    public:
      ThreadSpecific();
      ~ThreadSpecific();
      static ThreadSpecific* init();
      static uint32_t getVtid();
      int getStackValue();
      void increaseStackValue();
      void decreaseStackValue();
      bool needDeferEmpty();
      void getServiceList(ServiceList& sl);
      void insertService(BaseMaceService* service);

    private:
      static void initKey();

    private:
      static pthread_key_t pkey;
      static pthread_once_t keyOnce;
      static unsigned int count;
      uint32_t vtid;
      int stack;
      ServiceList needDefer;

    }; // ThreadSpecific
  };

/**
 * @}
 */

}

#endif //_SCOPED_STACK_EXECUTION_H
