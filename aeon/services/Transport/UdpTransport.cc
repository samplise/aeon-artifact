/* 
 * UdpTransport.cc : part of the Mace toolkit for building distributed systems
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
#include "massert.h"
#include "Accumulator.h"

#include "TransportScheduler.h"
#include "UdpTransport.h"
#include "lib/ServiceFactory.h"
#include "lib/ServiceConfig.h"

using std::max;
using std::cout;
using std::cerr;
using std::endl;
using std::ostream;

const size_t UdpTransport::MAX_MESSAGE_SIZE = 65507;
const size_t UdpTransport::MAX_PAYLOAD_SIZE = MAX_MESSAGE_SIZE - TransportHeader::ssize();

namespace UdpTransport_namespace {

    UdpTransportService::~UdpTransportService() {
        mace::ServiceFactory<TransportServiceClass>::registerInstance("UdpTransport", this);
    } // ~UdpTransport

  TransportServiceClass& new_UdpTransport_Transport(int portOffset) {
    UdpTransportService *t = new UdpTransportService(portOffset);
    mace::ServiceFactory<TransportServiceClass>::registerInstance("UdpTransport", t);
    ServiceClass::addToServiceList(*t);
    return *t;
  }

  TransportServiceClass& private_new_UdpTransport_Transport(int portOffset) {
    return *(new UdpTransportService(portOffset));
  }

  TransportServiceClass& configure_new_UdpTransport_Transport(bool shared) {
    if (shared) {
      return new_UdpTransport_Transport(INT_MAX);
    } else {
      return private_new_UdpTransport_Transport(INT_MAX);
    }
  }

  void load_protocol() {
    if (macesim::SimulatorFlags::simulated()) {
        //std::cout << "Not loading UdpTransport protocol during simulation" << Log::endl;
        //FYI - Ken can do so here as well using compile-time flag?
    } else {
        mace::ServiceFactory<TransportServiceClass>::registerService(&configure_new_UdpTransport_Transport, "UdpTransport");
        mace::ServiceConfig<TransportServiceClass>::registerService("UdpTransport", mace::makeStringSet("lowlatency,ipv4,UdpTransport",","));
    }
  }

} // UdpTransport_namespace

UdpTransport::UdpTransport(int portOffset) : BaseTransport(portOffset), rc(0), wc(0) {
  rbuf = new char[MAX_MESSAGE_SIZE];
} // UdpTransport

UdpTransport::~UdpTransport() {
  delete rbuf;
} // ~UdpTransport

UdpTransportPtr UdpTransport::create(int portOffset) {
  UdpTransportPtr p(new UdpTransport(portOffset));
  TransportScheduler::add(p);
  return p;
} // create

void UdpTransport::doIO(CONST_ISSET fd_set& rset, CONST_ISSET fd_set& wset, uint64_t st) {
  ADD_SELECTORS("UdpTransport::doIO");
  static Accumulator* readaccum = Accumulator::Instance(Accumulator::UDP_READ);
  static Accumulator* netaccum = Accumulator::Instance(Accumulator::NETWORK_READ);

  if (!running) {
    return;
  }


  if (FD_ISSET(transportSocket, &rset)) {
    sockaddr_in sa;
    while (true) {
      socklen_t salen = sizeof(sa);
      int r = recvfrom(transportSocket, rbuf, MAX_MESSAGE_SIZE, 0,
		       (struct sockaddr*)&sa, &salen);
      if (r <= 0) {
	if (SockUtil::errorWouldBlock()) {
	  return;
	}
	else {
	  Log::perror("recvfrom");
	  // XXX
	  ABORT("recvfrom");
	}
      }

//       maceout << "read " << r << " ssize=" << TransportHeader::ssize() << Log::endl;

      ASSERT(r > (int)TransportHeader::ssize());

      readaccum->accumulate(r);
      netaccum->accumulate(r);
      rc += r;

      StringPtr hdr = StringPtr(new std::string(rbuf, TransportHeader::ssize()));
      size_t rhdrsize = TransportHeader::deserializeSize(*hdr);
      ASSERT(rhdrsize < MAX_PAYLOAD_SIZE);
      ASSERT(rhdrsize == r - TransportHeader::ssize());

      StringPtr buf = StringPtr(new std::string(rbuf + TransportHeader::ssize(),
						rhdrsize));
      //       macedbg(2) << "receiving message(" << buf->size() << ")=" << Log::toHex(*buf) << Log::endl;
//       std::cerr << "buf=" << Log::toHex(*buf) << std::endl;

      if (!shuttingDown) {
        rq.push(buf);
        rhq.push(hdr);
        signalDeliver();
      }
    }
  }
} // doIO

bool UdpTransport::runDeliverCondition(ThreadPoolType* tp, uint threadId) {
  ADD_SELECTORS("UdpTransport::runDeliverCondition");

  //   macedbg(1) << "Called on threadId " << threadId << Log::endl;

  unregisterHandlers();
  if (!rhq.empty()) { 
    //     macedbg(1) << "true <- !rhq.empty()" << Log::endl;
    return true; 
  }
  if (shuttingDown) { 
    //     macedbg(1) << "true <- shuttingDown" << Log::endl;
    return true;
  }
  //   macedbg(1) << "false <- Nothing to Do!" << Log::endl;
  return false;
}

//XXX: Concern - this extra functionality while holding the tp lock could slow down the IO thread.
void UdpTransport::runDeliverSetup(ThreadPoolType* tp, uint threadId) {
  ADD_SELECTORS("UdpTransport::runDeliverSetup");

  //   macedbg(1) << "runDeliverSetup( " << threadId << " )" << Log::endl;
  ASSERT(shuttingDown || !rhq.empty()); //Remove once things are working.

  DeliveryData& d = tp->data(threadId);

  if (shuttingDown && !rhq.empty() && dataHandlers.empty()) {
    //     macedbg(1) << "Cancelling " << rhq.size() << " messages because shuttingDown and dataHandlers.empty()" << Log::endl;
    rhq.clear();
    rq.clear();
  }

  if (rhq.empty()) {
    //     macedbg(1) << "State to FINITO" << Log::endl;
    d.deliverState = FINITO;
  } else {
    //     macedbg(1) << "Dequeueing message" << Log::endl;
    d.deliverState = DELIVER;

    d.shdr = *rhq.front();
    rhq.pop();

    d.s = *rq.front();
    rq.pop();

    //Get ticket lock here...
    //           mace::AgentLock::getNewTicket();
    // chuangw: not needed any more
    //ThreadStructure::newTicket();
    
    deliverDataSetup(tp, d);
  }

}

void UdpTransport::runDeliverProcessUnlocked(ThreadPoolType* tp, uint threadId) {
  ADD_SELECTORS("UdpTransport::runDeliverProcessUnlocked");
  DeliveryData& d = tp->data(threadId);

  //   macedbg(1) << "runDeliverProcessUnlocked( " << threadId << " ) -- data.deliverState: " << d.deliverState << Log::endl;

  if (d.deliverState == DELIVER) {
    deliverData(tp->data(threadId));
  } else {
    tp->halt();
  }
}

void UdpTransport::runDeliverFinish(ThreadPoolType* tp, uint threadId) {}


// void UdpTransport::runDeliverThread() {
//   // So it appears the UdpTransport is not using the thread pool?
//   ADD_SELECTORS("UdpTransport::runDeliverThread");
//   ABORT("UNUSED!");
//   while (!shuttingDown || !rhq.empty()) {
//     unregisterHandlers();
//     if (rhq.empty()) {
//       waitForDeliverSignal();
//       continue;
//     }
// 
//     while (!rhq.empty()) {
//       StringPtr shdr = rhq.front();
//       rhq.pop();
//       StringPtr sbuf = rq.front();
//       rq.pop();
//       deliverData(*shdr, *sbuf, 0, &suspended);
//     }
//   }
// } // runDeliverThread

bool UdpTransport::sendData(const MaceAddr& src, const MaceKey& dest,
			    const MaceAddr& nextHop, registration_uid_t rid,
			    const std::string& ph, const std::string& s, bool checkQueueSize, bool rts) {
  ADD_SELECTORS("UdpTransport::sendData");
  // if (!macedbg(1).isNoop()) {
  //   macedbg(1) << "STARTING (UdpTransport::sendData)" << Log::endl;
  // }
  static Accumulator* sendaccum = Accumulator::Instance(Accumulator::TRANSPORT_SEND);
  static Accumulator* writeaccum = Accumulator::Instance(Accumulator::UDP_WRITE);
  static Accumulator* netaccum = Accumulator::Instance(Accumulator::NETWORK_WRITE);

  ASSERT(s.size() <= MAX_PAYLOAD_SIZE);

  const SockAddr& ma = getNextHop(nextHop);

  lock();
  SourceTranslationMap::const_iterator i = translations.find(ma);
  unlock();

  std::string m;
  TransportHeader::serialize(m, ((i == translations.end()) ? src : i->second),
			     dest.getMaceAddr(), rid, 0, ph.size() + s.size());

  m.append(ph);
  m.append(s);

//   maceout << "sending pipeline header of " << ph.size() << " m=" << m.size() << Log::endl;
  bool success = true;

  sockaddr_in sa;
  SockUtil::fillSockAddr(ma.addr, ma.port + portOffset, sa);

  ASSERT(m.size() <= MAX_MESSAGE_SIZE);

  int r = sendto(transportSocket, m.data(), m.size(), 0,
		 (const struct sockaddr*)&sa, sizeof(sa));

  if (r < 0) {
    Log::perror("sendto");
    success = false;
    // XXX
    ABORT("sendto");
  }

  writeaccum->accumulate(m.size());
  netaccum->accumulate(m.size());
  sendaccum->accumulate(s.size());
  wc += m.size();
  // if (!macedbg(1).isNoop()) {
  //   macedbg(1) << "ENDING (UdpTransport::sendData)" << Log::endl;
  // }
  return success;
} // sendData
