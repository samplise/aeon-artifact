/* 
 * Pipeline.h : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, Charles Killian, James W. Anderson, Ryan Braud
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
#ifndef PIPELINE_H
#define PIPELINE_H

#include <string>
#include "ScopedStackExecution.h"
#include "TimeUtil.h"
#include "Util.h"
#include "pip_includer.h"
#include "BinaryLogObject.h"
#include "hash_string.h"

/*
 * Message pipeline uses:
 *
 * - [temp] connection pipeline by tracking state
 *   - connection token
 *   - identification exchange
 * - Message ID
 * - Pip Path ID
 * - Logical Clock
 * - Experimental statistical collection
 * - Debug logging
 * - Message transformations
 *   - Selective encryption (certain messages, channels, or addresses)
 *   - Message compression
 *   - Message dropping
 *   - Erasure or network coding? 
 *   - ECC (e.g. Reed-Solomon coding)
 * - Digitally signing messages (not needed with TLS?)
 *
 * Pipeline requirements and limitations:
 *
 * - Must be re-entrant [flexible, refine]
 *   - 1 sending thread at a time
 *   - 1 deliver thread at a time
 * - cannot change source or destination address
 * - Must be lock-free
 * - Operate on 1 message at a time
 * - Can they inject messages?
 *   - could then do acknowledgements
 * - Can they close connections?
 *
 */

class PipelineElement {
  public:
    PipelineElement() {}
    virtual void routeData(const MaceKey& dest, std::string& pipelineHeader, std::string& message, registration_uid_t rid) = 0;
    virtual void deliverData(const MaceKey& src, std::string& message, registration_uid_t rid) = 0;
    virtual void messageError(const MaceKey& dest, std::string& pipelineHeader, std::string& message, registration_uid_t rid) = 0; 
    virtual ~PipelineElement() { }

};

class Pipeline : public PipelineElement {
  public:
    typedef mace::deque<PipelineElement*, mace::SoftState> PipelineList;

  private: 
    PipelineList elements;

  public:
    void pushBackElement(PipelineElement* p) { elements.push_back(p); }
    void pushFrontElement(PipelineElement* p) { elements.push_front(p); }
    
    void routeData(const MaceKey& dest, std::string& pipelineHeader, std::string& message, registration_uid_t rid) {
      for (PipelineList::iterator i = elements.begin(); i != elements.end(); i++) {
        (*i)->routeData(dest, pipelineHeader, message, rid);
      }
    }
    void deliverData(const MaceKey& src, std::string& message, registration_uid_t rid) {
      for (PipelineList::reverse_iterator i = elements.rbegin(); i != elements.rend(); i++) {
        (*i)->deliverData(src, message, rid);
      }
    }
    void messageError(const MaceKey& dest, std::string& pipelineHeader, std::string& message, registration_uid_t rid) {
      for (PipelineList::reverse_iterator i = elements.rbegin(); i != elements.rend(); i++) {
        (*i)->messageError(dest, pipelineHeader, message, rid);
      }
    }

    Pipeline(PipelineList list) : elements(list) {}

    ~Pipeline() {
      while (!elements.empty()) {
        delete elements.front();
        elements.pop_front();
      }
    }

};

class FlowLog : public mace::BinaryLogObject, public mace::PrintPrintable {
  public: 
    uint8_t pipeline;
    MaceKey source;
    uint64_t time;
    std::string path; ///< only set if PIP is being used
    uint32_t size;

    FlowLog() {}
    FlowLog(uint8_t p, const MaceKey& s, uint64_t t, const std::string& pa, uint32_t sz) : pipeline(p), source(s), time(t), path(pa), size(sz) {}

  protected:
    static mace::LogNode* rootLogNode;
    
    void serialize(std::string& __str) const {
      mace::serialize(__str, &pipeline);
      mace::serialize(__str, &source);
      mace::serialize(__str, &time);
      mace::serialize(__str, &path);
      mace::serialize(__str, &size);
    }
    
    int deserialize(std::istream& is) throw(mace::SerializationException) {
      int b = 0;
      b += mace::deserialize(is, &pipeline);
      b += mace::deserialize(is, &source);
      b += mace::deserialize(is, &time);
      b += mace::deserialize(is, &path);
      b += mace::deserialize(is, &size);
      return b;
    }

    void print(std::ostream& out) const {
      out << "Message of size " << size << " from " << source << " pipeline " << (uint32_t)pipeline << " at time " << time << " under path ";
      mace::printItem(out, &path);
    }

    const std::string& getLogType() const {
      static const std::string type = "FLOW";
      return type;
    }
    
    const std::string& getLogDescription() const {
      static const std::string desc = "FlowPipeline";
      return desc;
    }
    
    LogClass getLogClass() const {
      return STRUCTURE;
    }
    
    mace::LogNode*& getChildRoot() const {
      return rootLogNode;
    }
};

/// Handles PIP path propagation, as well as matching send with receive of messages.
class FlowPipeline : public PipelineElement {
  private:
    static uint8_t pipelineCount;
    const uint8_t pipeline;
    const MaceKey localaddr;

  public:
    FlowPipeline() : pipeline(pipelineCount++), localaddr(MaceKey(ipv4, Util::getMaceAddr())) {}

    void routeData(const MaceKey& dest, std::string& pipelineHeader, std::string& message, registration_uid_t rid) {
      static const log_id_t logId = Log::getId("FlowPipeline::routeData");
      FlowLog flowdata(pipeline, localaddr, TimeUtil::timeu(), annotate_get_path(), message.size() + pipelineHeader.size());

      if (PIP) {
	ANNOTATE_SEND_STR(NULL, 0, message.size() + pipelineHeader.size(), PRIu8 "-%s-" PRIu64, flowdata.pipeline, flowdata.source.toString().c_str(), flowdata.time);
      }
      Log::binaryLog(logId, flowdata, 0);

      std::string hdr;
      mace::serialize(hdr, &flowdata.pipeline);
      mace::serialize(hdr, &flowdata.time);
      if (PIP) {
        mace::serialize(hdr, &flowdata.path);
      }
      pipelineHeader.insert(0, hdr);
    }
    void deliverData(const MaceKey& src, std::string& message, registration_uid_t rid) {
      static const log_id_t logId = Log::getId("FlowPipeline::deliverData");
      std::istringstream in(message);

      FlowLog flowdata;
      mace::deserialize(in, &flowdata.pipeline);
      mace::deserialize(in, &flowdata.time);
      if (PIP) {
        mace::deserialize(in, &flowdata.path);
      }
      message = message.substr(in.tellg());

      flowdata.source = src;
      flowdata.size = message.size();
      Log::binaryLog(logId, flowdata, 0);

      if (PIP) {
	ANNOTATE_SET_PATH_ID(NULL, 0, flowdata.path.data(), flowdata.path.size());
	ANNOTATE_RECEIVE_STR(NULL, 0, message.size(), PRIu8 "-%s-" PRIu64, flowdata.pipeline, flowdata.source.toString().c_str(), flowdata.time);
      }
    }
    void messageError(const MaceKey& dest, std::string& pipelineHeader, std::string& message, registration_uid_t rid) {
      static const log_id_t logId = Log::getId("FlowPipeline::deliverData");
      std::istringstream in(pipelineHeader);

      FlowLog flowdata;
      mace::deserialize(in, &flowdata.pipeline);
      mace::deserialize(in, &flowdata.time);
      if (PIP) {
        mace::deserialize(in, &flowdata.path);
      }
      pipelineHeader = pipelineHeader.substr(in.tellg());

      flowdata.source = localaddr;
      flowdata.size = message.size() + pipelineHeader.size();
      Log::binaryLog(logId, flowdata, 0);

      if (PIP) {
	ANNOTATE_SET_PATH_ID(NULL, 0, flowdata.path.data(), flowdata.path.size());
	ANNOTATE_RECEIVE_STR(NULL, 0, message.size() + pipelineHeader.size(), PRIu8 "-%s-" PRIu64, flowdata.pipeline, flowdata.source.toString().c_str(), flowdata.time);
      }
    }
};

/// Pipeline element to handle distributed logical clocks.  Works with the LogicalClass object.
/**
 * \todo Add BinaryLogObject for logging send and receive
 */
class LogicalClockPipeline : public PipelineElement {
public:
  class MessageId : public mace::BinaryLogObject, public mace::PrintPrintable {
  public:
    uint64_t id;
    mace::LogicalClock::lclockpath_t path;
    // XXX make this be explicitly sized
    uint32_t hash;

  public:
    MessageId() : id(0), path(0), hash(0) { }
    MessageId(uint64_t i, mace::LogicalClock::lclockpath_t p, uint32_t h) :
      id(i), path(p), hash(h) { }

    void print(std::ostream& out) const {
      out << id << " " << path << " " << hash;
    }

    void serialize(std::string& __str) const { }
    
    int deserialize(std::istream& is) throw(mace::SerializationException) {
      return 0;
    }
    
    void sqlize(mace::LogNode* node) const {
      int index = node->nextIndex();
      
      if (index == 0) {
	std::string out = "CREATE TABLE " + node->logName +
	  " (_id INT PRIMARY KEY, messageId NUMERIC(20, 0), path BIGINT);\n";
	Log::logSqlCreate(out, node);
      }
      std::ostringstream out;
      out << node->logId << "\t" << index << "\t" << id << "\t" 
	  << path << std::endl;
      ASSERT(fwrite(out.str().c_str(), out.str().size(), 1, node->file) > 0);
    }
    
    const std::string& getLogType() const {
      static const std::string type = "__MID";
      return type;
    }
    
    const std::string& getLogDescription() const {
      static const std::string desc = "LogicalClockPipeline";
      return desc;
    }
    
    LogClass getLogClass() const {
      return STRUCTURE;
    }
    
    mace::LogNode*& getChildRoot() const {
      return rootLogNode;
    }
    
  protected:
    static mace::LogNode* rootLogNode;
  };

protected:
  static uint64_t maxId;
  const uint32_t localaddr; // Used as the low 32 bits of clock values to make them unique.  Note: not necessarily perfect.

public:
  LogicalClockPipeline() : localaddr(Util::getAddr()) {
  }
  
  void routeData(const MaceKey& dest, std::string& pipelineHeader,
		 std::string& message, registration_uid_t rid) {
    static const log_id_t logId = Log::getId("LogicalClockPipeline::routeData");
    static const bool SS = params::get("LOG_SCOPED_SERIALIZE", false);

    mace::LogicalClock& cl = mace::LogicalClock::instance();
//       mace::LogicalClock::lclock_t t = cl.getClock();
//       mace::LogicalClock::lclockpath_t p = cl.getPath();
    mace::LogicalClock::lclock_t t;
    mace::LogicalClock::lclockpath_t p;
    bool shouldLog;
    cl.getClock(t, p, shouldLog);
    std::string clock;
    uint64_t id = TimeUtil::timeu();
    id = (id << 32) | localaddr;
    if (id <= maxId) {
      id = maxId + 1;
    }
    maxId = id;
    mace::serialize(clock, &t);
    mace::serialize(clock, &p);
    mace::serialize(clock, &id);
    mace::serialize(clock, &shouldLog);
    pipelineHeader.insert(0, clock);
    if (shouldLog) {
      uint32_t h = 0;
      if (SS) {
	h = HashString::hash(message);
      }
      Log::binaryLog(logId, MessageId(id, p, h));
    }
  }
  void deliverData(const MaceKey& src, std::string& message,
		   registration_uid_t rid) {
    std::istringstream in(message);
    mace::LogicalClock::lclock_t clockv;
    mace::LogicalClock::lclockpath_t pathv;
    bool shouldLog;
    uint64_t id;
    mace::deserialize(in, &clockv);
    mace::deserialize(in, &pathv);
    mace::deserialize(in, &id);
    mace::deserialize(in, &shouldLog);
    
    message = message.substr(in.tellg());
    mace::LogicalClock::instance().updatePending(clockv, pathv, id, shouldLog);
  }
  void messageError(const MaceKey& dest, std::string& pipelineHeader,
		    std::string& message, registration_uid_t rid) {
    std::istringstream in(pipelineHeader);
    mace::LogicalClock::lclock_t clockv;
    mace::LogicalClock::lclockpath_t pathv;
    uint64_t id;
    mace::deserialize(in, &clockv);
    mace::deserialize(in, &pathv);
    mace::deserialize(in, &id);
    //don't do anything, but need to remove bytes...
    message = message.substr(in.tellg());
  }
};



/*
  string phs;
  if (PIP) {
    static uint32_t pipSeq = 0;
    int size;
    const void* b;
    b = ANNOTATE_GET_PATH_ID(&size);
    ANNOTATE_SEND_STR(NULL, 0, buf.size(), "%.8x:%d-%d", laddr, lport, pipSeq);
    mace::string pathId((const char*)b, size);
    //     maceLog("size %d sendstr %.8x:%d-%d path %s\n", buf.size(), laddr, lport, pipSeq, pathId.toString().c_str());
    PipTransportHeader ph(pathId, pipSeq);
    pipSeq++;
    ph.serialize(phs);
  }
  else if (MSG_MATCHING) {
    static uint32_t pipSeq = 0;
    macedbg(1) << "Sending TCP pkt from src=" << th.src.local << " to dest=" << th.dest << " on port="<< ntohs(lport) << " with seq=" << pipSeq << Log::endl;
    std::string pathId = "msg_matching";
    PipTransportHeader ph(pathId, pipSeq);
    pipSeq++;
    ph.serialize(phs);
  }
  th.pip_size = phs.size();

  if (PIP || MSG_MATCHING) {
    wq.push(phs);
    wqueued += phs.size();
    wqcount++;
    //     maceout << "hdr.size " << hdr.size() << " phs.size " << phs.size() << " buf.size " << buf.size() << Log::endl;
  }


    if (PIP || MSG_MATCHING)
    {
      mace::serialize(s, &pip_size);
    }



    if (PIP || MSG_MATCHING)
    {
      o += mace::deserialize(in, &pip_size);
    } 
    else {
      pip_size = 0;
    }




    if (PIP || MSG_MATCHING) {
      memcpy(&f, s.data() + ssize() - sizeof(size_t) - sizeof(size_t) - sizeof(f), sizeof(f));
    }




    if (PIP || MSG_MATCHING)
    {
      uint32_t ns2;
      memcpy(&ns2, s.data() + ssize() - sizeof(ns) - sizeof(ns2), sizeof(ns2));
      return ntohl(ns) + ntohl(ns2);
    }




  static uint32_t deserializePipSize(const std::string& s) {
    if(PIP || MSG_MATCHING) {
      uint32_t ns;
      memcpy(&ns, s.data() + ssize() - sizeof(ns), sizeof(ns));
      return ntohl(ns);
    } else {
      return 0;
    }
  }





class PipTransportHeader {
public:
  PipTransportHeader(const std::string& pathId = "", int mn = 0) :
    pathId(pathId), mnum(mn) { }
  virtual ~PipTransportHeader() { }

  void serialize(std::string& s) const {
    mace::serialize(s, &pathId);
    mace::serialize(s, &mnum);
  } // serialize

  int deserialize(std::istream& in) throw (mace::SerializationException) {
    int o = 0;
    o += mace::deserialize(in, &pathId);
    o += mace::deserialize(in, &mnum);
    return o;
  } // deserialize

  mace::string pathId;
  int mnum;
}; // PipTransportHeader




  waitingForPipHeader(true),
  pipHeaderSize(0),
  pq(QUEUE_SIZE) 



    if (PIP || MSG_MATCHING)
    {
      std::string& phs = buffed.front();

      std::istringstream in(phs);
      PipTransportHeader ph;
      ph.deserialize(in);

      if(PIP){
        ANNOTATE_SET_PATH_ID(NULL, 0, ph.pathId.data(), ph.pathId.size());
        ANNOTATE_NOTICE(NULL, 0, "MessageError %s error %d", hdr.dest.toString().c_str(), error);
      //       ANNOTATE_RECEIVE_STR(NULL, 0, buf.size(), "%.8x:%d-%d", laddr, lport, pipSeq); FIXME
      }
      buffed.pop();
    } 




    if (PIP || MSG_MATCHING)
    {
      if (waitingForPipHeader) {
	if (rqueued < pipHeaderSize) {
	  return;
	}
	readString(phdrstr, pipHeaderSize);
	rqueued -= pipHeaderSize;
	waitingForPipHeader = false;
      }
    }





    if (PIP || MSG_MATCHING)
    {
      pipHeaderSize = 0;
      waitingForPipHeader = true;
      pq.push(phdrstr);
    }





  if(PIP)
  {
    ASSERT(!pq.empty());
    std::string& ps = pq.front();
    PipTransportHeader h;
    std::istringstream in(ps);
    h.deserialize(in);
    ANNOTATE_SET_PATH_ID(NULL, 0, h.pathId.data(), h.pathId.size());
    ANNOTATE_RECEIVE_STR(NULL, 0, buf.size(), "%.8x:%d-%d",
			 raddr, rport, h.mnum);
    pq.pop();
  }
  else if (MSG_MATCHING) {
    ASSERT(!pq.empty());
    std::string& ps = pq.front();
    PipTransportHeader h;
    std::istringstream in(ps);
    h.deserialize(in);

    TransportHeader thdr;
    try {
      istringstream in(hdr);
      thdr.deserialize(in);
    }
    catch (const Exception& e) {
      Log::err() << "Transport deliver deserialization exception: " << e << Log::endl;
      return;
    }
    MaceKey src(ipv4, thdr.src);

    macedbg(1) << "RECEIVED TCP MESSAGE(src_macekey=" << src << ", port=" << ntohs(rport) << ", seq=" << h.mnum << ")" << Log::endl;
    pq.pop();
  }



  std::string phs;
  if(PIP || MSG_MATCHING) {
    static uint32_t pipSeq = 0;
    int size;
    const void* b;
    b = ANNOTATE_GET_PATH_ID(&size);
    ANNOTATE_SEND_STR(NULL, 0, s.size(), "%s:%d-%d", src.toString().c_str(), portOffset, pipSeq);

    if(PIP){
      mace::string pathId((const char*)b, size);
      PipTransportHeader ph(pathId, pipSeq);
      pipSeq++;
      ph.serialize(phs);
    }
    else{
      macedbg(1) << "Sending UDP pkt from src="  << src.local << " to dest=" << dest.getMaceAddr() << " on port=" << src.local.port << " with seq=" << pipSeq << Log::endl;
      std::string pathId = "msg_matching";
      PipTransportHeader ph(pathId, pipSeq);
      pipSeq++;
      ph.serialize(phs);
    }
    //     maceLog("size %d sendstr %.8x:%d-%d path %s\n", buf.size(), laddr, lport, pipSeq, pathId.toString().c_str());
  }


      if (PIP || MSG_MATCHING) {
        std::istringstream tin(*shdr);
        TransportHeader th;
        th.deserialize(tin);
        StringPtr& ps = pipq.front();
        PipTransportHeader h;
        std::istringstream in(*ps);
        h.deserialize(in);
        if(PIP){
          ANNOTATE_SET_PATH_ID(NULL, 0, h.pathId.data(), h.pathId.size());
          ANNOTATE_RECEIVE_STR(NULL, 0, sbuf->size(), "%s:%d-%d", th.src.toString().c_str(), portOffset, h.mnum);
        }else{
          MaceKey src(ipv4, th.src);
          macedbg(1) << "RECEIVED UDP MESSAGE(src_macekey=" << src << ", port=" << ntohs(src.getMaceAddr().proxy.port) << ", seq=" << h.mnum << ")" << Log::endl;
        }
        pipq.pop();
      }


*/

#endif //PIPELINE_H
