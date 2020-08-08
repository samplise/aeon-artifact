/* 
 * ThreadUtil.h : part of the Mace toolkit for building distributed systems
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
#ifndef __THREAD_UTIL_H
#define __THREAD_UTIL_H

#include <pthread.h>

/**
 * \file ThreadUtil.h
 * \brief defines the ThreadUtil utility, presently unused
 */

/**
 * \addtogroup Utils
 * @{
 */

/// Defines methods for thread utilities
/**
 * This utility is presently unused, and it's not clear what its future will be.
 */
class ThreadUtil {
public:
  /// creates a new thread
  static void create(pthread_t& t, const pthread_attr_t& attr,
		    void *(*start_routine)(void*), void* arg);
  static void lock(pthread_mutex_t& mutex); ///< acquire a lock
  static void unlock(pthread_mutex_t& mutex); ///< acquire an unlock
  static void wait(pthread_cond_t& signal, pthread_mutex_t& mutex); ///< wait on a signal and mutex
  static void signal(pthread_cond_t& sig); ///< signal a condition variable
  static void broadcast(pthread_cond_t& sig); ///< broadcast a signal to all waiters
  static void yield();
}; // ThreadUtil

/** @} */

#endif // __THREAD_UTIL_H
