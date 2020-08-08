/* 
 * StringDHT.cc : part of the Mace toolkit for building distributed systems
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
#include "DHT-init.h"
// #include "Pastry-init.h"
// #include "TcpTransport-init.h"

#include "StringDHT.h"

using mace::string;
using mace::pair;
using std::cout;
using std::endl;

StringDHT::StringDHT() {
//   tcp = &TcpTransport_namespace::new_TcpTransport_Transport();
//   tcp->maceInit();
//   overlay = &Pastry_namespace::new_Pastry_OverlayRouter(*tcp, *tcp);
//   overlay->maceInit();
//   dht = &DHT_namespace::new_DHT_DHT(*overlay);
  dhtsv = &DHT_namespace::new_DHT_DHT();
  dhtsv->maceInit();
  DHTDataHandler* h = new DHTDataHandler();
  dht = new SynchronousDHT(*dhtsv, *h);
} // StringDHT

StringDHT::~StringDHT() {
  shutdown();
  delete dhtsv;
} // ~StringDHT

void StringDHT::shutdown() {
  dhtsv->maceExit();
}

bool StringDHT::containsKey(const string& k) {
//   cout << "calling containsKey" << endl;
  boost::shared_ptr<SynchronousDHT::SyncContainsKeyResult> p = dht->syncContainsKey(MaceKey(sha160, k));
  bool r = p->result;
//   cout << "containsKey returned " << r << endl;
  return r;
} // containsKey

pair<string, bool> StringDHT::get(const string& k) {
  boost::shared_ptr<SynchronousDHT::SyncGetResult> p = dht->syncGet(MaceKey(sha160, k));
  return pair<string, bool>(p->value, p->found);
} // get

int StringDHT::put(const string& k, const string& v) {
  dht->put(MaceKey(sha160, k), v);
  return 0;
} // put

int StringDHT::remove(const string& k) {
  dht->remove(MaceKey(sha160, k));
  return 0;
} // remove

