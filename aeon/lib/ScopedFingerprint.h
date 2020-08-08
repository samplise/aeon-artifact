/* 
 * ScopedFingerprint.h : part of the Mace toolkit for building distributed systems
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
#ifndef _SCOPED_FINGERPRINT_H
#define _SCOPED_FINGERPRINT_H

#include "mstring.h"
#include "ScopedStackExecution.h"

/**
 * \file ScopedFingerprint.h
 * \brief declares mace::ScopedFingerprint
 */

namespace mace {

/**
 * \addtogroup Scoped
 * \brief The scoped tools all take advantage of begin and end behaviors in the constructor and destructor of objects.
 * @{
 */

/**
 * \brief Used to compute a fingerprint of the call path of an event.
 *
 * Used in the simulator to tell what event fired for time-based estimation.
 * May have other uses, such as getting a stack trace.  
 *
 * \todo Consider disabling ScopedFingerprint for deployment executions.
 */
class ScopedFingerprint {
  private:
  
  public:
    /// Place a scoped fingerprint object after the lock or ScopedLock object, and before each ScopedStackExecution object.
    /**
     * Must be placed before \e every ScopedStackExecution object or it might miss events or being cleared.
     */
    ScopedFingerprint(const std::string& selectorId) {
      ThreadSpecific *t = ThreadSpecific::init();

      if (ScopedStackExecution::getStackDepth() == 0) {
        t->clearFingerprint();
      }
      t->addFingerprint(selectorId + "(");
    }
    ~ScopedFingerprint() {
      ThreadSpecific *t = ThreadSpecific::init();
      t->addFingerprint(")");
    }

    /// returns the current fingerprint 
    /**
     * It is thread-safe function
     */
    static string& getFingerprint() {
      ThreadSpecific *t = ThreadSpecific::init();
      return t->getFingerprint();
    }


  private:  
    class ThreadSpecific {

    public:
      ThreadSpecific();
      ~ThreadSpecific();
      static ThreadSpecific* init();
      static uint32_t getVtid();
      void clearFingerprint();
      void addFingerprint(const string& s);
      string& getFingerprint();

    private:
      static void initKey();

    private:
      static pthread_key_t pkey;
      static pthread_once_t keyOnce;
      static unsigned int count;
      uint32_t vtid;
      string fingerprint;
  };
};



/**
 * @}
 */

}

#endif //_SCOPED_FINGERPRINT_H
