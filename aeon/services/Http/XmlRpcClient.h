/* 
 * XmlRpcClient.h : part of the Mace toolkit for building distributed systems
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
#ifndef XML_RPC_CLIENT_H
#define XML_RPC_CLIENT_H

#include "HttpClient.h"
#include "HttpClientException.h"

class XmlRpcClient : protected HttpClient, public HttpClientResponseHandler {
public:
  XmlRpcClient(const std::string& url, bool async = true);
  XmlRpcClient(const std::string& host, uint16_t port, const std::string& url,
	       bool async = true);
  virtual ~XmlRpcClient() = 0;
  virtual void setServer(const std::string& host, uint16_t port) {
    HttpClient::setServer(host, port);
  }
  virtual void closeConnection() {
    HttpClient::closeConnection();
  }

protected:
  std::string url;
}; // XmlRpcClient

template<class T>
class XmlRpcResponseState {
public:
  XmlRpcResponseState(T* ret, const HttpClientException* e = 0)
    : response(ret), exception(e) {
  }

  T getResponse() const throw(HttpClientException) {
    if (exception) {
      throw (*exception);
    }
    return *response;
  }

private:
  T* response;
  const HttpClientException* exception;
};

class XmlRpcVoidResponseState {
public:
  XmlRpcVoidResponseState(const HttpClientException* e = 0) : exception(e) {
  }

  void getResponse() const throw(HttpClientException) {
    if (exception) {
      throw (*exception);
    }
  }

private:
  const HttpClientException* exception;
};

#endif // XML_RPC_CLIENT_H
