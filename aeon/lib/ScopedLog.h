/* 
 * ScopedLog.h : part of the Mace toolkit for building distributed systems
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
#ifndef _SCOPED_LOG_H
#define _SCOPED_LOG_H

#include <string>
#include "params.h"
#include "Log.h"
#include "pip_includer.h"
#include "ScopedLogReader.h"

/**
 * \file ScopedLog.h
 * \brief declares the ScopedLog class.
 */

/**
 * \addtogroup Scoped
 * @{
 */

/**
 * \brief handles printing a log message in the constructor and destructor
 *
 * Contains options for prefix and postfix, log and annotate, to support when
 * and where to print.  It also supports a log level, note to print for Pip, and
 * selector id to print to.
 *
 * Common usage:
 * \code
 * void fn() {
 *   static const log_id_t lid = Log::getId("global::fn");
 *   ScopedLog sl("function fn", 0, lid, true, true, true, false);
 *   // do some work.
 * }
 * \endcode
 * Which causes "STARTING/ENDING "function fn" messages to be printed at the
 * beginning and ending of the fn function, to the log class using lid
 * "global::fn", and not using Pip annotations.
 *
 * \todo place in the mace namespace.
 */
class ScopedLog {
public:
  /**
   * \brief constructs a new scoped log obejct
   * @param _note A Pip-only description of what starting and endings are being logged
   * @param _level The log level to pass to the log class
   * @param _logId The log selector id to use for logging
   * @param _prefix Whether to log in the constructor
   * @param _suffix Whether to log in the destructor
   * @param _log Whether to log to the log class
   * @param _annotate Whether to use Pip annotations
   */
  ScopedLog(const std::string& _note, log_level_t _level, log_id_t _logId, bool _prefix, bool _suffix, bool _log, bool _annotate) : 
    note(_note), level(_level), logId(_logId), prefix(_prefix), suffix(_suffix), doLog(_log), annotate(_annotate), stl(0)
  {
    static const log_id_t lid = Log::getId("ScopedLogTimes");
    static const bool LOG_TIMES = params::get(params::MACE_LOG_SCOPED_TIMES,
					      false);
    static const bool PROFILE_TIMES = params::get(params::MACE_PROFILE_SCOPED_TIMES, false);
    if (PROFILE_TIMES) {
      stl = new Log::ScopedTimeLog(lid, _note);
      return;
    }
    
    if(prefix) {
      if(suffix) {
        if(doLog) {
	  Log::binaryLog(logId, ScopedLogReader_namespace::ScopedLogReader(true), level);
	  if (LOG_TIMES) {
	    Log::log(lid) << note << " STARTING" << Log::endl;
	  }
        }
        if(annotate) {
          ANNOTATE_START_TASK(NULL, 0, note.c_str());
          int __pip_path_len__;
          const void* tp = ANNOTATE_GET_PATH_ID(&__pip_path_len__);
          ANNOTATE_PUSH_PATH_ID(NULL, 0, tp, __pip_path_len__);
        }
      }
      else {
        if(doLog) {
	  Log::binaryLog(logId, ScopedLogReader_namespace::ScopedLogReader(true, false), level);
        }
        if(annotate) {
          ANNOTATE_NOTICE(NULL, 0, "%s", note.c_str());
        }
      }
    }
  }

  virtual ~ScopedLog() {
    static const log_id_t lid = Log::getId("ScopedLogTimes");
    static const bool LOG_TIMES = params::get(params::MACE_LOG_SCOPED_TIMES,
					      false);
    static const bool PROFILE_TIMES = params::get(params::MACE_PROFILE_SCOPED_TIMES, false);
    if (PROFILE_TIMES) {
      Log::ThreadSpecific::logScopedTime(stl);
      stl = 0;
      return;
    }
    if(suffix) {
      if(prefix) {
        if(doLog) {
	  Log::binaryLog(logId, ScopedLogReader_namespace::ScopedLogReader(false), level);
	  if (LOG_TIMES) {
	    Log::log(lid) << note << " ENDING" << Log::endl;
	  }
        }
        if(annotate) {
          ANNOTATE_POP_PATH_ID(NULL, 0);
          ANNOTATE_END_TASK(NULL, 0, note.c_str());
        }
      }
      else {
        if(doLog) {
	  Log::binaryLog(logId, ScopedLogReader_namespace::ScopedLogReader(false, false), level);
        }
        if(annotate) {
          ANNOTATE_NOTICE(NULL, 0, "%s", note.c_str());
        }
      }
    }
  }

protected:
  const std::string& note;
  const log_level_t level;
  const log_id_t logId;
  const bool prefix;
  const bool suffix;
  const bool doLog;
  const bool annotate;
  Log::ScopedTimeLog* stl;
}; // ScopedLog

/**
 * @}
 */

#endif // _SCOPED_LOG_H
