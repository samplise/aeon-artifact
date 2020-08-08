/* 
 * RandomUtil.cc : part of the Mace toolkit for building distributed systems
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
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <limits.h>
#include "RandomUtil.h"
#include "TimeUtil.h"
#include "maceConfig.h"

void RandomUtil::reset() {
  if (_inst != NULL) {
    delete _inst;
  }

  _inst = NULL;
}

void RandomUtil::seedRandom(unsigned int s) {
  if (!srandInit) {
#ifdef HAVE_DRAND48
    srand48(s);
#else
#ifdef HAVE_RAND_S
#else
#ifdef HAVE_RAND
    srand(s);
#endif
#endif
#endif
    srandInit = true;
  }
}

RandomUtil& RandomUtil::Instance() { 
  if(_inst == NULL) { 
    seedRandom(TimeUtil::timeu());
    _inst = new RandomUtil(); 
  } 
  return *_inst; 
}

unsigned RandomUtil::randIntImpl() {
  return randIntImpl(UINT_MAX);
}

unsigned RandomUtil::randIntImpl(unsigned max) {
  // returns a random int in the range {0, max-1}
#ifdef HAVE_DRAND48
  double value = drand48();
  value *= max;
  unsigned r = (unsigned)floor(value);
  if (r == max) {
    r--;
  }
  return r;
#else
#ifdef HAVE_RAND_S
  unsigned int n;
  ASSERT(rand_s(&n) == 0);
  return (unsigned int)((double)n / (double)UINT_MAX * max);
#else
#ifdef HAVE_RAND
#if RAND_MAX != 32767
#error "unexpected randmax!"
#endif
  unsigned int n = (rand() << 17) | (rand() << 2) | (rand() & 0x3);
  return (unsigned int)((double)n / (double)UINT_MAX * max);
#endif
#endif
#endif
} // randInt

unsigned RandomUtil::randIntImpl(unsigned max, unsigned first, va_list ap) {
  va_list copy;
  va_copy(copy, ap);
  
  unsigned newMax = first;
  for (size_t i = 0; i < max - 1; i++) {
    unsigned t = va_arg(ap, unsigned);
    newMax += t;
  }

  unsigned rand = randIntImpl(newMax);
  unsigned r = 0;
  while (r < max - 1 && rand >= first) {
    r++;
    first += va_arg(copy, unsigned);
  }
  va_end(copy);

  return r;
}


RandomUtil* RandomUtil::_inst = NULL;
bool RandomUtil::srandInit = false;
