/* 
 * Accumulator.h : part of the Mace toolkit for building distributed systems
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
#ifndef ACCUMULATOR_H
#define ACCUMULATOR_H

#include <cassert>
#include <string>
#include "mhash_map.h"
#include "Log.h"
#include "TimeUtil.h"
#include "ScopedLock.h"
#include "BinaryLogObject.h"

/**
 * \file Accumulator.h
 * \brief declares the Accumulator class for simple accumulated statistics
 */

//NOTE: This could build off of the statistical filter, and compute a wide variety of statistics.

/**
 * \brief Simple class for accumulating basic statistics.
 *
 * The accumulator is a new system to replace filters.  Used for accumulating
 * network statistics, byte counts, etc.  It uses uint64_t to be useful in a
 * wider variety of situations (without wrapping).
 *
 * Common usage:
 * \code
 * // in main()
 * Log::add(Accumulator::NETWORK_READ_SELECTOR);
 * Accumulator::startLogging(); //causes periodic logs of the accumulator values
 *
 * // elsewhere
 * Accumulator* a = Accumulator::Instance(Accumulator::NETWORK_READ);
 *
 * while (true) {
 *   int r = ::read(fd, buf, size);
 *   if (r > 0) {
 *     a->accumulate(r);
 *   }
 * }
 * \endcode
 *
 * Each accumulator will log values with a selector of the format
 * "Accumulator::SELECTORNAME".  So there are two variables for the basic
 * accumulators, one with the accumulator name, one with the selector name.
 */
class Accumulator {
public:
  static const std::string NETWORK_READ; ///< accumulates bytes from all transports reading
  static const std::string NETWORK_READ_SELECTOR; ///< selector for Accumulator::NETWORK_READ
  static const std::string NETWORK_WRITE; ///< accumulates bytes from all transport writing
  static const std::string NETWORK_WRITE_SELECTOR; ///< selector for Accumulator::NETWORK_WRITE
  static const std::string TCP_READ; ///< accumulates bytes from all TCP transpot reads
  static const std::string TCP_READ_SELECTOR; ///< selector for Accumulator::TCP_READ
  static const std::string TCP_WRITE; ///< accumulates bytes from all TCP transport writes
  static const std::string TCP_WRITE_SELECTOR; ///< selector for Accumulator::TCP_WRITE
  static const std::string UDP_READ; ///< accumulates bytes from all UDP transpot reads
  static const std::string UDP_READ_SELECTOR; ///< selector for Accumulator::UDP_READ
  static const std::string UDP_WRITE; ///< accumulates bytes from all UDP transport writes
  static const std::string UDP_WRITE_SELECTOR; ///< selector for Accumulator::UDP_WRITE
  static const std::string HTTP_CLIENT_READ; ///< accumulates bytes from HTTP reads
  static const std::string HTTP_CLIENT_READ_SELECTOR; ///< selector for Accumulator::HTTP_CLIENT_READ
  static const std::string HTTP_CLIENT_WRITE; ///< accumulates bytes from HTTP writing
  static const std::string HTTP_CLIENT_WRITE_SELECTOR; ///< selector for Accumulator::HTTP_CLIENT_WRITE
  static const std::string HTTP_SERVER_READ; ///< accumulates bytes from HTTP server reading
  static const std::string HTTP_SERVER_READ_SELECTOR; ///< selector for Accumulator::HTTP_SERVER_READ
  static const std::string HTTP_SERVER_WRITE; ///< accumulates bytes from HTTP server writing
  static const std::string HTTP_SERVER_WRITE_SELECTOR; ///< selector for Accumulator::HTTP_SERVER_WRITE
  static const std::string TRANSPORT_SEND; ///< accumulates bytes enqueued in the transports
  static const std::string TRANSPORT_SEND_SELECTOR; ///< selector for Accumulator::TRANSPORT_SEND
  static const std::string TRANSPORT_RECV; ///< accumulates bytes delivered from the transports
  static const std::string TRANSPORT_RECV_SELECTOR; ///< selector for Accumulator::TRANSPORT_RECV
  static const std::string TRANSPORT_RECV_CANCELED; ///< accumulates bytes received for cancelled messages
  static const std::string TRANSPORT_RECV_CANCELED_SELECTOR; ///< selector for Accumulator::TRANSPORT_RECV_CANCELED
  static const std::string FILE_WRITE; ///< accumulates bytes written from the file util
  static const std::string FILE_WRITE_SELECTOR; ///< selector for Accumulator::FILE_WRITE
  static const std::string APPLICATION_RECV; ///< intended for use by applications to measure goodput received
  static const std::string APPLICATION_RECV_SELECTOR; ///< selector for Accumulator::APPLICATION_RECV
  static const std::string APPLICATION_SEND; ///< intended for use by applications to measure goodput sent
  static const std::string APPLICATION_SEND_SELECTOR; ///< selector for Accumulator::APPLICATION_SEND
  static const std::string DB_WRITE_BYTES; ///< accumulates bytes written from a db
  static const std::string DB_WRITE_BYTES_SELECTOR; ///< selector for Accumulator::DB_WRITE_BYTES
  static const std::string DB_WRITE_COUNT; ///< accumulates writes to a db
  static const std::string DB_WRITE_COUNT_SELECTOR; ///< selector for Accumulator::DB_WRITE_COUNT
  static const std::string DB_ERASE_COUNT; ///< accumulates erases to a db
  static const std::string DB_ERASE_COUNT_SELECTOR; ///< selector for Accumulator::DB_ERASE_COUNT
  static const std::string EVENT_COMMIT_COUNT; ///< accumulates erases to a globally committed event
  static const std::string EVENT_COMMIT_COUNT_SELECTOR; ///< selector for Accumulator::EVENT_COMMIT_COUNT
  static const std::string EVENT_CREATE_COUNT; ///< accumulates erases to an created event
  static const std::string EVENT_CREATE_COUNT_SELECTOR; ///< selector for Accumulator::EVENT_CREATE_COUNT
  static const std::string ASYNC_EVENT_LIFE_TIME;
  static const std::string ASYNC_EVENT_LIFE_TIME_SELECTOR;
  static const std::string MIGRATION_EVENT_LIFE_TIME;
  static const std::string MIGRATION_EVENT_LIFE_TIME_SELECTOR;

  static const std::string ASYNC_EVENT_REQCOMMIT_TIME;
  static const std::string ASYNC_EVENT_REQCOMMIT_TIME_SELECTOR;
  static const std::string MIGRATION_EVENT_REQCOMMIT_TIME;
  static const std::string MIGRATION_EVENT_REQCOMMIT_TIME_SELECTOR;

  static const std::string ASYNC_EVENT_COMMIT;
  static const std::string ASYNC_EVENT_COMMIT_SELECTOR;
  static const std::string MIGRATION_EVENT_COMMIT;
  static const std::string MIGRATION_EVENT_COMMIT_SELECTOR;

  static const std::string EVENT_READY_COMMIT;
  static const std::string EVENT_READY_COMMIT_SELECTOR;

  static const std::string EVENT_REQUEST_COUNT;
  static const std::string EVENT_REQUEST_COUNT_SELECTOR; 

  static const std::string AGENTLOCK_COMMIT_COUNT; ///< accumulates erases to a globally committed event
  static const std::string AGENTLOCK_COMMIT_COUNT_SELECTOR; ///< selector for Accumulator::EVENT_COMMIT_COUNT

public:
  /// returns an accumulator for the given counter string
  static Accumulator* Instance(const std::string& counter) {
    ScopedLock sl(alock);
    Accumulator* r = 0;
    AccumulatorMap::iterator i = instances.find(counter);
    if (i == instances.end()) {
      r = new Accumulator();
      r->logId = Log::getId("Accumulator::" + counter);
      instances[counter] = r;
    }
    else {
      r = i->second;
    }
    return r;
  }

  /**
   * \brief call to begin a timer for accumulators
   *
   * the first call to accumulateUnlocked() after startTimer() sets startTime
   * (and each call sets lastTime).  This is so you can see the amount
   * accumulated between startTime and endtime, without bias for time passing
   * without accumulation.
   *
   * Clear the timer using stopTimer();
   */
  void startTimer() {
    startTime = 0;
    useTimer = true;
  }

  /// disables the timer started with startTimer()
  void stopTimer() { useTimer = false; }

  /**
   * \brief adds amount to the accumulator
   *
   * Calls accumulateUnlocked() after acquring a lock.  This is because it is
   * unsafe to call accumulate multithreaded without synchronization.
   *
   * @param amount amount to add to the accumulator
   */
  void accumulate(unsigned int amount) {
    ScopedLock sl(alock);
    accumulateUnlocked(amount);
  }

  /**
   * \brief adds amount to the accumulator
   *
   * Caller promises to not call accumulateUnlocked from two threads at the
   * same time.  If that may occur, caller should use accumulate() instead.
   * 
   * @param amount amount to add to the accumulator
   */
  void accumulateUnlocked(unsigned int amount) {
    totalBytes += amount;
    if (useTimer) {
      lastTime = TimeUtil::timeu();
      if (startTime == 0) {
	startTime = lastTime;
      }
    }
  }

  /// returns the current accumulator amount
  uint64_t getAmount() const { return totalBytes; }
  /// returns the amount since diff was last reset
  uint64_t getDiff() const { return totalBytes - diff; }
  /// returns the amount since diff was last reset and also resets diff.
  uint64_t resetDiff() {
    // leave this unlocked so that we do not need a recursive lock
    uint64_t r = totalBytes - diff;
    diff = totalBytes;
    return r;
  }

  uint64_t getStartTime() { return startTime; } ///< returns the startTime for the timer
  uint64_t getLastTime() { return lastTime; } ///< returns the lastTime for the timer

  static void dumpAll(); ///< prints all values in text to selector "Accumulator::dumpAll"
  static void logAll();  ///< prints all values to their specific accumulator
  static void startLogging(uint64_t interval = 1*1000*1000); ///< begins the logging thread
  static void stopLogging(); ///< ends the logging thread.  Must br called after start for clean shutdown.

protected:
  static void* startLoggingThread(void* arg);

private:
  static void lock() {
    assert(pthread_mutex_lock(&alock) == 0);
  }
  static void unlock() {
    assert(pthread_mutex_unlock(&alock) == 0);
  }


private:
  typedef mace::hash_map<std::string, Accumulator*, mace::SoftState, hash_string> AccumulatorMap;
  static AccumulatorMap instances;
  static bool isLogging;
  static pthread_t athread;
  static pthread_mutex_t alock;

  uint64_t totalBytes;
  uint64_t diff;
  bool useTimer;
  uint64_t startTime;
  uint64_t lastTime;
  log_id_t logId;
  
  Accumulator() : totalBytes(0), diff(0), useTimer(false), startTime(0), lastTime(0) {}
}; // class Accumulator

class AccumulatorLogObject : public mace::BinaryLogObject, public mace::PrintPrintable {
public:
  static const std::string type;
  std::string name;
  uint64_t total;
  uint64_t diff;
  
  AccumulatorLogObject() : total(0), diff(0) {}
  AccumulatorLogObject(const std::string& n, uint64_t t, uint64_t d) :
    name(n), total(t), diff(d) {}

protected:
  static mace::LogNode* rootLogNode;

public:
  // XXX consider using a variable sized integer serialization for this type
  void serialize(std::string& __str) const {
    mace::serialize(__str, &total);
    mace::serialize(__str, &diff);
  }
    
  int deserialize(std::istream& in) throw(mace::SerializationException) {
    int sz = 0;
    sz += mace::deserialize(in, &total);
    sz += mace::deserialize(in, &diff);
    return sz;
  }

  void sqlize(mace::LogNode* node) const {
    int index = node->nextIndex();
      
    if (index == 0) {
      std::ostringstream out;
      out << "CREATE TABLE " << node->logName
	  << " (_id INT PRIMARY KEY, name TEXT, total NUMERIC(20, 0), diff NUMERIC(20, 0));" 
	  << std::endl;
      Log::logSqlCreate(out.str(), node);
    }
    std::ostringstream out;
    out << node->logId << "\t" << index << "\t" << name << "\t" 
	<< total << "\t" << diff << std::endl;
    ASSERT(fwrite(out.str().c_str(), out.str().size(), 1, node->file) > 0);
  }

  void print(std::ostream& __out) const {
    __out << total << " " << diff;
  }

  const std::string& getLogType() const {
    return type;
  }

  const std::string& getLogDescription() const {
    static const std::string desc = "Accumulator";
    return desc;
  }

  LogClass getLogClass() const {
    return OTHER;
  }
  
  mace::LogNode*& getChildRoot() const {
    return rootLogNode;
  }
  
  static void init();
};

#endif // ACCUMULATOR_H
