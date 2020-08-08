/* 
 * mace-macros.h : part of the Mace toolkit for building distributed systems
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

/**
 * \file mace-macros.h
 *
 * \brief Defines standard macros for use throughout mace code.
 */

#include <limits.h>

#ifndef _MACEDON_MACROS_H
#define _MACEDON_MACROS_H

#ifndef MAX_LOG
#define MAX_LOG UINT_MAX ///< used to compile out logs -- disabling those at or above MAX_LOG
#endif

#ifndef SELECTOR_TAG
#define SELECTOR_TAG
#endif

#include "RandomUtil.h"
#include "LogIdSet.h"

#define MSEC_IN_MICROS *1000 ///< convenience macro for milliseconds (in microseconds)
#define SEC_IN_MICROS *1000 MSEC_IN_MICROS ///< convenience macro for seconds (in microseconds)

// #define logf(args...) nologf()
// #define log(args...) nolog()
// #define convertToString(args...) noConvertToString("")
// #define LOGLOGTYPE const Log::NullOutputType
#define LOGLOGTYPE Log::MaceOutputStream

#define DOWNGRADE(x) mace::AgentLock::downgrade(mace::AgentLock::x)

/// convenience method for getting a random integer instead of RandomUtil::randInt(uint32_t)
inline int randint(int x)		{ return RandomUtil::randInt(x); }
  
/**
 * \brief passes args to upcall \a func for each registered handler, used with
 * methods that return void.
 *
 * @param func the upcall to make
 * @param args the arguments to pass to func, less the registration_uid_t
 */
#define upcallAllVoid(func, args...) \
  for(std::map<int, typeof_upcall_##func*>::iterator _m_i = map_typeof_upcall_##func.begin(); \
      _m_i != map_typeof_upcall_##func.end(); _m_i++) { \
      upcall_##func(args, _m_i->first); \
  }

/**
 * \brief passes args to upcall \a func for each registered handler, combining
 * the results using \a ag
 *
 * @param var  the variable to store the result in
 * @param ag   the function to pairwise combine return values in \c var
 * @param func the upcall to make
 * @param args the arguments to pass to func, less the registration_uid_t
 */
#define upcallAll(var, ag, func, args...) \
  for(std::map<int, typeof_upcall_##func*>::iterator _m_i = map_typeof_upcall_##func.begin(); \
      _m_i != map_typeof_upcall_##func.end(); _m_i++) { \
      ag(var, upcall_##func(args, _m_i->first)); \
  }

#define andEq(x, y) x = x && y ///< convenience macro for an upcallAll which should and the results
    
#define orEq(x, y) x = x || y ///< convenience macro for an upcallAll which should or the results

/// Internal.  Used by generated code to prepare the backing for logs.
#define ADD_LOG_BACKING \
  LOGLOGTYPE* _maceout __attribute((unused)) = NULL;\
  LOGLOGTYPE* _macedbg[6] __attribute((unused)) = { NULL, NULL, NULL, NULL, NULL, NULL };\
  LOGLOGTYPE* _macewarn __attribute((unused)) = NULL;\
  LOGLOGTYPE* _maceerr __attribute((unused)) = NULL;\
  LOGLOGTYPE* _macecompiler[2] __attribute((unused)) = { NULL, NULL };\

/**
 * \brief Prepares a method for logging using standard macros
 *
 * Not needed within Mace services.
 *
 * @param x The string to use as the base string for the selectors
 */
#define ADD_SELECTORS(x) \
  static const std::string selector = x; \
  static const LogIdSet* const selectorId __attribute((unused)) = new LogIdSet(selector); \
  static const log_id_t __trace_selector __attribute((unused)) = Log::getTraceId(x); \
  ADD_LOG_BACKING
    
/// A version of #ADD_SELECTORS(x) which uses __PRETTY_FUNCTION__ as the selector base.
#define ADD_FUNC_SELECTORS ADD_SELECTORS(__PRETTY_FUNCTION__)

/// Internal.  Used by generated code to prepare functions for the #curtime macro.
#define PREPARE_FUNCTION \
  uint64_t _curtime __attribute__((unused)) = 0; \

//printf style
/**
 * \brief printf style debug message
 * @param pri log level
 * @param args arguments to printf
 */
#define maceDebug(pri, args...) do { if(pri < MAX_LOG) { Log::logf(selectorId->debug, pri, args); } } while(0)
/**
 * \def maceLog(args...) 
 * \brief printf style log message
 * @param args arguments to printf
 */
/**
 * \def maceWarn(args...)
 * \brief printf style warning message
 * @param args arguments to printf
 */
/**
 * \def maceError(args...)
 * \brief printf style error message
 * @param args arguments to printf
 */
#if MAX_LOG != 0
#define maceLog(args...) Log::logf(selectorId->log, args)
#define maceWarn(args...) Log::logf(selectorId->warn, args)
#define maceError(args...) Log::logf(selectorId->error, args)
#else 
#define maceLog(args...)
#define maceWarn(args...)
#define maceError(args...)
#endif
/**
 * \brief printf style compiler generated message
 * @param pri log level
 * @param args arguments to printf
 */
#define maceCompiler(pri,args...) do { if(pri < MAX_LOG) { Log::logf(selectorId->compiler, pri, args); } } while(0)

//ostream style
#define traceout Log::trace(__trace_selector)
#define tracein Log::replay(__trace_selector)

/// output stream based log.  Flush and output with << Log::endl
#define maceout (_maceout == NULL? *(_maceout = &Log::log((0 != MAX_LOG?selectorId->log:Log::NULL_ID))): *_maceout)
/// output stream based debug.  requires log level parameter. Flush and output with << Log::endl
#define macedbg(pri) (pri < 6? (_macedbg[pri] == NULL? *(_macedbg[pri] = &Log::log((pri < MAX_LOG?selectorId->debug:Log::NULL_ID), (log_level_t)pri)): *_macedbg[pri]): Log::log((pri < MAX_LOG?selectorId->debug:Log::NULL_ID), (log_level_t)pri))
/// output stream based warning.  Flush and output with << Log::endl
#define macewarn (_macewarn == NULL? *(_macewarn = &Log::log((0 != MAX_LOG?selectorId->warn:Log::NULL_ID))): *_macewarn)
/// output stream based error.  Flush and output with << Log::endl
#define maceerr (_maceerr == NULL? *(_maceerr = &Log::log((0 != MAX_LOG?selectorId->error:Log::NULL_ID))): *_maceerr)
/// output stream based compiler generated message.  requires log level parameter. Flush and output with << Log::endl
#define macecompiler(pri) (pri < 2? (_macecompiler[pri] == NULL? *(_macecompiler[pri] = &Log::log((pri < MAX_LOG?selectorId->compiler:Log::NULL_ID), (log_level_t)pri)): *_macecompiler[pri]): Log::log((pri < MAX_LOG?selectorId->compiler:Log::NULL_ID), (log_level_t)pri))

#define curtime (_curtime==0?(_curtime=TimeUtil::timeu()):_curtime) ///< Returns the current time of day, but the same value multiple times in a method

/// Prints warning if condition fails, allows optional success block
/**
 * example:
 * \code
 * EXPECT(x != 0) { 
 *   return y/x;
 * }
 * return 0;
 * \endcode
 *
 * or
 *
 * \code
 * EXPECT(x > 2);
 * return x-2;
 * \endcode
 */
#define EXPECT(x) if(!(x)) { maceerr << "EXPECT_FAILURE: Condition " << #x << " failed!" << Log::endl; } else 

#endif // _MACEDON_MACROS_H
