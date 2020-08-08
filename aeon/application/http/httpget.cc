/* 
 * httpget.cc : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, James W. Anderson, Ryan Braud
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

#include "HttpClient.h"

using namespace std;

string version = "1.1";
bool quiet = false;
bool profile = false;
// bool latency = false;
// bool throughput = false;
bool uniqueid = false;
string clientprefix;

int fetch(string host, uint16_t port, string path, int& count) {

  HttpClient c(host, port);

  long long totaltime = 0;
  int reqCount = count;
  size_t bytes = 0;

//   do {
//     timeval starttime;
//     gettimeofday(&starttime, 0);

//     HttpResponse r = c.getUrl(path, version, false);
//     if (!quiet) {
//       if (r.headers.containsKey("Content-Type")) {
// 	if (r.headers["Content-Type"].find("text/") == 0) {
// 	  cout << r.content << endl;
// 	}
// 	else {
// 	  cout << "unsupported content type: " << r.headers["Content-Type"] << endl;
// 	}
//       }
//     }

//     timeval end;
//     gettimeofday(&end, 0);
//     totaltime += Util::timediff(starttime, end);

//     count--;
//   } while (count);

  int ccount = count;
  int id = 0;
  do {
    id++;
    string p = path;
    if (uniqueid) {
      p += "/" + clientprefix + "/" + boost::lexical_cast<string>(id);
    }
    c.getUrlAsync(p, version, version == "1.1");

    count--;
  } while (count);

  count = ccount;
  do {
    HttpResponse r;
    if (c.hasAsyncResponse()) {
      r = c.getAsyncResponse();
      bytes += r.rawheaders.size() + r.content.size();
      count--;
    }
    else {
      struct timeval tv = { 0, 50 * 1000 };
      SysUtil::select(0, 0, 0, 0, &tv);
      continue;
    }

    if (!quiet) {
      if (r.headers.containsKey("Content-Type")) {
	if (r.headers["Content-Type"].find("text/") == 0) {
	  cout << r.content << endl;
	}
	else {
	  cout << "unsupported content type: " << r.headers["Content-Type"] << endl;
	}
      }
    }

    totaltime += r.totalTime();
  } while (count);

  long long avgTime = totaltime / reqCount;
  
  if (profile) {
    // this is in bytes/second
    cerr << setprecision(10) << bytes / ((double)totaltime / 1000000)
	 << " " << avgTime << endl;
  }
  return count;
} // fetch

int main(int argc, char* argv[]) {
  params::set("MACE_ADDRESS_ALLOW_LOOPBACK", "1");
  params::set("MACE_WARN_LOOPBACK", "0");

  uint16_t port = 80;
  int count = 1;

  string usage = "usage: ";
  usage += argv[0];
  usage += " url [--version 1.0|1.1] [--count count] [--quiet] [--unique] [--profile]\n";

  while (1) {
    int option_index = 0;
    static struct option long_options[] = {
      {"count", required_argument, 0, 'c'},
      {"version", required_argument, 0, 'v'},
      {"quiet", no_argument, 0, 'q'},
      {"profile", no_argument, 0, 'p'},
//       {"profile_lat", no_argument, 0, 'l'},
      {"unique", no_argument, 0, 'u'},
      {"clientprefix", required_argument, 0, 'C'},
      {"help", no_argument, 0, 'h'},
      {0, 0, 0, 0}
    };

    int c = getopt_long(argc, argv, "c:v:qhpu", long_options, &option_index);
    if (c == -1) {
      break;
    }

    string sarg;

    switch (c) {

    case 'c':
      sarg = optarg;
      count = boost::lexical_cast<int>(sarg);
      break;
    case 'v':
      version = optarg;
      ASSERT(version == "1.1" || version == "1.0");
      break;
    case 'q':
      quiet = !quiet;
      break;
    case 'p':
      profile = !profile;
      break;
    case 'u':
      uniqueid = !uniqueid;
      break;
    case 'C':
      clientprefix = optarg;
      break;
//     case 'l':
//       latency = !latency;
//       break;
    case 'h':
      cerr << usage;
      exit(-1);

    default:
      cerr << "unknown option\n";
      cerr << usage;
      exit(-1);
    }
  }

  if (argc <= optind) {
    cerr << usage;
    exit(-1);
  }
  
  string url = argv[optind];

  string http = "http://";
  if (url.find(http) == 0) {
    url = url.substr(http.size());
  }

  StringList m = StrUtil::match("([^/]+)/(.*)", url);
  if (m.empty()) {
    cerr << "bad url " << url << endl;
    exit(-1);
  }
  
//   if (throughput && latency) {
//     cout << "Error: Can't profile on throughput and latency at the same "
// 	 << "time" << endl;
//   }
  
  string host = m[0];
  string path = "/" + m[1];

  m = StrUtil::match("([^:]+):(\\d+)", host);
  if (!m.empty()) {
    host = m[0];
    port = boost::lexical_cast<uint16_t>(m[1]);
  }

  try {
//     cout << "requesting " << path << " from " << host << endl;
    fetch(host, port, path, count);
  } catch (const Exception& e) {
    cerr << e << endl;
  }
  if (count) {
    cerr << count << " requests remaining" << endl;
  }

  return 0;
} // main

