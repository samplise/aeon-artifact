/* 
 * HttpClient.h : part of the Mace toolkit for building distributed systems
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
#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include "ProcessingQueue.h"

#include "HttpResponse.h"
#include "HttpClientException.h"

// HttpClient provides both synchronous (get, post, getUrl, postUrl)
// and asynchonous (getUrlAsync, postUrlAsync) methods.  Calling a
// synchronous method while waiting for the result of an asynchronous
// call will yield undefined responses.  It is perfectly acceptible,
// however, to have multiple outstanding asynchronous resquests.

class HttpClient;
class HttpClientResponseState;

class HttpClientResponseHandler {
public:
  virtual void postRequestResult(HttpClientResponseState s) = 0;
  virtual void getRequestResult(HttpClientResponseState s) = 0;
  virtual ~HttpClientResponseHandler() {}
}; // HttpClientResponseHandler

typedef void (HttpClientResponseHandler::*HttpClientResponseCallback)
  (HttpClientResponseState);

class HttpClientResponseState {

  friend class HttpClient;

public:
  HttpClientResponseState(const std::string& host, uint16_t port, 
			  const std::string& url,
			  HttpClientResponseHandler* c = 0,
			  HttpClientResponseCallback f = 0)
    : hasException(false), response(host, port, url), exception(""),
      obj(c), callback(f) { }
  HttpResponse& getResponse() throw(HttpClientException) {
    if (hasException) {
      throw exception;
    }
    return response;
  }

  const std::string& getRequest() const { return request; }

private:
  bool hasException;
  HttpResponse response;
  HttpClientException exception;
  HttpClientResponseHandler* obj;
  HttpClientResponseCallback callback;
  std::string request;
}; // HttpClientResponseState

class HttpClient {

  typedef ProcessingQueueCallbacks::callback_copy<HttpClient*,
			HttpClient,
			HttpClientResponseState*,
			HttpClientResponseState*> HttpClientRequestCallback;

  typedef ProcessingQueue<HttpClientRequestCallback, HttpClientResponseState*,
			  HttpClientResponseState*> HttpClientRequestQueue;

public:
  static const std::string DEFAULT_USER_AGENT;
  
  HttpClient(bool async = true, const std::string& userAgent = DEFAULT_USER_AGENT);
  HttpClient(const std::string& server, uint16_t port = 80,
	     bool async = true, const std::string& userAgent = DEFAULT_USER_AGENT);
  virtual ~HttpClient();

  // get and post take an absolute url
  virtual HttpResponse get(const std::string& url, 
			   const std::string& httpVersion = "1.1")
    throw (HttpClientException);
  virtual HttpResponse post(const std::string& url, 
			    const std::string& contentType,
			    const std::string& content,
			    const std::string& httpVersion = "1.1")
    throw (HttpClientException);
  // getUrl and postUrl take relative urls and assume that setServer 
  // has been called
  virtual void setServer(const std::string& host, uint16_t port);
  virtual std::string setServerFromUrl(std::string url);
  virtual HttpResponse rawRequest(const std::string& request)
    throw (HttpClientException);
  virtual HttpResponse getUrl(const std::string& url,
			      const std::string& httpVersion = "1.1",
			      bool keepAlive = true,
			      const StringHMap& headers = StringHMap())
    throw (HttpClientException);
  virtual HttpResponse postUrl(const std::string& url, 
			       const std::string& contentType,
			       const std::string& content,
			       const std::string& httpVersion = "1.1",
			       bool keepAlive = true) 
    throw (HttpClientException);
  virtual void getUrlAsync(const std::string& url, 
			   const std::string& httpVersion = "1.1",
			   bool keepAlive = true,
			   const StringHMap& headers = StringHMap(),
			   HttpClientResponseHandler* c = 0,
			   HttpClientResponseCallback f = 0);
  virtual void postUrlAsync(const std::string& url, 
			    const std::string& contentType,
			    const std::string& content,
			    const std::string& httpVersion = "1.1", 
			    bool keepAlive = true,
			    const StringHMap& headers = StringHMap(),
			    HttpClientResponseHandler* c = 0,
			    HttpClientResponseCallback f = 0);
  virtual bool hasAsyncResponse();
  virtual bool isBusy() const;
  // if any exceptions occur while processing the request, they will
  // be thrown when getAsyncResponse is called
  virtual HttpResponse getAsyncResponse() throw (HttpClientException);

  virtual void setResponseSignal(pthread_cond_t* sig, pthread_mutex_t* siglock);
  virtual void clearResponseSignal();
  
  virtual void closeConnection();
  virtual void testServerClosed();
  virtual bool isConnected() const;
  
protected:
  virtual HttpClientResponseState* execute(HttpClientResponseState* request);

private:
  void init(bool async);
  void sendRequest(const std::string& req) throw (HttpClientException);
  void readResponse(HttpResponse& response) throw (HttpClientException);
  std::string getRequest(const std::string& method, const std::string& path,
			 const std::string& version, bool keepAlive,
			 const StringHMap& headers,
			 const std::string& contentType = "",
			 const std::string& content = "");
  // return true if a new connection is made
  void connectToServer() throw (HttpClientException);
  int connectToServer(const struct sockaddr_in& sin) 
    throw (HttpClientException);

private:
  std::string userAgent;
  int s;
  std::string server;
  uint16_t port;

  HttpClientRequestQueue* requests;

  std::string readBuffer;
}; // HttpClient

#endif // HTTP_CLIENT_H
