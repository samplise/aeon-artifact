/* 
 * LogSelector.h : part of the Mace toolkit for building distributed systems
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
// #include <stdio.h>
#include <string>
#include "Serializable.h"

#if __GNUC__ >= 3
#  if __GNUC__ >= 4
#    if __GNUC_MINOR__ >= 3
#      include <tr1/unordered_map>
#      define __MACE_BASE_HMAP__ std::tr1::unordered_map
#      define __MACE_BASE_HASH__ std::tr1::hash
#      define __MACE_USE_UNORDERED__
#    else 
#      include <ext/hash_map>
#      define __MACE_BASE_HMAP__ __gnu_cxx::hash_map
#      define __MACE_BASE_HASH__ __gnu_cxx::hash
#    endif
#  else
#    include <ext/hash_map>
#    define __MACE_BASE_HMAP__ __gnu_cxx::hash_map
#    define __MACE_BASE_HASH__ __gnu_cxx::hash
#  endif
#else
#  include <hash_map>
#  define __MACE_BASE_HMAP__ __gnu_cxx::hash_map
#  define __MACE_BASE_HASH__ __gnu_cxx::hash
#endif


/**
 * \file LogSelector.h
 * \brief declares LogSelector, associated with every log
 */

#ifndef LOG_SELECTOR_H
#define LOG_SELECTOR_H

/// How to print the timestamp
enum LogSelectorTimestamp { LOG_TIMESTAMP_DISABLED, ///< don't print the timestamp
			    LOG_TIMESTAMP_EPOCH, ///< print the timestamp as seconds and microseconds since the epoch
			    LOG_TIMESTAMP_HUMAN  ///< print the timestamp as a human-readable date and time
                            };
/// How to print the thread id
enum LogSelectorThreadId { LOG_THREADID_DISABLED, ///< don't print the thread id
                           LOG_THREADID_ENABLED   ///< print the virtual thread id
                         };
/// How to print the selector name
enum LogSelectorName { LOG_NAME_DISABLED, ///< don't print the log selector name
                       LOG_NAME_ENABLED   ///< print the log selector name
                       };
/// How to output the log 
enum LogSelectorOutput { LOG_FPRINTF,  ///< use fprintf for text logs
                         LOG_PIP,      ///< use Pip's ANNOTATE_NOTICE() functionality
                         LOG_BINARY,    ///< use the binary log utility to print this log
			 LOG_PGSQL     ///< log in a format suitable for loading into psql
                         };

class SimCommon;

/**
 * \brief An object describing a logging selection directive that indicates how
 * to handle matching logs.
 *
 * Each log is logged with a log_id_t, which maps directly to a list of
 * LogSelector objects.  Using Log::add(), Log::autoAdd(), or Log::autoAddAll()
 * causes new LogSelector objects to be created and added to the list.  Each
 * object describes how to handle matching logs, and includes a file output
 * stream and a set of directives on how to generate the logging header.
 *
 * The LogSelector's main functionality is to store these preferences and to
 * create the header for being logged.  LogSelectors should not be visible
 * outside the code in the Log class.  
 *
 * By using the Log*add methods more than once, multiple LogSelector objects
 * may be created for the same selector string, allowing the log to be printed
 * with different options to different FILE* destinations.  Each set of options
 * can only be associated once with a FILE*, multiple invocations with the same
 * FILE* will only override prior ones.
 *
 * Binary logging ignores the directives on how to print the header fields, as
 * well as what file to output to.  The system will only create one binary log.
 */
class LogSelector {
  friend class SimCommon;
  friend class Sim;

public:
  /// construct a new log selector, passing in options on how to print them
  LogSelector(const std::string& name,
	      FILE* fp, 
	      LogSelectorTimestamp logTimestamp,
	      LogSelectorName logSelectorName,
	      LogSelectorThreadId logSelectorThreadId,
              LogSelectorOutput logSelectorOutput);
  ~LogSelector();

  std::string getSelectorHeader(uint64_t ts = 0, uint32_t tid = 0) const; ///< returns a text-readable selector header obeying the user-directives for formatting
  std::string getSerializedSelectorHeader() const; ///< returns a binary selector header which always is in the same format

  ///< returns the selector name associated with this object
  std::string getName() const { return name; }
  ///< returns the FILE* to print matching logs to
  FILE* getFile() const { return fp; }
  int getFileNo() const { return fn; }
  ///< returns the timestamp option
  LogSelectorTimestamp getLogTimestamp() const { return logTimestamp; }
  ///< returns the log selector name option
  LogSelectorName getLogName() const { return logName; }
  ///< returns the log thread id option
  LogSelectorThreadId getLogThreadId() const { return logThreadId; }
  ///< returns the log selector output option
  LogSelectorOutput getLogSelectorOutput() const { return logOutput; }

private:
  const std::string name;
  FILE* fp;
  int fn;
  LogSelectorTimestamp logTimestamp;
  LogSelectorName logName;
  LogSelectorThreadId logThreadId;
  LogSelectorOutput logOutput;

  typedef __MACE_BASE_HMAP__<unsigned long, int, mace::SoftState> ThreadIdMap;
  static ThreadIdMap threadIds;
  static const std::string* prefix;
};

#endif // LOG_SELECTOR_H
