/* 
 * WebObject.h : part of the Mace toolkit for building distributed systems
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
#ifndef WEB_OBJECT_H
#define WEB_OBJECT_H

#include <string>
#include "HttpServerException.h"

class WebObject {
public:  
  WebObject(bool head = false, bool chunked = false);
  virtual ~WebObject() { close(); }
  virtual std::string getContentType() const { return contentType; }
  virtual unsigned int getLength() const { return length; }
  virtual void close() { }
  virtual void read(char* buf, unsigned int len) throw (HttpServerException);
  virtual bool isHeadRequest() const { return headRequest; }
  virtual bool isChunked() { return chunked; }
  virtual bool hasMoreChunks() { return false; }
  virtual bool isChunkReady() { return false; }
  virtual bool isRaw() { return false; }
  virtual std::string readChunk() throw (HttpServerException) {
    throw HttpServerException("readChunk unsupported");
  }

protected:
  // adds data to the content
  virtual void append(const std::string& content);
  // appends the final data to the content, sets the length
  virtual void appendFinal(const std::string& content);
  void setLength(unsigned int l) { length = l; }
  void setContentType(const std::string& type) { contentType = type; }

protected:
  unsigned int offset;

private:
  unsigned int length;
  std::string contentType;
  std::string content;
  bool headRequest;
  bool chunked;
}; // WebObject

#endif // WEB_OBJECT_H
