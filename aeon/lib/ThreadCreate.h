/* 
 * ThreadCreate.h : part of the Mace toolkit for building distributed systems
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
#ifndef __THREAD_CREATE_H
#define __THREAD_CREATE_H

#include <pthread.h>
#include <string>
#include <stdint.h>

/**
 * \file ThreadCreate.h
 *
 * \brief provides macro declarations for creating new threads.
 *
 * All threads should be created Using these macros.  They have at least the following benefits:
 * - automatic logging at thread creation and destruction
 * - automatic call to Scheduler::shutdownJoinThread() to make sure it gets
 *   joined on exit
 * - automatic call to Scheduler::joinThread() on end to allow the thread to be
 *   joined quickly
 * - Ability to start threads in static methods or object methods
 */

/// Class marker for classes which are used for running new threads.
class RunThreadClass {};

typedef void* (*func)(void*); ///< non-class function for starting a thread
typedef void* (RunThreadClass::*classfunc)(void*); ///< class-member function for starting a thread

/// Start a new thread in a static method or global method. 
/**
 * @param a pthread to store the created thread
 * @param b the function to call on start
 * @param c the void* parameter to pass the function
 * @param f the pthread attributes
 */
#define runNewThread(a, b, c, f) _runNewThread(a, b, c, f, #b)
/// Start a new thread in a class method
/**
 * @param a pthread to store the created thread
 * @param cl the class object to call the function on 
 * @param b the function to call on start
 * @param c the void* parameter to pass the function
 * @param f the pthread attributes
 */
#define runNewThreadClass(a, cl, b, c, f) _runNewThreadClass(a, cl, b, c, f, #b)

/// Generally you should not use this directly.  Exceptions are if you don't want joinThread or if you want to specify a different string
void _runNewThread(pthread_t* t, func f, void* arg, pthread_attr_t* attr, 
		   const char* fname, bool joinThread = true);
/// Generally you should not use this directly.  Exceptions are if you don't want joinThread or if you want to specify a different string
void _runNewThreadClass(pthread_t* t, RunThreadClass* c, classfunc f, 
			void* arg, pthread_attr_t* attr, const char* fname,
			bool joinThread = true);
/// Method to do standard logging at beginning and end of thread.  Declared here to be able to call in main.
void logThread(uint64_t vtid, const std::string& fname, bool ending = false);

#endif
