/* 
 * MaceTime.h : part of the Mace toolkit for building distributed systems
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
#ifndef _MACE_TIME_H
#define _MACE_TIME_H

#include <inttypes.h>
#include <pthread.h>
#include "RandomUtil.h"
#include "TimeUtil.h"
#include "Printable.h"
#include "Serializable.h"
#include "hash_string.h"
#include "Traits.h"
#include "Log.h"
#include "params.h"
#include "MaceKey.h"

/**
 * \file MaceTime.h
 * \brief defines mace::MaceTime and mace::MonotoneTime
 */

class Sim;

namespace mace {
  typedef uint64_t MonotoneTime; ///< monotone times are just uint64_t timestamps, and should be strictly monotone increasing.

  class MonotoneTimeImpl {
    public:
      static MonotoneTimeImpl* mt;

      virtual MonotoneTime getTime() {
        return TimeUtil::timeu();
      }

      virtual ~MonotoneTimeImpl() {
      }
  };

  class MonotoneTimeInit {
    public:
      static MonotoneTimeInit init;
      MonotoneTimeInit() {
        if (MonotoneTimeImpl::mt == NULL) {
          MonotoneTimeImpl::mt = new MonotoneTimeImpl();
        }
      }
  };

  /// Use this method to get a MonotoneTime.
  inline MonotoneTime getmtime() {
    return MonotoneTimeImpl::mt->getTime();
  }

  /// Simulator and Modelchecker friendly versions of time-of-day objects.  Can be used for real execution too.
  /**
   * In the model-checker, comparing two times is generally non-deterministic,
   * to support interleaving differently timed events.  Special care is taken
   * for maps, which would behave awkwardly if they were sorted randomly and
   * compared.  The comparison functions call random if we are being simulated,
   * and some versions support weights for biasing the results. Also, instead
   * of multiplication, you should scale your time, which would properly
   * maintain the taintedness of a time object.
   *
   * \todo all the times seem to be tainted.  is this broken?
   */
  class MaceTime : public PrintPrintable, public Serializable {
    friend class ::Sim;
    friend class mace::KeyTraits<MaceTime>;

  public:
    /// default constructor (time 0)
    MaceTime() : realtime(0), tainted(true) {
    }

    /// construct time at a specific offset
    MaceTime(uint64_t offset) : realtime(offset), tainted(true) {
    }

    /// copy constructor
    MaceTime(const MaceTime& other) : realtime(other.realtime), tainted(other.tainted) {
    }

    /// assigment operator
    MaceTime& operator=(const MaceTime& other) {
      realtime = other.realtime;
      tainted = other.tainted;
      return *this;
    }

    /// assign MaceTime from a fixed offset
    MaceTime& operator=(uint64_t offset) {
      realtime = offset;
      tainted = true;
      return *this;
    }

    /// add two MaceTime objects
    MaceTime operator+(const MaceTime& other) const {
      return plus(other);
    }

    /// subtract two MaceTime objects
    MaceTime operator-(const MaceTime& other) const {
      return minus(other);
    }

    /// equality operator
    bool operator==(const MaceTime& other) const {
      return realtime == other.realtime;
    }

    /// test equality against a fixed offset in microseconds.
    bool operator==(const uint64_t& other) const {
      return realtime == other;
    }

    /// inequality operator
    bool operator!=(const MaceTime& other) const {
      return realtime != other.realtime;
    }

    /// test inequality against a fixed offset in microseconds.
    bool operator!=(const uint64_t& other) const {
      return realtime != other;
    }

//     operator bool() const {
//       return realtime != 0;
//     }

    /// version of less than which supports passed in weights
    /**
     * The weights should represent effective real-world likelihood of the two
     * possible outcomes.  These weights are used in simulation with tainted times to bias the
     * execution to a realistic execution.
     */
    bool lessThan(const MaceTime& other, uint trueWeight = 1, uint falseWeight = 1) const {
      if (simulated && (tainted || other.tainted)) {
	return RandomUtil::randInt(2, falseWeight, trueWeight);
      }
      else {
	return realtime < other.realtime;
      }
    }

    /// version of greater than which supports passed in weights
    /**
     * The weights should represent effective real-world likelihood of the two
     * possible outcomes.  These weights are used in simulation to bias the
     * execution to a realistic execution.
     */
    bool greaterThan(const MaceTime& other,
		     uint trueWeight = 1, uint falseWeight = 1) const {
      return !lessThanOrEqual(other, falseWeight, trueWeight);
    }

    /// version of less than or equal which supports passed in weights
    /**
     * The weights should represent effective real-world likelihood of the two
     * possible outcomes.  These weights are used in simulation with tainted times to bias the
     * execution to a realistic execution.
     */
    bool lessThanOrEqual(const MaceTime& other,
			 uint trueWeight = 1,  uint falseWeight = 1) const {
      if (simulated && (tainted || other.tainted)) {
	return RandomUtil::randInt(2, falseWeight, trueWeight);
      }
      else {
	return realtime <= other.realtime;
      }
    }

    /// version of greater than or equal which supports passed in weights
    /**
     * The weights should represent effective real-world likelihood of the two
     * possible outcomes.  These weights are used in simulation with tainted times to bias the
     * execution to a realistic execution.
     */
    bool greaterThanOrEqual(const MaceTime& other,
			    uint trueWeight = 1,  uint falseWeight = 1) const {
      return !lessThan(other, falseWeight, trueWeight);
    }
  
    /// version of equals which supports passed in weights
    /**
     * The weights should represent effective real-world likelihood of the two
     * possible outcomes.  These weights are used in simulation with tainted times to bias the
     * execution to a realistic execution.
     */
    bool equals(const MaceTime& other, uint trueWeight = 1, uint falseWeight = 1) const {
      if (simulated && (tainted || other.tainted)) {
	return RandomUtil::randInt(2, falseWeight, trueWeight);
      }
      else {
	return realtime == other.realtime;
      }
    }

    /// version of inequality which supports passed in weights
    /**
     * The weights should represent effective real-world likelihood of the two
     * possible outcomes.  These weights are used in simulation with tainted times to bias the
     * execution to a realistic execution.
     */
    bool notEquals(const MaceTime& other,
		   uint trueWeight = 1, uint falseWeight = 1) const {
      return !equals(other, falseWeight, trueWeight);
    }

    /// add two MaceTimes
    MaceTime plus(const MaceTime& other) const {
      MaceTime t(*this);
      t.realtime += other.realtime;
      t.tainted |= other.tainted;
      return t;
    }

    /// subtract two MaceTimes
    MaceTime minus(const MaceTime& other) const {
      MaceTime t(*this);
      t.realtime -= other.realtime;
      t.tainted |= other.tainted;
      return t;
    }

    /// scale the time by a factor (multiplication or division)
    MaceTime scaleBy(uint amount) const {
      MaceTime t(*this);
      t.realtime *= amount;
      // clear tainted if amount == 0 ?
      return t;
    }

    /// scale the time by a factor (multiplication or division)
    MaceTime scaleBy(double amount) const {
      MaceTime t(*this);
      t.realtime = (uint64_t)((double)t.realtime * amount);
      // clear tainted if amount == 0 ?
      return t;
    }

    void serialize(std::string& s) const {
      mace::serialize(s, &realtime);
    }

    int deserialize(istream& in) throw(SerializationException) {
      int off = 0;
      off += mace::deserialize(in, &realtime);
      tainted = true;
      return off;
    }
    
    void sqlize(LogNode* node) const {
      int index = node->nextIndex();
      
      if (index == 0) {
	std::ostringstream out;
	out << "CREATE TABLE " << node->logName
	    << " (_id INT PRIMARY KEY, time INT8);" << std::endl;
	Log::logSqlCreate(out.str(), node);
      }
      std::ostringstream out;
      out << node->logId << "\t" << index << "\t" << realtime << std::endl;
      ASSERT(fwrite(out.str().c_str(), out.str().size(), 1, node->file) > 0);
    }
    
    void print(std::ostream& out) const {
      out << realtime;
    }
    
    void printState(std::ostream& out) const {
      if(simulated && tainted) {
        out << "NON-DET";
      } else {
        out << realtime;
      }
    }
    
    size_t hashOf() const {
      static const hash_bytes<uint64_t> h = hash_bytes<uint64_t>();
      return h(realtime);
    }
    
    uint64_t time() const {
      return realtime;
    }
    
    /// tests the time against 0
    bool isZero() const {
      return realtime == 0;
    }
    
    /// returns the "real" minimum of two times, even in simulation.  (Don't use unless you have good reason)
    static const MaceTime& min(const MaceTime& l, const MaceTime& r) {
      if (l.realtime < r.realtime) {
	return l;
      }
      return r;
    }
    
    /// return an object corresponding to the present time.  In most Mace code, this is just curtime
    static MaceTime currentTime() {
      MaceTime t(TimeUtil::timeu());
      t.tainted = true;
      return t;
    }
    
//     static uint64_t timeu() {
// //       if (simulated) {
// // 	timeofday += 1000;
// // 	return timeofday;
// //       }
//       return Util::timeu();
//     }

  private:
    uint64_t realtime;
    bool tainted;
    static bool simulated;
//     static uint64_t timeofday;
  }; // MaceTime

  /// functor for ordering MaceTime objects sanely in a map in the simulator
  class MaceTimeComparitor {
  public:
    bool operator()(const MaceTime& l, const MaceTime& r) const {
      return l.time() < r.time();
    }
  };

  /// functor for equality testing MaceTime objects sanely in a map in the simulator
  class MaceTimeEquality {
  public:
    bool operator()(const MaceTime& l, const MaceTime& r) const {
      return l.time() == r.time();
    }
  };

  /// functor for hashing a MaceTime object
  class MaceTimeHash {
  public:
    size_t operator()(const MaceTime& x) const {
      return x.hashOf();
    }
  };

  template<>
  class KeyTraits<MaceTime> : public KeyTraits<unsigned long long> {
    public:
    static void append(std::string& str, const MaceTime& key) {
      char buf[30];

      sprintf(buf, "%" PRIu64, key.time());
      str += buf;
    }
    static bool isDeterministic() {
      return !MaceTime::simulated;
    }
    static MaceTime extract(std::istream& in) 
    throw(SerializationException) {
      std::string token = SerializationUtil::get(in, '<');
      return strtoull(token.c_str(), NULL, 10);
    }
  };

  /// \todo CK - Document!
  class LogicalClock {
  public:
    typedef uint32_t lclock_t;
    typedef uint64_t lclockpath_t;

  private:
    lclock_t clock;
    lclockpath_t path;
    bool logPath;
    uint64_t mid;
    lclock_t rclock;
    uint32_t tid;
    static LogicalClock* inst;
    static double LOG_PATH_PCT;
    
    LogicalClock() : clock(0), path(0), logPath(true), mid(0), rclock(0), tid(0) {
      LOG_PATH_PCT = params::get<double>("MACE_LOG_PATH_PCT", 1);
    }

  public:
    lclock_t getClock() const { return clock; }
    lclockpath_t getPath() const { return path; }
    bool shouldLogPath() const { return logPath; }
    lclock_t getRemoteClock() const { return rclock; }
    uint64_t getMessageId() const { return mid; }
    void setMessageId(uint64_t m) { 
      ThreadSpecific* t = ThreadSpecific::init();
      t->setMessageId(m);
    }
    void getClock(lclock_t& cl, lclockpath_t& p, bool& lp) {
      lclock_t tcl;
      lclockpath_t tpath;
      bool tlp;
      if (ThreadSpecific::clockAdvanced(clock, tcl, tpath, tlp) &&
	  Log::ThreadSpecific::getVtid() == tid) {
	cl = clock;
	p = path;
	lp = logPath;
      }
      else {
	cl = tcl;
	p = tpath;
	lp = tlp;
      }
    }

//       void updatePending(lclock_t val) { 
//         ThreadSpecific::setPendingClock(val);
//       }
    void updatePending(lclock_t val, lclockpath_t pv, uint64_t id, bool lp) { 
      ThreadSpecific::setPendingClock(val, pv, id, clock, lp);
    }

    void tick() { 
      lclock_t tcl;
      ThreadSpecific::getPending(tcl, path, mid, rclock, logPath);
      clock = std::max(clock, tcl) + 1;
      tid = Log::ThreadSpecific::getVtid();
    }

    static LogicalClock& instance() {
      if (inst == NULL) {
	inst = new LogicalClock();
      }
      return *inst;
    }

  private:
    /// Used internally by the logical clock to store pending logical clock values
    /**
     * Uses the pthread_specific functionality to efficiently implement per-thread storage and management.
     */
    class ThreadSpecific {
      //friend class LogicalClock;

    public:
      ThreadSpecific();
      ~ThreadSpecific();
      static ThreadSpecific* init();
//           static lclock_t getPendingClock();
// 	  static lclockpath_t getPendingPath();
//   	  static lclock_t getRemoteClock();
      static bool clockAdvanced(lclock_t cur, lclock_t& cl, lclockpath_t& p,
				bool& lp);
      static void getPending(lclock_t& cl, lclockpath_t& p, uint64_t& id,
			     lclock_t& rcl, bool &logp);
//           static void setPendingClock(lclock_t val);
      static void setPendingClock(lclock_t val, lclockpath_t pv, uint64_t id,
				  lclock_t cur, bool lp);
// 	  static void setPendingPath(lclockpath_t val);
      void setMessageId(uint64_t m) {
	mid = m;
      }
    private:
      static void initKey();

    private:
      static pthread_key_t pkey;
      static pthread_once_t keyOnce;
      lclock_t pending;
      lclockpath_t path;
      bool logPath;
      uint64_t mid;
      lclock_t rclock;
      lclock_t curclock;
    }; // ThreadSpecific
  };

}; // mace

#ifdef __MACE_USE_UNORDERED__

namespace std {
namespace tr1 {
  /// functor which makes hashing MaceTime objects automatic.
  template<> struct hash<mace::MaceTime> {
    size_t operator()(const mace::MaceTime& x) const {
      return x.hashOf();
    }
  };
}
}

#else

namespace __gnu_cxx {
  /// functor which makes hashing MaceTime objects automatic.
  template<> struct hash<mace::MaceTime> {
    size_t operator()(const mace::MaceTime& x) const {
      return x.hashOf();
    }
  };
}

#endif

#endif // _MACE_TIME_H
