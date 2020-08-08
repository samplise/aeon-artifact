/* 
 * ScopedLock.h : part of the Mace toolkit for building distributed systems
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
#ifndef _SCOPED_LOCK_H
#define _SCOPED_LOCK_H

#include <pthread.h>
#include "massert.h"

/**
 * \file ScopedLock.h
 * \brief declares the ScopedLock class.
 */


/**
 * \addtogroup Scoped
 * @{
 */

/**
 * \brief provides a scoped use of a lock
 *
 * Acquires the lock in the constructor, releases the lock in the destructor.
 * Thus properly unlocks (even for recursive locks) in the case of exceptions,
 * return statements, etc., without having to worry about releasing the lock.
 *
 * Also supports programmatic releasing and acquiring of the lock, and will
 * make sure the lock is unlocked with the object goes out of scope.
 *
 * Common usage:
 * \code
 * // for some pthread_mutex_t m
 * ScopedLock sl(m);
 * // some stuff
 * sl.unlock();
 * // stuff without the lock
 * if (!keepgoing) {
 *   return;
 * }
 * sl.lock();
 * // some more stuff
 * \endcode
 *
 * \todo consider placing ScopedLock in the mace namespace.
 */
class ScopedLock {
public:
  /// locks the mutex passed in the constructor, holds the lock until destruction
  ScopedLock(pthread_mutex_t &l) : locked(false), slock(&l) {
    lock();
  }

  ScopedLock(pthread_mutex_t* l) : locked(false), slock(l) {
    if (slock) {
      lock();
    }
  }
  
  ~ScopedLock() {
    if (locked) {
      unlock();
    }
  }

  /// acquire the lock, and ensure unlocking it when the object is destructed
  /**
   * \pre !locked
   * \post locked
   */
  void lock() {
    ASSERT(slock);
    ASSERT(!locked);
    ASSERT(pthread_mutex_lock(slock) == 0);
    locked = true;
  }

  /// release the lock, ensuring not re-unlocking it when the object is destructed
  /**
   * \pre locked
   * \post !locked
   */
  void unlock() {
    ASSERT(locked);
    locked = false;
    ASSERT(pthread_mutex_unlock(slock) == 0);
  }
  
protected:
  bool locked; ///< stores whether the lock is held by this object or not.
  pthread_mutex_t* slock; ///< reference to the lock of this object.
  
}; // ScopedLock

/**
 * @}
 */

#endif // _SCOPED_LOCK_H
