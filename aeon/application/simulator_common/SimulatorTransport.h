/* 
 * SimulatorTransport.h : part of the Mace toolkit for building distributed systems
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
#ifndef SIMULATOR_TRANSPORT_H
#define SIMULATOR_TRANSPORT_H

#include <string>

#include "BufferedTransportServiceClass.h"
#include "MaceTypes.h"
#include "TransportServiceClass.h"

struct SimulatorMessage {
  public:
  int source;
  int destination;
  uint32_t messageId;
  
  int forwarder;
  int nextHop;
  
  uint64_t sendTime;
  uint64_t arrivalTime;
  
  enum MessageType { MESSAGE, READ_ERROR, DEST_NOT_READY } messageType;
  registration_uid_t regId;
  std::string msg;
  
  SimulatorMessage(const int& src, const int& dest, uint32_t id,
                   int fwd, int next,
                   uint64_t srctime, uint64_t desttime,
                   const MessageType& mtype, registration_uid_t rid,
                   const std::string& msg)
    : source(src), destination(dest), messageId(id),
    forwarder(fwd), nextHop(next),
    sendTime(srctime), arrivalTime(desttime),
    messageType(mtype), regId(rid),
    msg(msg)
  {}
};

class SimulatorTransport : public virtual TransportServiceClass {
  public:
    enum TransportSignal { BIDI_READ_ERROR, SOURCE_RESET, DEST_READY };
    virtual bool isAvailableMessage(int destNode) const = 0;
    virtual SimulatorMessage getMessage(int destNode) = 0;
    virtual int getPort() const = 0;
    virtual std::string simulateEvent(const SimulatorMessage& msg) = 0;
    virtual bool isListening() const = 0;
    virtual void printNetwork(std::ostream& out, bool printState = false) const = 0;
    virtual void signal(int source, TransportSignal sig) = 0;
    virtual int getNumFlows() const {
      return 0;
    }
  virtual std::string simulateFlushed() { ASSERT(0); }
  virtual std::string simulateCTS(int node) { ASSERT(0); }
  virtual bool hasFlushed() { return false; }
  virtual void prepareDestNotReady(int node, uint64_t t) { }
  virtual bool hasCTS(int node) { return false; }
  virtual BufferStatisticsPtr getStats(int node) {
    ASSERT(0);
  }
};

#endif // SIMULATOR_TRANSPORT_H
