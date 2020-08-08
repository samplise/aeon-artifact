/* 
 * SimulatorUDPCommon.cc : part of the Mace toolkit for building distributed systems
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
#include "Util.h"
#include "Log.h"
#include "SimulatorUDPCommon.h"
#include "NumberGen.h"
#include "Accumulator.h"
#include "mace_constants.h"
#include "MaceTypes.h"
#include "mace-macros.h"
#include "ServiceConfig.h"

using std::string;

int SimulatorUDPCommon_load_protocol()
{
  return 0;
}

namespace SimulatorUDP_namespace {

  SimulatorUDPCommonService::SimulatorUDPCommonService(uint16_t port, bool shared) : localPort((port == std::numeric_limits<uint16_t>::max()? Util::getPort() + NumberGen::Instance(NumberGen::PORT)->GetVal() : port)), localNode(SimCommon::getCurrentNode()), listening(false), queuedMessages(SimCommon::getNumNodes()) {
    ADD_SELECTORS("UDP::constructor");

    maceout << "local_address " << SimCommon::getMaceKey(SimCommon::getCurrentNode()) << " port " << port << Log::endl;

    if (shared) {
        mace::ServiceFactory<TransportServiceClass>::registerInstance("SimulatorUDP", this);
    }
    ServiceClass::addToServiceList(*this);
  }

  void SimulatorUDPCommonService::maceInit() { 
    listening = true;
  }

  SimulatorUDPCommonService::~SimulatorUDPCommonService() { 
      mace::ServiceFactory<TransportServiceClass>::unregisterInstance("SimulatorUDP", this);
  }

  bool SimulatorUDPCommonService::isListening() const {
    return listening;
  }
  
  void SimulatorUDPCommonService::maceReset() {
    listening = false;
    handlers.clear();
  }

  registration_uid_t SimulatorUDPCommonService::registerHandler(ReceiveDataHandler& handler, registration_uid_t handlerUid) {
    if (handlerUid == -1) {
      handlerUid = NumberGen::Instance(NumberGen::HANDLER_UID)->GetVal();
    }
    handlers[handlerUid] = &handler;
    return handlerUid;
  }

  registration_uid_t SimulatorUDPCommonService::registerHandler(NetworkErrorHandler& handler, registration_uid_t handlerUid) {
    if (handlerUid == -1) {
      handlerUid = NumberGen::Instance(NumberGen::HANDLER_UID)->GetVal();
    }
    return handlerUid;
  }

  bool SimulatorUDPCommonService::route(const MaceKey& dest, const string& msg, registration_uid_t handlerUid) {
    ADD_SELECTORS("SIMULATOR::UDP::route::pkt::(downcall)");
    static bool allowErrors = params::get("USE_UDP_ERRORS", false);
    
    maceout << "dest=" << dest.toString() << " size=" << msg.size() << " priority=" << 0 << " handlerUid=" << handlerUid << Log::endl;
    uint32_t msgId = getNetMessageId();
    int destNode = SimCommon::getNode(dest);
    maceout << "drop message routed from " << localNode << " to " << destNode << "? (0-no 1-yes) " << Log::endl;
    
    if (allowErrors && RandomUtil::randInt(2, 1, 0)) { //In search mode, either error or non-error may occur.  In gusto mode, no errors may occur
      maceout << "route([dest=" << dest << "][s_deserialized=pkt(id=" << msgId << ", SIM_DROP=1)])" << Log::endl;
    } else {
      maceout << "route([dest=" << dest << "][s_deserialized=pkt(id=" << msgId << ")])" << Log::endl;
      // Modelchecker or simulator specific event-queuing
      queueMessage(destNode, msgId, handlerUid, msg);
    }
    return true; //UDP always appears to send message
  }

  bool SimulatorUDPCommonService::isAvailableMessage(int destNode) const {
    return !queuedMessages[destNode].empty();
  }

  SimulatorMessage SimulatorUDPCommonService::getMessage(int destNode) {
    static bool allowReorder = params::get("USE_UDP_REORDER", true);
    ADD_SELECTORS("SIMULATOR::UDP::getMessage");
    ASSERT(isAvailableMessage(destNode));
    std::deque<SimulatorMessage>& mqueue = queuedMessages[destNode];
    int msg = 0;
    //if (allowReorder && SimCommon::gusto) -- original, buggy code -- only allows reordering during gusto phase
    //if (allowReorder || SimCommon::gusto) -- possible "correct" code for practical model checking -- always allow reordering during gusto
    if (allowReorder && !SimCommon::getGusto()) {
      maceout << "Which message in queue to deliver?" << std::endl;
      if (! maceout.isNoop()) {
        int position = 0;
        for (MessageQueue::const_iterator j = mqueue.begin(); j != mqueue.end(); j++, position++) {
          maceout << position << ": message " << j->messageId << std::endl;
        }
      }
      maceout << Log::endl;
      msg = RandomUtil::randInt(mqueue.size());
    }
    MessageQueue::iterator i = mqueue.begin() + msg;
    SimulatorMessage str = *(i);
    mqueue.erase(i);
    
    enqueueEvent(destNode);
    return str;
  }

  int SimulatorUDPCommonService::getPort() const { return localPort; }
  const MaceKey& SimulatorUDPCommonService::localAddress() const { 
    //     ADD_SELECTORS("UDP::localAddress");
    //     maceLog("called getLocalAddress");
    return SimCommon::getMaceKey(localNode); 
  }

  std::string SimulatorUDPCommonService::simulateEvent(const SimulatorMessage& msg) {
    ASSERT(msg.destination == localNode);
    if (msg.messageType == SimulatorMessage::MESSAGE) {
      recv(msg);
      return "";
    } else if (msg.messageType == SimulatorMessage::DEST_NOT_READY) {
      ASSERT(isAvailableMessage(msg.source));
      SimulatorMessage m = getMessage(msg.source);
      return "id " + boost::lexical_cast<std::string>(m.messageId);
    } else { //msg.messageType == READ_ERROR
      ASSERT(0);
    }
  }
  void SimulatorUDPCommonService::recv(const SimulatorMessage& msg) {
    ADD_SELECTORS("UDP::recv");
    maceout << "from=" << SimCommon::getMaceKey(msg.source).toString() << " dsize=" << msg.msg.size() << Log::endl;
    ReceiveHandlerMap::iterator i = handlers.find(msg.regId);
    if (i == handlers.end()) { macewarn << "Could not find handler with id " << msg.regId << " for msgId " << msg.messageId << " : dropping packet" << Log::endl; return; }
    i->second->deliver(SimCommon::getMaceKey(msg.source), SimCommon::getMaceKey(msg.destination), msg.msg, msg.regId);
  }

  uint32_t SimulatorUDPCommonService::hashState() const {
    static uint32_t hashAnd = (localPort) * localNode;
    static hash_string strHasher;
    uint32_t hashWork = hashAnd;
    for (mace::map < registration_uid_t, ReceiveDataHandler*, mace::SoftState>::const_iterator i = handlers.begin(); i != handlers.end(); i++) {
      hashWork = (hashWork << 1) ^ i->first;
    }
    int node;
    MessageQueueMap::const_iterator i;
    for (i = queuedMessages.begin(), node = 0; i != queuedMessages.end(); i++, node++) {
      hashWork += node;
      for (MessageQueue::const_iterator j = i->begin(); j != i->end(); j++) {
        hashWork ^= strHasher((*j).msg);
      }
    }
    return hashWork;
  }

  void SimulatorUDPCommonService::printState(std::ostream& out) const {
    out << "[SimulatorUDPCommon]" << std::endl;
    out << "[localPort = " << localPort << "][localKey = " << SimCommon::getMaceKey(localNode) << "]";
    out << "[RDH] ";
    for (mace::map < registration_uid_t, ReceiveDataHandler*, mace::SoftState>::const_iterator i = handlers.begin(); i != handlers.end(); i++) {
      out << i->first << " ";
    }
    out << "[/RDH]";
    out << "[/SimulatorUDPCommon]" << std::endl;
  }

  void SimulatorUDPCommonService::print(std::ostream& out) const {
    out << "[localPort = " << localPort << "][localKey = " << SimCommon::getMaceKey(localNode) << "]";
    out << "[RDH] ";
    for (mace::map < registration_uid_t, ReceiveDataHandler*, mace::SoftState>::const_iterator i = handlers.begin(); i != handlers.end(); i++) {
      out << i->first << " ";
    }
    out << "[/RDH]";
    //This is getting printed by printNetwork();
    //     out << "[messages]";
    //     int node;
    //     MessageQueueMap::const_iterator i;
    //     for (i = queuedMessages.begin(), node=0; i != queuedMessages.end(); i++, node++) {
    //       out << "[toDest(" << node << ")]";
    //       for (MessageQueue::const_iterator j = i->begin(); j != i->end(); j++) {
    //         out << "[msg]" << (*j).messageId << "::" << (*j).messageType << "[/msg]";
    //       }
    //       out << "[/toDest(" << node << ")]";
    //     }
    //     out << "[/messages]";
  }

  void SimulatorUDPCommonService::printNetwork(std::ostream& out, bool printState) const {
    out << "[messages]";
    int node;
    MessageQueueMap::const_iterator i;
    for (i = queuedMessages.begin(), node=0; i != queuedMessages.end(); i++, node++) {
      if (i->size()) {
        out << "[toDest(" << node << ")] ";
        for (MessageQueue::const_iterator j = i->begin(); j != i->end(); j++) {
          if (printState)
            out << " " << (*j).regId << " " << (*j).messageType << " " << Base64::encode((*j).msg) << " ";
          else
            out << (*j).messageId << "::" << (*j).messageType << " ";
        }
        out << "[/toDest(" << node << ")]";
      }
    }
    out << "[/messages]";
  }

}
