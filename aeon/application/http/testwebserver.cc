/* 
 * testwebserver.cc : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, James W. Anderson
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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <getopt.h>
#include <sstream>
#include <fstream>
#include <iostream>

#include "m_net.h"

#include <boost/lexical_cast.hpp>
#include <string>

#include "params.h"
#include "TimeUtil.h"
#include "StrUtil.h"
#include "SysUtil.h"
#include "HashUtil.h"

#include "HttpClient.h"

using namespace std;


string indexSha = "13a72e96fd675d4a750b5d3b93c6c77db82ddf71";
string jpegSha = "472b9b4c432ced51d5c2daf213be40555cdeeb95";
string fooSha = "f34e525036e4f710e8b4679fb37dbb9322ea1b4f";
string barSha = "c62425ee6a6d1c7ff5eaf03d25e6646fc3621306";
string quuxSha = "4de0136360cda3ffeb6fb356f8859387041bcf05";
string bigSha = "f1c84fa1d04b7e8b8396dedaffd611dc35075fd2";

void test(string test, bool comp) {
  if (comp) {
    cout << "PASSED";
  }
  else {
    cout << "FAILED";
  }
  cout << " - " << test << endl;
}

void testAll(string host, uint16_t port) {
  HttpClient cl(host, port);
  string hs;
  StringHMap headers;
  headers["Connection"] = "close";

  try {
    // basic HTTP/1.0
    HttpResponse r = cl.getUrl("/index.html", "1.0", false);
    ASSERT(!r.persistent);
    HashUtil::computeSha1(r.content, hs);
    test("HTTP/1.0 response code", r.responseCode == HttpResponse::OK);
    test("HTTP/1.0 html content", Log::toHex(hs) == indexSha);
//     cout << "HTTP/1.0 - read " << r.content.size() << " "
// 	 << Log::toHex(hs) << endl;
    test("HTTP/1.0 not connected", !cl.isConnected());
    test("HTTP/1.0 Content-Type text/html",
	 r.headers["Content-Type"].find("text/html") != string::npos);
    cl.closeConnection();
  }
  catch (const Exception& e) {
    cerr << "HTTP/1.0 html error: " << e << endl;
  }

  try {
    // HTTP/1.0 image
    HttpResponse r = cl.getUrl("/index_files/facad.jpg", "1.0", false);
    ASSERT(!r.persistent);
    HashUtil::computeSha1(r.content, hs);
    test("HTTP/1.0 response code", r.responseCode == HttpResponse::OK);
    test("HTTP/1.0 jpg content", Log::toHex(hs) == jpegSha);
    test("HTTP/1.0 Content-Type image/jpg",
	 (r.headers["Content-Type"].find("image/jpg") != string::npos) ||
	 (r.headers["Content-Type"].find("image/jpeg") != string::npos));
    cl.closeConnection();
  }
  catch (const Exception& e) {
    cerr << "HTTP/1.0 image error: " << e << endl;
  }

  try {
    // sanity check HTTP/1.1
    HttpResponse r = cl.getUrl("/index.html");
    test("HTTP/1.1 response code", r.responseCode == HttpResponse::OK);
    HashUtil::computeSha1(r.content, hs);
    test("HTTP/1.1 content", Log::toHex(hs) == indexSha);
    test("HTTP/1.1 Content-Type text/html",
	 r.headers["Content-Type"].find("text/html") != string::npos);
    test("HTTP/1.1 Content-Length",
	 r.headers["Content-Length"] == StrUtil::toString(r.content.size()));
    cl.closeConnection();
//     cout << "response code=" << r.responseCode << endl;
  }
  catch (const Exception& e) {
    cerr << "basic HTTP/1.1 error: " << e << endl;
  }
  
  try {
    // translate / 
    HttpResponse r = cl.getUrl("/");
    HashUtil::computeSha1(r.content, hs);
    test("HTTP/1.1 translate /", Log::toHex(hs) == indexSha);
    test("HTTP/1.1 translate / - response code",
	 r.responseCode == HttpResponse::OK);
    cl.closeConnection();

    r = cl.getUrl("/csepeople/");
    test("HTTP/1.1 translate / - not found",
	 r.responseCode == HttpResponse::NOT_FOUND);
    cl.closeConnection();
  }
  catch (const Exception& e) {
    cerr << "translate / error: " << e << endl;
  }

  try {
    // not found
    HttpResponse r = cl.getUrl("/notfound/facultyfoo.html");
    test("HTTP/1.1 not found", r.responseCode == HttpResponse::NOT_FOUND);
    cl.closeConnection();
  }
  catch (const Exception& e) {
    cerr << "not found error: " << e << endl;
  }

  try {
    // permission denied
    HttpResponse r = cl.getUrl("/permissions/index.html");
    test("HTTP/1.1 forbidden", r.responseCode == HttpResponse::FORBIDDEN);
    cl.closeConnection();
  }
  catch (const Exception& e) {
    cerr << "permission denied error: " << e << endl;
  }

  try {
    // malformed request
    HttpResponse r = cl.rawRequest("GET /foo HTTfoo\r\n\r\n");
    test("malformed request", r.responseCode == HttpResponse::BAD_REQUEST);
    cl.closeConnection();
  }
  catch (const Exception& e) {
    cerr << "malformed request error: " << e << endl;
  }

  try {
    // .htaccess
    HttpResponse r = cl.getUrl("/forbidden/index.html");
    test("HTTP/1.1 .htaccess - forbidden",
	 r.responseCode == HttpResponse::FORBIDDEN);
    cl.closeConnection();
  }
  catch (const Exception& e) {
    cerr << ".htaccess denied error: " << e << endl;
  }

  try {
    HttpResponse r = cl.getUrl("/allowed/index.html");
    test("HTTP/1.1 .htaccess - allowed",
	 r.responseCode == HttpResponse::OK);
    HashUtil::computeSha1(r.content, hs);
    test("HTTP/1.1 .htaccess - allowed content", Log::toHex(hs) == indexSha);
    cl.closeConnection();
  }
  catch (const Exception& e) {
    cerr << ".htaccess allowed error: " << e << endl;
  }

  try {
    // HTTP/1.1 - persistent connections
    HttpResponse r = cl.getUrl("/foo");
    HashUtil::computeSha1(r.content, hs);
    test("HTTP/1.1 foo", Log::toHex(hs) == fooSha);
    test("HTTP/1.1 connected", cl.isConnected());
    r = cl.getUrl("/bar");
    HashUtil::computeSha1(r.content, hs);
    test("HTTP/1.1 bar", Log::toHex(hs) == barSha);
    test("HTTP/1.1 persistent", r.persistent);
    test("HTTP/1.1 connected", cl.isConnected());
    r = cl.getUrl("/quux", "1.1", false, headers);
    HashUtil::computeSha1(r.content, hs);
    test("HTTP/1.1 quux", Log::toHex(hs) == quuxSha);
    test("HTTP/1.1 persistent", r.persistent);
//     test("HTTP/1.1 connected", cl.isConnected());
    cl.testServerClosed();
    test("HTTP/1.1 closed", !cl.isConnected());
  }
  catch (const Exception& e) {
    cerr << "persistent connections error: " << e << endl;
  }

  try {
    // HTTP/1.1 - pipelined requests
    cl.getUrlAsync("/foo");
    cl.getUrlAsync("/bar");
    cl.getUrlAsync("/quux", "1.1", false, headers);
    SysUtil::sleepm(100);
    bool pipelinePass = false;
    if (cl.hasAsyncResponse()) {
      HttpResponse r = cl.getAsyncResponse();
      HashUtil::computeSha1(r.content, hs);
      test("HTTP/1.1 pipelined foo", Log::toHex(hs) == fooSha);
      if (cl.hasAsyncResponse()) {
	r = cl.getAsyncResponse();
	HashUtil::computeSha1(r.content, hs);
	test("HTTP/1.1 pipelined bar", Log::toHex(hs) == barSha);
	test("HTTP/1.1 pipeline connected", r.persistent);
	if (cl.hasAsyncResponse()) {
	  r = cl.getAsyncResponse();
	  HashUtil::computeSha1(r.content, hs);
	  test("HTTP/1.1 pipelined quux", Log::toHex(hs) == quuxSha);
	  test("HTTP/1.1 pipeline connected", r.persistent);
	  pipelinePass = true;
	  cl.testServerClosed();
	  test("HTTP/1.1 pipeline closed", !cl.isConnected());
	}
      }
    }
    if (!pipelinePass) {
      cout << "FAILED - pipelining" << endl;
    }
  }
  catch (const Exception& e) {
    cerr << "pipelining error: " << e << endl;
  }

  try {
    // concurrent connections
    typedef std::deque<HttpClient*> ClientQueue;
    ClientQueue clients;
    size_t NCL = 10;
    for (size_t i = 0; i < NCL; i++) {
      clients.push_back(new HttpClient(host, port));
    }
    for (ClientQueue::iterator i = clients.begin(); i != clients.end(); i++) {
      HttpClient* c = *i;
      c->getUrlAsync("/big");
    }
    uint64_t start = TimeUtil::timeu();
    uint64_t timeout = 15*1000*1000;
    bool busy = true;
    while (busy && TimeUtil::timeu() - start < timeout) {
      busy = false;
      for (ClientQueue::iterator i = clients.begin(); i != clients.end(); i++) {
	HttpClient* c = *i;
	if (c->isBusy()) {
	  busy = true;
// 	  cout << "client busy, " << c->hasAsyncResponse() << endl;
	}
      }
      SysUtil::sleepm(1000);
    }
    test("concurrent timeout", !busy);
    size_t n = 0;
    ostringstream os;
//     cout << "expecting " << bigSha << endl;
    for (ClientQueue::iterator i = clients.begin(); i != clients.end(); i++) {
      HttpClient* c = *i;
      if (c->hasAsyncResponse()) {
	HttpResponse r = c->getAsyncResponse();
	HashUtil::computeSha1(r.content, hs);
	test("client " + StrUtil::toString(n) + " concurrent download",
	     Log::toHex(hs) == bigSha);
// 	cout << n << ": " << Log::toHex(hs) << endl;
	time_t st = r.requestBegin / (1000*1000);
	char stbuf[200];
	ctime_r(&st, stbuf);
	time_t et = r.responseComplete / (1000*1000);
	char etbuf[200];
	ctime_r(&et, etbuf);
	os << n << ": " << r.responseTime() / (1000 * 1000) << " "
	   << st << " " << et << " "
	   << StrUtil::trim(stbuf) << " - " << StrUtil::trim(etbuf)
	   << endl;
// 	     << r.requestComplete << " "
// 	     << r.responseBegin << " "
// 	     << r.responseComplete << endl;
      }
      else {
	test("client " + StrUtil::toString(n) + " concurrent download", false);
      }
      n++;
    }
    cout << os.str();
  }
  catch (const Exception& e) {
    cerr << "concurrent error: " << e << endl;
  }
}

int main(int argc, char* argv[]) {
  params::set("MACE_ADDRESS_ALLOW_LOOPBACK", "1");
  params::set("MACE_WARN_LOOPBACK", "0");

  string usage = "usage: ";
  usage += argv[0];
  usage += " server port\n";

  if (argc != 3) {
    cerr << usage;
    exit(-1);
  }

  uint16_t port = boost::lexical_cast<uint16_t>(argv[2]);
  testAll(argv[1], port);

  return 0;
} // main
