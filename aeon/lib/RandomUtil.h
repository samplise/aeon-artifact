/* 
 * RandomUtil.h : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, Charles Killian, Dejan Kostic, Ryan Braud, James W. Anderson, John Fisher-Ogden, Calvin Hubble, Duy Nguyen, Justin Burke, David Oppenheimer, Amin Vahdat, Adolfo Rodriguez, Sooraj Bhat, Karthik Nagaraj
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
#ifndef __RANDOM_UTIL_H
#define __RANDOM_UTIL_H

#include <stdarg.h>
#include <vector>
#include <limits.h>
/**
 * \file RandomUtil.h
 * \brief defines the RandomUtil random utility
 */

/**
 * \addtogroup Utils
 * @{
 */

/// Implements a standard utility for getting random integer values, which can be replaced at runtime with a simulator/modelchecker version.
class RandomUtil {
  protected:
    static RandomUtil* _inst;
    RandomUtil() { }
    static RandomUtil& Instance();

  public:
    static void reset();

    // randInt will call srand automatically if needed
    virtual unsigned randIntImpl();
    virtual unsigned randIntImpl(unsigned max);
    virtual unsigned randIntImpl(unsigned max, unsigned first, va_list ap);
    virtual unsigned randIntImpl(const std::vector<unsigned>& weights) {
      unsigned max = 0;
      for(std::vector<unsigned>::const_iterator i = weights.begin(); i != weights.end(); i++) {
        max += *i;
      }
      unsigned rand = randIntImpl(max);
      std::vector<unsigned>::const_iterator j;
      unsigned current;
      unsigned pos;
      for(j = weights.begin(), current = *j, pos = 0; j != weights.end() && rand >= current; ++j, pos++, current += *j);
      return pos;
    }
    
    static void seedRandom(unsigned int s); ///< seed the random number generator (a la srand48(int))
    static unsigned randInt() { return Instance().randIntImpl(); } ///< return a random integer between [0, UINT_MAX)
    /// return a random integer between 0 and \c max
    static unsigned randInt(unsigned max) {
      return Instance().randIntImpl(max);
    }
    /// return a random integer between 0 and \c max with weights as given, one for each outcome
    static unsigned randInt(unsigned max, unsigned first ...) {
      va_list ap;
      va_start(ap, first);
      unsigned r = Instance().randIntImpl(max, first, ap);
      va_end(ap);
      return r;
    }
    /// return a random integer between 0 and \c max using a va_list for calling from other variable arg methods
    static unsigned vrandInt(unsigned max, unsigned first, va_list ap) {
      return Instance().randIntImpl(max, first, ap);
    }
    /// return a random integer between 0 and \c weights.size(), each element being a weight
    static unsigned randInt(std::vector<unsigned> weights) {
      return Instance().randIntImpl(weights);
    }

    /// return a random element from a map, where the key is a random outcome, and the value is a weight
    template <typename T> static const typename T::key_type& random(const T& map) {
      ASSERT(!map.empty());
      std::vector<unsigned> weights;
      for(typename T::const_iterator i = map.begin(); i != map.end(); i++) {
        weights.push_back(i->second);
      }
      unsigned pos = Instance().randIntImpl(weights);
      typename T::const_iterator j = map.begin();
      for(unsigned i = 0; i < pos && j != map.end(); i++, j++);
      ASSERT(j != map.end());
      return j->first;
    }

    virtual ~RandomUtil() {}

  private:
    static bool srandInit;
};

/** @} */


#endif 
