/* 
 * FileWebObject.cc : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, James W. Anderson, Ryan Braud, Charles Killian
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
#include "FileUtil.h"

#include "FileWebObject.h"
#include "HttpResponse.h"

using namespace std;

StringHMap FileWebObject::contentTypes;
PathFileStreamMap FileWebObject::files;
StringCountHMap FileWebObject::fileCount;

FileWebObject::FileWebObject(string p, bool head) throw (FileException) :
  WebObject(head), path(p) {
  fp = 0;
  if (files.find(p) != files.end()) {
    fp = files[p];
    fileCount[p]++;
  }
  else {
    loadContentTypes();
    fp = new fstream();
    fp->open(path.c_str(), fstream::in);
    files[p] = fp;
    fileCount[p] = 1;
  }

  offset = 0;
  if (!fp || !fp->is_open()) {
    throw FileException("open: " + path);
  }

  struct stat sbuf;
  FileUtil::statWithErr(path, sbuf);
  if (S_ISDIR(sbuf.st_mode)) {
    throw FileException("open: " + path + " is dir");
  }
  setLength(sbuf.st_size);

  // set the content type

  size_t i = path.find_last_of(".");
  if (i != string::npos) {
    string t = path.substr(i + 1);
    if (contentTypes.containsKey(t)) {
      setContentType(contentTypes[t]);
    }
  }
} // FileWebObject

void FileWebObject::loadContentTypes() {
  if (contentTypes.empty()) {
    // load types
    fstream types("/etc/mime.types", fstream::in);
    while (!types.eof()) {
      char buf[512];
      types.getline(buf, 512);
      string l(buf);
      if (l == "" || l[0] == '#') {
	continue;
      }
      istringstream ss(l);
      string type;
      ss >> type;
      while (!ss.eof()) {
	string ext;
	ss >> ext;
	contentTypes[ext] = type;
      }
    }
  }
} // loadContentTypes

void FileWebObject::close() {
  fileCount[path]--;
  if ((fileCount[path] == 0) && fp->is_open()) {
    fp->close();
    delete fp;
    fp = 0;
    files.erase(path);
    fileCount.erase(path);
  }
} // close

void FileWebObject::read(char* buf, uint len) throw (HttpServerException) {
  fp->seekg(offset);
  fp->read(buf, len);
  offset += len;
} // read

