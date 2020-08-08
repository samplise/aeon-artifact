/* 
 * Log.h : part of the Mace toolkit for building distributed systems
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
#ifndef _LOG_H
#define _LOG_H

#include <stdio.h>
#include <list>
#include <vector>
#include <cstdarg>
#include <iostream>
#include <sstream>
#include <fstream>
#include <pthread.h>
#include <map>
#include <deque>
#include "mstring.h"
// #include "m_map.h"
//#include "mhash_map.h"
#include "CircularBuffer.h"
#include "LogSelector.h"
#include "Serializable.h"
#include "BinaryLogObject.h"

//namespace mace {
//class BinaryLogObject;
//}

/**
 *  \file Log.h
 *
 *  \brief Details about the logging infrastructure.
 *
    All log messages have selectors and log levels.  Logging can additionally
    be enabled or disabled at compile time or runtime.

    For a log to get printed, it's level must be below (or equal to) the log
    level of the system.  It's selector must be selected, and the logging must
    not be disabled.  Furthermore, there is a way to disable (at compile time)
    all normal logs by defining some macros and/or setting a compile-time max
    log level.

    For relevant code, look at lib/mace-macros.h and lib/LogIdSet.h.

    Selectors are added by the macro #ADD_SELECTORS(x);  If called by
    #ADD_FUNC_SELECTORS, it uses
    \c __PRETTY_FUNCTION__.  Mace services
    generally use \a service::function::state or
    \a service::function::message::state.
    \a message is a message name or
    timer name, \a state is the guard, or "(demux)" if it's the demux method,
    or some other strings on generated methods. However, this format can be
    overridden by the \c log_selectors{} block.  This can be done
    to shorten selectors for example.  A prefix is added sometimes
    based on the macro and LogIdSet.

    All logging macros use log level 0, except #macecompiler(x) and
    #macedbg(x) (or their \c printf variants).  These take
    a required log level, though the #macecompiler(x) versions should only be used
    by generated code.

    Finally, there is the configuration of logging output.  This handled by
    - Log::add()
    - Log::autoAdd()
    - Log::autoAddAll()
    - Log::configure()

    Log::configure() utilizes those other ones, and references a number of
    configuration parameters (from the Params singleton),
    including params::MACE_LOG_AUTO_SELECTORS,
    params::MACE_LOG_LEVEL, and more.
    The Log::add, Log::autoAdd, and Log::autoAddAll methods allow configuration of which selectors get printed to
    which \c FILE*(s), and using what formats (timestamp, selector, thread id,
    etc.).  By default, \c WARNING and
    \c ERROR are Log::autoAdd -ed to std::stderr.  This can be disabled.  Selectors can
    be printed to more than one file descriptor.  Different selectors can be
    printed using different formats to the same file descriptor, and the
    same selector can be printed using different formats to different file
    descriptors.

    A log selector string is mapped to a \c log_id at the beginning of the
    execution.  If the selector is not selected before the mapping occurs, \c
    Log::NULL_ID is returned.  \c Log::NULL_ID causes fast-pass log
    suppression.   Otherwise the log id is an index into a data structure
    telling how to do the logging.

    You can in theory call Log::logXXX("selector"...), which checks the
    selector mapping each time (though once a mapping is selected, I'm not
    sure it is re-evaluated).  But that is highly inefficient -- instead the
    \c log_id should be used whenever possible, which the mace logging macros
    handle for you.

    So that's just a brief introduction to Mace logging.  It is supremely
    flexible and highly optimized, albeit a bit complex to fully understand.

    That should help you get started.
 */

struct hash_string;

typedef uint32_t log_level_t;

/**
 * \brief The main Mace logging subsystem class.  The overview of the subsystem is defined in lib/Log.h.
 *
 * With the exception of the configure and add methods, we anticipate that
 * users generally need not call methods on the Log class.  Instead, we expect
 * users will use the mace macros in mace-macros.h.
 *
 * Generally, you'll want the following setup:
 *
 * \code
 * int main(int argc, char** argv) {
 *   params::loadparams(argc, argv);
 *   Log::configure();
 *   //optional programmed Log::add, autoAdd, or autoAddAll calls.
 *
 *   //do work.
 * }
 * \endcode
 *
 * In a Mace service you can use the macros directly.  In other functions, you
 * need to either call ADD_SELECTORS("string") or ADD_FUNC_SELECTORS to prepare
 * the macros.
 *
 * \code
 * int calcSum() {
 *   ADD_SELECTORS("calcSum");
 *
 *   maceout << "This is a stream-based log, like cout" << Log::end;
 *   maceLog("This is a printf style log %s", ", don't you see");
 *
 *   //other options include macewarn, macedbg, maceerr, maceWarn, maceDebug,
 *   //maceError
 * }
 * \endcode
 *
 */
class Log {
public:
  static class LogFlushManipulator {} end, endl; //!< Simple objects used to flush a mace stream log 

  /**
   * \brief This class is used for tracing output.  Currently non-working, this
   * was the beginning of a liblog style system for Mace.
   */
  class MaceTraceStream {
  private:
    friend class Log;
    std::ostringstream* stream;
    log_id_t id;

  public:
    MaceTraceStream& operator<<(const LogFlushManipulator& l) {
      if(stream != NULL) {
	Log::flush(*this);
      }
      return *this;
    }

    template<typename T>
    MaceTraceStream& operator<<(const T* r) { 
      if(stream != NULL) {
	std::string tmp;
	mace::serialize(tmp, r);
	(*stream) << tmp;
      }
      return *this;
    }

    template<typename T>
    MaceTraceStream& operator<<(const T& r) { 
      if(stream != NULL) {
	std::string tmp;
	mace::serialize(tmp, &r);
	(*stream) << tmp;
      }
      return *this;
    }

    MaceTraceStream(log_id_t lid = NULL_ID) :
      stream(lid==NULL_ID ? NULL : new std::ostringstream()), id(lid) {}
    bool isNoop() const { return stream == NULL; }
  }; // MaceTraceStream
  
  class MaceOutputStream {
  private:
    friend class Log;
    /** When null, implies the stream is not selected for logging.  Otherwise
     * it stores a memory-backed stream for buffering output.
     */
    std::ostringstream* stream;
    /** The id is used by the Log class on flush to determine which log
     * parameters to apply.
     */
    log_id_t id;
  public:
    /// Causes the output stream to be flushed and logged.
    MaceOutputStream& operator<<(const LogFlushManipulator& l) {
      if(stream != NULL) {
	Log::flush(*this);
      }
      return *this;
    }
    /// Matches certain function pointers, for manipulation, newline, etc.
    MaceOutputStream& operator<<(std::ostream::__ios_type& (*__pf)(std::ostream::__ios_type&)) {
      if(stream != NULL) {
	(*stream) << __pf;
      }
      return *this;
    }
    /// Matches certain function pointers, for manipulation, newline, etc.
    MaceOutputStream& operator<<(std::ostream::ios_base& (*__pf)(std::ostream::ios_base&)) {
      if(stream != NULL) {
	(*stream) << __pf;
      }
      return *this;
    }
    /// Matches certain function pointers, for manipulation, newline, etc.
    MaceOutputStream& operator<<(std::ostream::__ostream_type& (*__pf)(std::ostream::__ostream_type&)) {
      if(stream != NULL) {
	(*stream) << __pf;
      }
      return *this;
    }
    /// Appends a generic pointer type to the stream by passthrough.
    template<typename T>
    MaceOutputStream& operator<<(const T* r) { 
      if(stream != NULL) {
	(*stream) << r;
      }
      return *this;
    }
    /// Appends a generic reference type to the stream by passthrough.
    template<typename T>
    MaceOutputStream& operator<<(const T& r) { 
      if(stream != NULL) {
	(*stream) << r;
      }
      return *this;
    }
    /// Appends pitem to the stream using the mace::printItem method instead of operator<<.
    template<typename T>
    void printItem(const T* pitem) {
      if(stream != NULL) {
        mace::printItem(*stream, pitem);
      }
    }
    /// Appends pitem to the stream using the mace::printXml method instead of operator<<.
    template<typename T>
    void printXml(const T* pitem) {
      if(stream != NULL) {
        mace::printXml(*stream, pitem, *pitem);
      }
    }
    MaceOutputStream(log_id_t lid = NULL_ID) :
      stream(lid==NULL_ID ? NULL : new std::ostringstream()), id(lid) {}
    /** \brief Used to determine if the stream represents a selected log.  
     *
     * Primarily used to prevent executing expensive string operations in
     * generated code.
     *
     * @returns true if the log will be printed on flush.
     */
    bool isNoop() const { return stream == NULL; }
  }; // MaceOutputStream

  /// Used when compiling logging out.  Designed to do nothing.
  class NullOutputType {
    public:
      template<typename T>
      const NullOutputType& operator<<(const T* r) const { return *this; }
      template<typename T>
      const NullOutputType& operator<<(const T& r) const { return *this; }

    const NullOutputType& operator<<(std::ostream::__ios_type& (*__pf)(std::ostream::__ios_type&)) const {
      return *this;
    }
    const NullOutputType& operator<<(std::ostream::ios_base& (*__pf)(std::ostream::ios_base&)) const {
      return *this;
    }
    const NullOutputType& operator<<(std::ostream::__ostream_type& (*__pf)(std::ostream::__ostream_type&)) const {
      return *this;
    }
  };

  typedef __MACE_BASE_HMAP__<log_id_t, MaceOutputStream*> StreamMap; ///< Maps log ids to logging output streams
  typedef __MACE_BASE_HMAP__<log_id_t, MaceTraceStream*> TraceStreamMap;
  
  /// Used internally by logging to store streams and per-thread information.
  /**
   * Uses the pthread_specific functionality to efficiently implement per-thread storage and management.
   */

  class ScopedTimeLog {
  public:
    ScopedTimeLog(log_id_t sel, const std::string& msg);
    virtual ~ScopedTimeLog() { }
  public:
    inline void setEndCount() {
      counter++;
      endCount = counter;
    }
    static inline uint64_t getAdvanceCounter() {
      counter++;
      return counter;
    }

  public:
    log_id_t selector;
    std::string message;
    uint64_t start;
    uint64_t end;
    uint32_t tid;
    uint64_t startCount;
    uint64_t endCount;
  protected:
    static uint64_t counter;
  };

  class ThreadSpecific {
    friend class Log;

  public:
    typedef std::vector<Log::ScopedTimeLog*> TimeList;
    typedef std::vector<mace::BinaryLogObject*> BinaryLogList;
    ThreadSpecific();
    virtual ~ThreadSpecific();
    static ThreadSpecific* init();
    static uint32_t getVtid();
    static StreamMap& getStreams();
    static TraceStreamMap& getTraceStreams();
    static void logScopedTime(Log::ScopedTimeLog* t);
    static void binaryLog(log_id_t sel, mace::BinaryLogObject* o);

  public:
    static const uint32_t DEFAULT_TIME_LIST_SIZE;
    static const uint32_t DEFAULT_BIN_LIST_SIZE;
    
  private:
    static void initKey();

  protected:
    uint32_t vtid;
    StreamMap streams;
    TraceStreamMap traceStreams;
    TimeList timelogs;
    BinaryLogList binlogs;
    typedef std::vector<ThreadSpecific*> ThreadSpecificList;
    static ThreadSpecificList threads;
    
  private:
    static pthread_key_t pkey;
    static pthread_once_t keyOnce;
    static unsigned int count;
  }; // ThreadSpecific


public:
  /** 
   * \brief returns the log id for the selector
   *
   * If the selector is selected for output, this method returns an id which matches the selector string.
   * If the selector is not selected, Log::NULL_ID is returned instead.
   *
   * @param selector The string identifying the log class.  e.g. Class::function
   * @return the log id, or Log::NULL_ID
   */
  static log_id_t getId(const std::string& selector);
  static const std::string& getSelector(log_id_t id);
  /**
   * \brief adds a specific selector to be printed.
   *
   * All parameters have a default value except the selector itself.  By
   * default, it will execute a text-stdout add with no timestamp, log name,
   * threadid.
   *
   * @param selector The selector to be added.
   * @param fp The FILE* to print it to.
   * @param lt The format of the timestamp.
   * @param ln Whether or not to print the selector name.
   * @param ltid Whether or not (and how) to print the thread id.
   * @param lso How to print the log, text, binary, or pip annotation.
   */
  static void add(const std::string& selector,
		  FILE* fp = stdout,
		  LogSelectorTimestamp lt = LOG_TIMESTAMP_DISABLED,
		  LogSelectorName ln = LOG_NAME_DISABLED,
		  LogSelectorThreadId ltid = LOG_THREADID_DISABLED,
                  LogSelectorOutput lso = LOG_FPRINTF);
  /**
   * \brief adds selectors matching a regular expression to be printed.
   *
   * All parameters have a default value except the selector itself.  By
   * default, it will execute a text-stdout add with timestamp, log name, and
   * threadid.
   *
   * @param fp The FILE* to print it to.
   * @param lt The format of the timestamp.
   * @param ln Whether or not to print the selector name.
   * @param ltid Whether or not (and how) to print the thread id.
   * @param lso How to print the log, text, binary, or pip annotation.
   */
  static void autoAddAll(FILE* fp = stdout,
			 LogSelectorTimestamp lt = LOG_TIMESTAMP_EPOCH,
			 LogSelectorName ln = LOG_NAME_ENABLED,
			 LogSelectorThreadId ltid = LOG_THREADID_ENABLED,
                         LogSelectorOutput lso = LOG_FPRINTF);
  /**
   * \brief adds all selectors to be printed.
   *
   * All parameters have a default value except the selector itself.  By
   * default, it will execute a text-stdout add with timestamp, log name,
   * threadid.
   *
   * @param subselector The selector regex to be added.
   * @param fp The FILE* to print it to.
   * @param lt The format of the timestamp.
   * @param ln Whether or not to print the selector name.
   * @param ltid Whether or not (and how) to print the thread id.
   * @param lso How to print the log, text, binary, or pip annotation.
   */
  static void autoAdd(const std::string& subselector,
		      FILE* fp = stdout,
		      LogSelectorTimestamp lt = LOG_TIMESTAMP_EPOCH,
		      LogSelectorName ln = LOG_NAME_ENABLED,
		      LogSelectorThreadId ltid = LOG_THREADID_ENABLED,
                      LogSelectorOutput lso = LOG_FPRINTF);
  /// Unsafe.  Do not use. \deprecated unsafe
  static void remove(const std::string& selector, FILE* fp = stdout);
  /// Unsafe.  Do not use. \deprecated unsafe
  static void removeAll(const std::string& selector);
  /// convenience method to open a logfile
  static FILE* openLogFile(const std::string& path, const char* mode = "a");
  /// convenience method to add a selector to a newly opened log file
  static void logToFile(const std::string& path, const std::string& selector,
			const char* mode = "w",
			LogSelectorTimestamp sts = LOG_TIMESTAMP_EPOCH,
			LogSelectorName sn = LOG_NAME_DISABLED,
			LogSelectorThreadId stid = LOG_THREADID_DISABLED);
  static void logSqlCreate(const std::string& createString, 
			   const mace::LogNode* node);
			   
  static void closeLogFiles();
  static void enableLogging(); ///< enable logging subsystem during runtime (default)
  static bool disableLogging(); ///< temporarily disable logging subsystem (during runtime)
  static void disableDefaultError() { errorSelected = false; } ///< cancels default logging of errors
  static void disableDefaultWarning() { warningSelected = false; } ///< cancels default logging of warnings
  static void perror(const std::string& s); ///< like C's \c perror, but prints to Log::err()
  static void sslerror(const std::string& s);
  static MaceOutputStream& err() { return log("ERROR"); } ///< returns a log stream based on the selector "ERROR"
  static MaceOutputStream& warn() { return log("WARNING"); } ///< returns a log stream based on the selector "WARNING"
  static MaceOutputStream& info() { return log("INFO"); } ///< returns a log stream based on the selector "INFO"
  static MaceOutputStream& log(log_id_t id, log_level_t level = DEFAULT_LEVEL); ///< returns a log stream from a log id and level
  /// Logs \c serializedObj of log type \c log_type according to selector \c id at level \c level
  static void binaryLog(log_id_t id, const std::string& log_type, 
			const std::string& serializedObj,
			log_level_t level = DEFAULT_LEVEL);
  /// Serializes \c object and uses it for binary log, and/or prints it to a string log
  static void binaryLog(log_id_t id, const mace::BinaryLogObject& object,
			log_level_t level = DEFAULT_LEVEL,
			uint64_t timestamp = 0, uint32_t vtid = 0);
  static void binaryLog(log_id_t id, mace::BinaryLogObject* object,
			log_level_t level = DEFAULT_LEVEL);
  /**
   * \brief Returns a log stream from a selector string and level.
   * \warning Very inefficient!  Use Log::getId() to get a log_id_t, then call Log::log(log_id_t, log_level_t) instead.
   */
  static MaceOutputStream& log(const std::string& selector,
			       log_level_t level = DEFAULT_LEVEL);
  /// logs a message for id at Log::DEFAULT_LEVEL
  static void log(log_id_t id, const std::string& message) {
    log(id, DEFAULT_LEVEL, message);
  }
  /// logs a message for id and level
  static void log(log_id_t id, log_level_t level, const std::string& message);
  /** \warning inefficient.  Use the log_id_t version instead. */
  static void log(const std::string& selector, const std::string& message) {
    log(selector, DEFAULT_LEVEL, message);
  }
  /** \warning inefficient.  Use the log_id_t version instead. */
  static void log(const std::string& selector, log_level_t level,
		  const std::string& message);
  // log a string to given id at Log::DEFAULT_LEVEL
  static void log(log_id_t id, const char* message) {
    log(id, DEFAULT_LEVEL, message);
  }
  /// Lowest level log method.  
  /** Prints message according to the \c id and \c level.  All other log and logf methods filter to this one. */
  static void log(log_id_t id, log_level_t level, const char* message,
		  uint64_t timestamp = 0, uint32_t vtid = 0);
  /** \warning inefficient.  Use the log_id_t version instead. */
  static void log(const std::string& selector, const char* message) {
    log(selector, DEFAULT_LEVEL, message);
  }
  /** \warning inefficient.  Use the log_id_t version instead. */
  static void log(const std::string& selector, log_level_t level, const char* message);
  /// printf style log with id
  static void logf(log_id_t id, const char* format ...) __attribute__((format(printf,2,3)));
  /// printf style log with id and level
  static void logf(log_id_t id, log_level_t level, const char* format ...) __attribute__((format(printf,3,4)));
  /** \warning inefficient.  Use the log_id_t version instead. */
  static void logf(const std::string& selector, const char* format ...) __attribute__((format(printf,2,3)));
  /** \warning inefficient.  Use the log_id_t version instead. */
  static void logf(const std::string& selector, log_level_t level, const char* format ...) __attribute__((format(printf,3,4)));
  static std::string toHex(const std::string& s); ///< helper method to convert \c s to a hex string to return.
  static void flush(MaceOutputStream& s); ///< called when Log::end or Log::endl are output onto a log stream.
  static void flush();
  static log_level_t getLevel() { return logLevel; } ///< gets the current log level.
  static void setLevel(log_level_t l = DEFAULT_LEVEL) { logLevel = l; } ///< sets the log level.  Safe to change at runtime.

  static void configure(); ///< Causes the logging subsystem to configure itself according to the parameters.

  static void nologf() {} ///< Used when compiling out logs.
  static const NullOutputType& nolog() { return nullOutputVar; } ///< Used when compiling out logs.

  static log_id_t getTraceId(const std::string& selector);
  static MaceTraceStream& trace(log_id_t id);
  static void flush(MaceTraceStream& s);
  static void traceNewThread(const mace::string& fname);
  static std::istream& replay(log_id_t id);
  static int dbCreate(const std::string& table, const std::string& type);
  
public:
  static const log_level_t DEFAULT_LEVEL; ///< The default level used for log methods.
  static const log_level_t MAX_LEVEL; ///< The maximum log level to be printed.
  static const log_id_t NULL_ID; ///< The log id value which indicates the log is not selected.
  static FILE* sqlCreatesLog;
  static FILE* sqlEventsLog;
  
private:
  typedef std::list<LogSelector*> SelectorList;

protected:
  static bool isSelected(const std::string& selector);
  static void remove(SelectorList* l, FILE* f);
  static void addl(const std::string& selector,
		   FILE* fp,
		   LogSelectorTimestamp,
		   LogSelectorName,
		   LogSelectorThreadId,
                   LogSelectorOutput);
  static void scheduleThread(unsigned int id, const mace::string& description);

  /// helper method to place a binary log in the file
  static void writeBinaryLog(log_id_t id, const LogSelector* sel, const std::string& log_type, const std::string& serializedObj);
  /// helper method to place a text log in the file
  static void writeTextLog(const LogSelector* sel, const char* text,
			   uint64_t timestamp = 0, uint32_t vtid = 0);
  static void initDbLogs();

private:
  static void lock();
  static void unlock();
  Log() { }

protected:
  static bool enabled;
  static bool autoAll;
  
//   typedef __MACE_BASE_HMAP__<std::string, SelectorList*, hash_string> SelectorMap;
//   typedef __MACE_BASE_HMAP__<std::string, FILE*, hash_string> LogFileMap;
//   typedef __MACE_BASE_HMAP__<std::string, log_id_t, hash_string> SelectorIdMap;
  typedef std::map<std::string, SelectorList*> SelectorMap;
  typedef std::map<std::string, FILE*> LogFileMap;
  typedef std::map<std::string, log_id_t> SelectorIdMap;
  typedef __MACE_BASE_HMAP__<log_id_t, std::string> IdSelectorMap;
  typedef std::map<int, CircularBuffer*> LogBufferMap;
  typedef std::vector<SelectorList*> IdSelectorList; //log_id_t index
//   typedef mace::hash_map<pthread_t, MaceOutputStream*> ThreadStreamMap;
//   typedef mace::vector<ThreadStreamMap*> LogStreamList; //log_id_t index

  static SelectorMap selectors;
  static IdSelectorList selectorLists;
  static LogFileMap logFiles;
  static LogBufferMap logBuffers;
  static const uint64_t DEFAULT_LOG_BUFFER_SIZE;
//   static LogStreamList streams;
  static SelectorList autoSelectors;
  static LogSelector* autoAllDefault;
  static pthread_mutex_t llock; // lock call to fprintf
  static pthread_mutex_t bufllock;
  static SelectorIdMap ids;
  static IdSelectorMap reverseIds;
  static uint32_t idCount;
  static const size_t MAX_MESSAGE_SIZE;
  static NullOutputType nullOutputVar;
  static log_level_t logLevel;
  static bool errorSelected;
  static bool warningSelected;
  static MaceTraceStream traceStream;
  static std::ifstream replayStream;
  static bool flushing;
}; // Log

// Log::MaceOutputStream& operator<<(Log::MaceOutputStream& o, Log::LogFlushManipulator l);

namespace mace {
  template<typename S> 
    void printItem(Log::MaceOutputStream& out, const S* pitem) {
      out.printItem(pitem);
    }

  //template<typename T> T const & logVal(T const & a, Log::MaceOutputStream& out, const mace::string& prefix) {
  //  out << prefix;
  //  printItem(out, &a);
  //  out << Log::end;
  //  return a;
  //}

  //template<typename T> T& logVal(T& a, Log::MaceOutputStream& out, const mace::string& prefix) {
  //  out << prefix;
  //  printItem(out, &a);
  //  out << Log::end;
  //  return a;
  //}

}

#define __LOG_H_DONE
#include "massert.h"
#endif // _LOG_H
