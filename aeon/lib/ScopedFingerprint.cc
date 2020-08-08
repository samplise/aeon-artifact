/* 
 * ScopedFingerprint.cc : part of the Mace toolkit for building distributed systems
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
#include "ScopedFingerprint.h"

//namespace mace {
//string ScopedFingerprint::fingerprint;
//}

pthread_key_t mace::ScopedFingerprint::ThreadSpecific::pkey;
pthread_once_t mace::ScopedFingerprint::ThreadSpecific::keyOnce = PTHREAD_ONCE_INIT;
unsigned int mace::ScopedFingerprint::ThreadSpecific::count = 0;

mace::ScopedFingerprint::ThreadSpecific::ThreadSpecific() {
  count++;
  vtid = count;
  fingerprint = "";
} // ThreadSpecific

mace::ScopedFingerprint::ThreadSpecific::~ThreadSpecific() {
} // ~ThreadSpecific

mace::ScopedFingerprint::ThreadSpecific* mace::ScopedFingerprint::ThreadSpecific::init() {
  pthread_once(&keyOnce, mace::ScopedFingerprint::ThreadSpecific::initKey);
  ThreadSpecific* t = (ThreadSpecific*)pthread_getspecific(pkey);
  if (t == 0) {
    t = new ThreadSpecific();
    assert(pthread_setspecific(pkey, t) == 0);
  }

  return t;
} // init

void mace::ScopedFingerprint::ThreadSpecific::initKey() {
  assert(pthread_key_create(&pkey, NULL) == 0);
} // initKey

unsigned int mace::ScopedFingerprint::ThreadSpecific::getVtid() {
  ThreadSpecific* t = (ThreadSpecific*)init();
  return t->vtid;
} // getVtid

void mace::ScopedFingerprint::ThreadSpecific::clearFingerprint() {
  this->fingerprint = "";
} // clearFingerprint

void mace::ScopedFingerprint::ThreadSpecific::addFingerprint(const string& s) {
  this->fingerprint += s;
} // addFingerprint()

mace::string & mace::ScopedFingerprint::ThreadSpecific::getFingerprint() {
  string &res = this->fingerprint;
  return res;
} // getFingerprint()

