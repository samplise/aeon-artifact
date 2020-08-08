/* 
 * macemc-getmtime.h : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, Charles Killian, James W. Anderson
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
#include "MaceTime.h"
#include "Sim.h"

#ifndef macemc_getmtime_h
#define macemc_getmtime_h

//Include this file only in applications which are defining getmtime.
//Elsewhere this function is declared by lib/MaceTime.h, so it can be defined
//separately by modelchecker.cc

namespace macesim {
  class MonotoneTimeImpl : public mace::MonotoneTimeImpl {
    private:
      const int size;
      uint64_t* mtime;

    public:
      static MonotoneTimeImpl* mt;

      MonotoneTimeImpl(int size) : size(size) {
        mtime = new uint64_t[size];
        reset();
      }

      void reset() {
        memset(mtime, 0, sizeof(uint64_t)*size);
      }

      /** assume i < size and i > 0 */
      mace::MonotoneTime getTime() {
        return mtime[Sim::getCurrentNode()]++;
      }

      ~MonotoneTimeImpl() {
        delete[] mtime;
      }
  };
}

#endif //macemc_getmtime_h

