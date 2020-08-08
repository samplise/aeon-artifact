/* 
 * filefind.cc : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, James W. Anderson, Charles Killian
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
// #include <sys/wait.h>
#include <signal.h>

#include "params.h"
#include "Scheduler.h"
#include "Find-init.h"
#include "SysUtil.h"

using namespace mace;
using namespace Find_namespace;
using std::cout;
using std::cerr;
using std::endl;

bool done = false;

void shutdownHandler(int sig) {
  done = true;
} // shutdownHandler

class F : public FileFindHandler {
public:
  void atFile(const std::string& path, const stat_t& sbuf, registration_uid_t rid) {
    cout << "FILE " << path << endl;
  }
  bool atDir(const std::string& path, const stat_t& sbuf, registration_uid_t rid) {
    cout << "DIR  " << path << endl;
    return true;
  }
  void leavingDir(const std::string& path, const StringList& contents, registration_uid_t rid) {
    cout << "---- leaving " << path << ": " << contents << endl;
  }
  void atLink(const std::string& path, const stat_t& sbuf, registration_uid_t rid) {
    cout << "LINK " << path << endl;
  }
  bool statError(const std::string& path, const FileException& e, registration_uid_t rid) {
    cout << "stat error " << path << " " << e << endl;
    return true;
  }
  bool readDirError(const std::string& path, const FileException& e, registration_uid_t rid) {
    cout << "read dir error " << path << " " << e << endl;
    return true;
  }
  void findComplete(const std::string& root, registration_uid_t rid) {
//     cout << "find complete for " << root << endl;
    shutdownHandler(0);
  }
};

int main(int argc, const char** argv) {
  if (argc != 2) {
    cerr << "usage: " << argv[0] << " path" << endl;
    return 0;
  }

//   Log::autoAddAll();

  SysUtil::signal(SIGINT, &shutdownHandler);
  SysUtil::signal(SIGTERM, &shutdownHandler);
  
  FileFindServiceClass* finder = &new_Find_FileFind();
  F fh;
  finder->maceInit();
  finder->registerUniqueHandler(fh);
  finder->beginFind(argv[1]);

  while (!done) {
    SysUtil::sleepm(100);
  }

  finder->maceExit();
  Scheduler::haltScheduler();
  return 0;
}
