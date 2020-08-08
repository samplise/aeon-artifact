/* 
 * RandomVariable.cc : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, Charles Killian, Karthik Nagaraj
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
#include "RandomVariable.h"
#include "RandomUtil.h"
#include "mace-macros.h"
#include <math.h>
#include <limits.h>

namespace RandomVariable {

  double pareto(double xm, double k) {
    double u = (double)RandomUtil::randInt() / UINT_MAX;
    return (xm / ( pow(u, (1/k))));
  }

  /* Computes zipf using f = 1/k^s * 1/SummationN(1/n^s).
   * N is the total number of elements
   * s is the exponent
   * k is the index of the element
   */
  int zipf(int n, double s) {
    ASSERT (n > 0 && s > 0);
    double d = 0; // Summation term
    for (int i = 0; i < n; i++)
      d += 1.0 / pow(i + 1, s);
    d = 1.0 / d;

    // (Sum of all frequency probabilities = 1)
    // Generate a random value. Cumulate frequencies till probability
    double random_prob = (double)RandomUtil::randInt() / UINT_MAX;
    double cumulative_prob = 0.0;
    for (int i = 0; i < n; i++) {
      cumulative_prob += d * 1.0 / pow(i + 1, s);
      if (cumulative_prob > random_prob)
        return i;
    }
    return n - 1;
  }

}
