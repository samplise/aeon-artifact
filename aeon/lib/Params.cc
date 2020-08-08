/* 
 * Params.cc : part of the Mace toolkit for building distributed systems
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
#include "params.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mace-macros.h"
#include "ParamReader.h"

using namespace std;

namespace params {

void Params::addRequired(const string& name, const string& desc) {
  required[name] = desc;
} // addRequired

void Params::verifyRequired() {
  // verify we have all the required args
  bool error = false;
  for (StringMap::iterator i = required.begin(); i != required.end(); i++) {
    if (!containsKey(i->first)) {
      fprintf(stderr, "required argument: %s - %s\n", i->first.c_str(), i->second.c_str());
      error = true;
    }
  }
  if (error) {
    exit(-1);
  }
} // verifyRequired

int Params::loadparams(int& argc, char**& argv, bool requireFile) {
//   printf("loading params, argc=%d\n", argc);
//   for (int i = 0; i < argc; i++) {
//     printf("%s ", argv[i]);
//   }
//   printf("\n");

  int startarg = 1;
  int found = 0;
  if (argc==1 || argv[1][0] == '-') {
//     printf("REPLAY PARAMETER FILE: params.default\n");
    loadfile("params.default");
  } else if (argc > 1) {
    startarg = 2;
//     printf("REPLAY PARAMETER FILE: %s\n",argv[1]);
    struct stat sbuf;
    if ((stat(argv[1], &sbuf) == 0) && S_ISREG(sbuf.st_mode)) {
      loadfile(argv[1]);
      argv[1] = 0;
      found = 1;
    }
    else if (requireFile) {
      std::cerr << "could not open params file " << argv[1] << std::endl;
      exit(-1);
    }
  }
  else {
    return 0;
  }

  for (int i = startarg; i<argc; i++) {
    if (!argv[i]) {
      continue;
    }
    char* arg = *(argv+i);
    if (arg[0] != '-' || strlen(arg) < 2) continue;
    arg++;
    if (arg[0] == '-') {
      arg++;
    }
//     printf("Node REPLAY_PARM: %s %s\n",argv[i],argv[i+1]);
    //NOTE: For now assuming no '='
    if (i < argc-1) {
      set(arg, argv[i+1]);
      argv[i] = 0;
      argv[i+1] = 0;
      found += 2;
    } else {
      argv[i] = 0;
      set(arg, string());
      found++;
    }
  }

  for (int i = 1; i < argc; i++) {
    if (!argv[i]) {
      for (int j = i + 1; j < argc; j++) {
	if (argv[j]) {
	  argv[i] = argv[j];
	  argv[j] = 0;
	  break;
	}
      }
    }
  }
  argc -= found;

//   printf("done loading params, argc=%d\n", argc);
//   for (int i = 0; i < argc; i++) {
//     printf("%s ", argv[i]);
//   }
//   printf("\n");

  verifyRequired();
  return 0;
}

string Params::trim(const string& s) {
  int start = 0;
  while (s[start] == ' ' || s[start] == '\t') {
    start++;
  }

  int end = s.size();
  while (s[end - 1] == ' ' || s[end - 1] == '\t') {
    end--;
  }
    
  if (end < start) {
    return "";
  }

  return s.substr(start, end - start);
} // trim

int Params::loadfile(const char* filename, bool allRequired) {
  ifstream f(filename);
  if (!f.is_open()) {
//     cout << "could not open file " << filename << endl;
    return 0;
  }

  uint count = 0;
  while (!f.eof()) {
    count++;
    const int BUF_SIZE = 1000000;
    char buf[BUF_SIZE];
    f.getline(buf, BUF_SIZE);

    string s(buf);
    if (s.find('#') != string::npos) {
      string::size_type openPos = s.find('"');
      string::size_type closePos = s.find_last_of('"');
      string::size_type commentPos = s.find('#');
      if (!(openPos != string::npos && closePos != string::npos &&
	    openPos < commentPos && commentPos < closePos)) {
	s = s.substr(0, commentPos);
      }
    }
    s = trim(s);
    if (s.empty()) {
      continue;
    }

    string::size_type i = s.find('=');
    if ((i == string::npos) || (i == 0) || (i == (s.size() - 1))) {
      cerr << "error in parameters file " << filename << endl
	   << "line " << count << ": " << s << endl
	   << "parameters format is key=\"value\"" << endl;
    }
    string key = s.substr(0, i);
    key = trim(key);
    string value = s.substr(i + 1);
    value = trim(value);
    if (value[0] == '"') {
      value = value.substr(1);
    }
    if (value[value.size() - 1] == '"') {
      value = value.substr(0, value.size() - 1);
    }

    setAppendNewline(key, value);
  }
  f.close();

  if (allRequired) {
    verifyRequired();
  }

  return 0;
} // loadfile

int Params::print(FILE* ostream) {
  int total=fprintf(ostream,"REPLAY PARAMETERS:\n");
  for (StringMap::iterator i = data.begin(); i != data.end(); i++) {
    total += fprintf(ostream, "REPLAY PARAMETER: %s = \"%s\"\n",
		     i->first.c_str(), i->second.c_str());
  }
  return total;
}

int Params::print(std::ostream& os) {
  for (StringMap::iterator i = data.begin(); i != data.end(); i++) {
    os << i->first << " = " << i->second << std::endl;
  }
  return data.size();
}

void Params::log() {
  static log_id_t lid = Log::getId("Params");
  
  Log::binaryLog(lid, ParamReader_namespace::ParamReader(data), 0);
}

void Params::printUnusedParams(FILE* ostream) {
  fprintf(ostream, "UNUSED PARAMETERS:\n");
  for (StringMap::iterator i = data.begin(); i != data.end(); i++) {
    if (!touched.contains(i->first)) {
      fprintf(ostream, "UNUSED PARAMETER: %s = \"%s\"\n", i->first.c_str(), i->second.c_str());
    }
  }
}

  Params::Params() : watch(false) {
}

void Params::getBootstrapPeers(const MaceKey& laddr, NodeSet& peers, bool& autoBootstrap) {
  ADD_SELECTORS("Params::getBootstrapPeers");
  autoBootstrap = containsKey(MACE_AUTO_BOOTSTRAP_PEERS);
  string s;
  if (autoBootstrap) {
    s = get<string>(MACE_AUTO_BOOTSTRAP_PEERS, "");
  }
  else {
    s = get<string>(MACE_BOOTSTRAP_PEERS, "");
  }

  if (s.size()) {
    istringstream is(s);
    while (!is.eof()) {
      string h;
      is >> h;
      MaceKey k(ipv4, h);
      if (k != laddr) {
        peers.insert(k);
      }
    }
  }

  if (autoBootstrap && peers.empty()) {
    maceerr << "No MACE_AUTO_BOOTSTRAP_PEERS defined" << Log::endl;
//     ASSERT(0);
  }
}


Params* Params::_inst;

}
