/* 
 * TcpTransport.cc : part of the Mace toolkit for building distributed systems
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
#include <algorithm>
#include <stdexcept>

#include "maceConfig.h"

// #include "HashUtil.h"
#include "SysUtil.h"
// #include "NumberGen.h"
#include "Accumulator.h"
#include "TcpTransport.h"
#include "TransportScheduler.h"
#include "ThreadCreate.h"
#include "FileUtil.h"
#include "ThreadUtil.h"
#include "mace.h"
#include "m_net.h"
#include "lib/ServiceFactory.h"
#include "lib/ServiceConfig.h"

#undef MAX_LOG
#define MAX_LOG 5

using std::max;
using std::cout;
using std::cerr;
using std::endl;
using std::ostream;

namespace TcpTransport_namespace {

TcpTransportService::~TcpTransportService() {
    mace::ServiceFactory<TransportServiceClass>::unregisterInstance("TcpTransport", this);
    mace::ServiceFactory<BufferedTransportServiceClass>::unregisterInstance("TcpTransport", this);
}

  OptionsMap createOptionsMap(TransportCryptoServiceClass::type cryptoFlags,
			      bool merror,
			      uint32_t queueSize,
			      uint32_t threshold,
			      int portoffset,
			      int numDeliveryThreads) {

    OptionsMap m;
    m[TcpTransport_namespace::CRYPTO] = cryptoFlags;
    m[TcpTransport_namespace::UPCALL_MESSAGE_ERROR] = merror;
    if (queueSize != std::numeric_limits<uint32_t>::max()) {
      m[TcpTransport_namespace::QUEUE_SIZE] = queueSize;
    }
    if (threshold != std::numeric_limits<uint32_t>::max()) {
      m[TcpTransport_namespace::THRESHOLD] = threshold;
    }
    if (portoffset != std::numeric_limits<int32_t>::max()) {
      m[TcpTransport_namespace::PORT_OFFSET] = portoffset;
    }
    if (numDeliveryThreads != std::numeric_limits<int32_t>::max()) {
      m[TcpTransport_namespace::NUM_DELIVERY_THREADS] = numDeliveryThreads;
    }
    return m;
  }

  TransportServiceClass& new_TcpTransport_Transport(TransportCryptoServiceClass::type cryptoFlags,
						    bool merror,
						    uint32_t queueSize,
						    uint32_t threshold,
						    int portoffset,
						    int numDeliveryThreads
						    )
  {
    TcpTransportService* t = new TcpTransportService(createOptionsMap(cryptoFlags, merror, queueSize, threshold, portoffset, numDeliveryThreads));
    // TODO: Figure out where to do unregisterInstance.  Currently only needed for simualtor, so not high priority.
    mace::ServiceFactory<TransportServiceClass>::registerInstance("TcpTransport", t);
    mace::ServiceFactory<BufferedTransportServiceClass>::registerInstance("TcpTransport", t);
    ServiceClass::addToServiceList(*t);
    return *t;
  }

  TransportServiceClass& new_TcpTransport_TransportEx(int numDeliveryThreads,
						    TransportCryptoServiceClass::type cryptoFlags,
						    bool merror,
						    uint32_t queueSize,
						    uint32_t threshold,
						    int portoffset
						    )
  {
    TcpTransportService* t = new TcpTransportService(createOptionsMap(cryptoFlags, merror, queueSize, threshold, portoffset, numDeliveryThreads));
    // TODO: Figure out where to do unregisterInstance.  Currently only needed for simualtor, so not high priority.
    mace::ServiceFactory<TransportServiceClass>::registerInstance("TcpTransport", t);
    mace::ServiceFactory<BufferedTransportServiceClass>::registerInstance("TcpTransport", t);
    ServiceClass::addToServiceList(*t);
    return *t;
  }


  BufferedTransportServiceClass& new_TcpTransport_BufferedTransport(TransportCryptoServiceClass::type cryptoFlags,
								    bool merror,
								    uint32_t queueSize,
								    uint32_t threshold,
								    int portoffset,
								    int numDeliveryThreads
								    )
  {
    TcpTransportService* t = new TcpTransportService(createOptionsMap(cryptoFlags, merror, queueSize, threshold, portoffset, numDeliveryThreads));
    mace::ServiceFactory<TransportServiceClass>::registerInstance("TcpTransport", t);
    mace::ServiceFactory<BufferedTransportServiceClass>::registerInstance("TcpTransport", t);
    ServiceClass::addToServiceList(*t);
    return *t;
  }

  TransportServiceClass& private_new_TcpTransport_Transport(TransportCryptoServiceClass::type cryptoFlags,
						    bool merror,
						    uint32_t queueSize,
						    uint32_t threshold,
						    int portoffset,
						    int numDeliveryThreads)
  {
    TcpTransportService* t = new TcpTransportService(createOptionsMap(cryptoFlags, merror, queueSize, threshold, portoffset, numDeliveryThreads));
    ServiceClass::addToServiceList(*t);
    return *t;
  }
  BufferedTransportServiceClass& private_new_TcpTransport_BufferedTransport(TransportCryptoServiceClass::type cryptoFlags,
								    bool merror,
								    uint32_t queueSize,
								    uint32_t threshold,
								    int portoffset,
								    int numDeliveryThreads)
  {
    TcpTransportService* t = new TcpTransportService(createOptionsMap(cryptoFlags, merror, queueSize, threshold, portoffset, numDeliveryThreads));
    ServiceClass::addToServiceList(*t);
    return *t;
  }

  TransportServiceClass& new_TcpTransport_Transport(const OptionsMap& m) {
    return *(new TcpTransportService(m));
  }

  TransportServiceClass& private_new_TcpTransport_Transport(const OptionsMap& m) {
    TcpTransportService* t = new TcpTransportService(m);
    ServiceClass::addToServiceList(*t);
    return *t;
  }
  
  BufferedTransportServiceClass& new_TcpTransport_BufferedTransport(const OptionsMap& m) {
    return *(new TcpTransportService(m));
  }

  TransportServiceClass& configure_new_TcpTransport_Transport(bool shared) {
    OptionsMap m;
    if (params::containsKey("MACE_TCP_UPCALL_MSG_ERROR")) { 
      bool merror = params::get("MACE_TCP_UPCALL_MSG_ERROR", false);
      m[TcpTransport_namespace::UPCALL_MESSAGE_ERROR] = merror; 
    } 
    if (shared) { 
      return new_TcpTransport_Transport(m); 
    } else { 
      return private_new_TcpTransport_Transport(); 
    } 
  } 
  
  BufferedTransportServiceClass& configure_new_TcpTransport_BufferedTransport(bool shared) {
      if (shared) {
          return new_TcpTransport_BufferedTransport();
      } else {
          return private_new_TcpTransport_BufferedTransport();
      }
  }

  void load_protocol() {
    if (macesim::SimulatorFlags::simulated()) {
        //std::cout << "Not loading TcpTransport protocol during simulation" << Log::endl;
        //FYI - Ken can do so here as well.
    } else {
        mace::ServiceFactory<TransportServiceClass>::registerService(&configure_new_TcpTransport_Transport, "TcpTransport");
        mace::ServiceFactory<BufferedTransportServiceClass>::registerService(&configure_new_TcpTransport_BufferedTransport, "TcpTransport");

        mace::ServiceConfig<TransportServiceClass>::registerService("TcpTransport", mace::makeStringSet("reliable,inorder,ipv4,TcpTransport",","));
        mace::ServiceConfig<BufferedTransportServiceClass>::registerService("TcpTransport", mace::makeStringSet("reliable,inorder,ipv4,TcpTransport",","));
    }
  }

} // namespace TcpTransportService

TcpTransportPtr TcpTransport::create(const TcpTransport_namespace::OptionsMap& o) {
  
  TransportCryptoServiceClass::type cryptoFlags = TransportCryptoServiceClass::NONE;
  bool upcallMessageErrors = false;
  bool nodelay = params::get<bool>( "SET_TCP_NODELAY", false );
  uint32_t queueSize = std::numeric_limits<uint32_t>::max();
  uint32_t threshold = std::numeric_limits<uint32_t>::max();
  int portoffset = std::numeric_limits<int32_t>::max();
  MaceAddr maddr = SockUtil::NULL_MACEADDR;
  int bl = SOMAXCONN;
  SockAddr forwarder;
  uint16_t forwarderPort = Util::getPort();
  SockAddr localHost;
  uint16_t localHostPort = Util::getPort();
  bool rejectRouteRts = false;
  uint32_t maxDeliver = 100;
  int numDeliveryThreads = params::get<int>( "NUM_TRANSPORT_THREADS", 8 );

  if (o.containsKey(TcpTransport_namespace::CRYPTO)) {
    cryptoFlags = o.get(TcpTransport_namespace::CRYPTO);
  }
  if (o.containsKey(TcpTransport_namespace::UPCALL_MESSAGE_ERROR)) {
    upcallMessageErrors = o.get(TcpTransport_namespace::UPCALL_MESSAGE_ERROR);
  }
  if (o.containsKey(TcpTransport_namespace::SET_NO_DELAY)) {
    nodelay = o.get(TcpTransport_namespace::SET_NO_DELAY);
  }
  if (o.containsKey(TcpTransport_namespace::QUEUE_SIZE)) {
    queueSize = o.get(TcpTransport_namespace::QUEUE_SIZE);
  }
  if (o.containsKey(TcpTransport_namespace::THRESHOLD)) {
    threshold = o.get(TcpTransport_namespace::THRESHOLD);
  }
  if (o.containsKey(TcpTransport_namespace::PORT_OFFSET)) {
    portoffset = o.get(TcpTransport_namespace::PORT_OFFSET);
  }
  if (o.containsKey(TcpTransport_namespace::NUM_DELIVERY_THREADS)) {
    numDeliveryThreads = o.get(TcpTransport_namespace::NUM_DELIVERY_THREADS);
  }
  if (o.containsKey(TcpTransport_namespace::BACKLOG)) {
    bl = o.get(TcpTransport_namespace::BACKLOG);
  }
  if (o.containsKey(TcpTransport_namespace::FORWARD_PORT)) {
    ASSERT(o.containsKey(TcpTransport_namespace::FORWARD_HOST));
    forwarderPort = o.get(TcpTransport_namespace::FORWARD_PORT);
  }
  if (o.containsKey(TcpTransport_namespace::FORWARD_HOST)) {
    int fw = o.get(TcpTransport_namespace::FORWARD_HOST);
    forwarder = SockAddr(fw, forwarderPort);
  }
  if (o.containsKey(TcpTransport_namespace::LOCAL_PORT)) {
    ASSERT(o.containsKey(TcpTransport_namespace::LOCAL_HOST));
    localHostPort = o.get(TcpTransport_namespace::LOCAL_PORT);
  }
  if (o.containsKey(TcpTransport_namespace::LOCAL_HOST)) {
    uint32_t lch = (uint32_t)o.get(TcpTransport_namespace::LOCAL_HOST);
    if (lch != INADDR_NONE) {
      localHost = SockAddr(lch, localHostPort);
    }
  }
  if (o.containsKey(TcpTransport_namespace::REJECT_ON_ROUTERTS)) {
    rejectRouteRts = o.get(TcpTransport_namespace::REJECT_ON_ROUTERTS);
  }
  if (o.containsKey(TcpTransport_namespace::MAX_CONSECUTIVE_DELIVER)) {
    maxDeliver = o.get(TcpTransport_namespace::MAX_CONSECUTIVE_DELIVER);
  }

  TcpTransportPtr p(new TcpTransport(cryptoFlags, upcallMessageErrors, nodelay,
				     queueSize, threshold,
				     portoffset, maddr, bl, forwarder,
				     localHost,
				     rejectRouteRts, maxDeliver, numDeliveryThreads));
  TransportScheduler::add(p);
  return p;
} // create

TcpTransport::TcpTransport(TransportCryptoServiceClass::type cryptoFlags,
			   bool merror,
			   bool nodelay,
			   uint32_t queueSize,
			   uint32_t threshold,
			   int portoffset,
			   MaceAddr maddr,
			   int bl,
			   SockAddr forwarder,
			   SockAddr localHost,
			   bool rejectRouteRts,
			   uint32_t maxDeliver,
			   int numDeliveryThreads) :
  BaseTransport(portoffset, maddr, bl, forwarder, localHost, numDeliveryThreads),
  upcallMessageError(merror),
  setNodelay(nodelay),
  cryptoFlags(cryptoFlags),
  maxQueueSize(queueSize),
  queueThreshold((threshold == std::numeric_limits<uint32_t>::max()) ?
		 ((maxQueueSize / 2) ? (maxQueueSize / 2) : 1) : threshold),
  rejectOnRouteRts(rejectRouteRts),
  MAX_CONSECUTIVE_DELIVER(maxDeliver),
  ctx(0), localPubkey(0) {

    ADD_SELECTORS("TcpTransport::constructor");
    macedbg(1) << "Creating TcpTransport!" << Log::endl;
  if (cryptoFlags & TransportCryptoServiceClass::TLS) {
    initSSL();
  }

//   pthread_cond_init(&dsignal, 0);
} // TcpTransport

TcpTransport::~TcpTransport() {
  shutdown();

  EVP_PKEY_free(localPubkey);
  SSL_CTX_free(ctx);
} // ~TcpTransport

void TcpTransport::initSSL() {
  ADD_SELECTORS("TcpTransport::initSSL");
  static bool loadSSL = true;
  if (loadSSL) {
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    ERR_load_BIO_strings();
    loadSSL = false;
  }

  ctx = SSL_CTX_new(TLSv1_method());
  if (!ctx) {
    ERR_print_errors_fp(stderr);
    Log::err() << "TcpTransport::initSSL " << Util::getSslErrorString()
	       << Log::endl;
    ABORT("TcpTransport::initSSL");
  }

  SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, 0);

  if (!params::containsKey(params::MACE_CERT_FILE)) {
    Log::err() << "parameter MACE_CERT_FILE not set" << Log::endl;
    exit(-1);
  }

  std::string certFile = params::get<std::string>(params::MACE_CERT_FILE);
//   if (SSL_CTX_use_certificate_chain_file(ctx, certFile.c_str(), SSL_FILETYPE_PEM) <= 0) {
  if (SSL_CTX_use_certificate_file(ctx, certFile.c_str(), SSL_FILETYPE_PEM) <= 0) {
    Log::err() << "error loading certificate " << certFile << Log::endl;
    ERR_print_errors_fp(stderr);
    Log::err() << "TcpTransport::initSSL " << Util::getSslErrorString() << Log::endl;
    exit(-1);
  }

  if (!params::containsKey(params::MACE_PRIVATE_KEY_FILE)) {
    Log::err() << "parameter MACE_PRIVATE_KEY_FILE not set" << Log::endl;
    exit(-1);
  }

  std::string keyFile = params::get<std::string>(params::MACE_PRIVATE_KEY_FILE);
  if (SSL_CTX_use_PrivateKey_file(ctx, keyFile.c_str(), SSL_FILETYPE_PEM) <= 0) {
    Log::err() << "error loading private key " << keyFile << Log::endl;
    ERR_print_errors_fp(stderr);
    Log::err() << "TcpTransport::initSSL " << Util::getSslErrorString() << Log::endl;
    exit(-1);
  }

  if (!SSL_CTX_check_private_key(ctx)) {
    Log::err() << "private key " << keyFile
	       << " does not match the public certificate " << certFile << Log::endl;
    exit(-1);
  }

  if (!params::containsKey(params::MACE_CA_FILE)) {
    Log::err() << "parameter MACE_CA_FILE not set" << Log::endl;
    exit(-1);
  }

  std::string caFile = params::get<std::string>(params::MACE_CA_FILE);
  STACK_OF(X509_NAME) *certNames = SSL_load_client_CA_file(caFile.c_str());
  if (certNames != NULL) {
    SSL_CTX_set_client_CA_list(ctx, certNames);
  }
  else {
    Log::err() << "could not load client CA file " << caFile << Log::endl;
    exit(-1);
  }

  if (!SSL_CTX_load_verify_locations(ctx, caFile.c_str(), 0)) {
    Log::err() << "could not load and verify CA file " << caFile << Log::endl;
    exit(-1);
  }

//   maceout << "loaded and verified keys" << Log::endl;

  ASSERT(SSL_CTX_set_cipher_list(ctx, "AES128-SHA"));

  std::string certbuf;
  try {
    certbuf = FileUtil::loadFile(certFile);
  }
  catch (const Exception& e) {
    Log::err() << "could not load " << certFile << ": " << e << Log::endl;
    exit(-1);
  }

  BIO *memb = BIO_new_mem_buf((void*)certbuf.c_str(), certbuf.size());
  X509* x509 = PEM_read_bio_X509(memb, 0, 0, 0);
  if (x509 == NULL) {
    Log::err() << "could not load X509" << Log::endl;
    exit(-1);
  }

  uint32_t len = 512;
  char peerCNbuf[len];
  if (X509_NAME_get_text_by_NID(X509_get_subject_name(x509),
				NID_commonName, peerCNbuf, len) == -1) {
    Log::err() << "common name not found" << Log::endl;
    exit(-1);
  }
  localCommonName = StrUtil::toLower(peerCNbuf);
//   std::cerr << "read common name=" << localCommonName << std::endl;

  EVP_PKEY* tmpkey = X509_get_pubkey(x509);
  if (tmpkey == NULL) {
    Log::err() << "could not read public key" << Log::endl;
    exit(-1);
  }

  RSA* rsakey = RSAPublicKey_dup(EVP_PKEY_get1_RSA(tmpkey));

  localPubkey = EVP_PKEY_new();
  EVP_PKEY_assign_RSA(localPubkey, rsakey);
  
  X509_free(x509);
  BIO_free(memb);
  
} // initSSL

void TcpTransport::notifyError(TcpConnectionPtr c, NetworkHandlerMap& handlers) {
  ADD_SELECTORS("TcpTransport::notifyError");
  const NodeSet& ns = c->remoteKeys();
  TransportError::type err = c->getError();
  const std::string& m = c->getErrorString();

  try {
    //   traceout << TRANSPORT_TRACE_ERROR << ns << err << m;
    for (NetworkHandlerMap::iterator i = handlers.begin();
        i != handlers.end(); i++) {
      for (NodeSet::const_iterator n = ns.begin(); n != ns.end(); n++) {
        i->second->error(*n, err, m, i->first);
      }
    }

    if (upcallMessageError && (c->directionType() == TransportServiceClass::Connection::OUTGOING)) {
      traceout << true;
      c->notifyMessageErrors(pipeline, handlers);
    }
    else {
      traceout << false;
    }
    traceout << Log::end;
  }
  catch(const ExitedException& e) {
    Log::warn() << "TcpTransport error caught exception for exited service: " << e << Log::endl;
  }
} // notifyError

void TcpTransport::clearToSend(const MaceKey& dest, registration_uid_t rid, ConnectionStatusHandler* h) {
  ADD_SELECTORS("TcpTransport::clearToSend");
  if (h == NULL) {
    Log::err() << "TcpTransport::clearToSend: no handler registered with "
	       << rid << Log::endl;
    return;
  }

//   traceout << TRANSPORT_TRACE_CTS << dest << rid << Log::end;
  h->clearToSend(dest, rid);
} // clearToSend

void TcpTransport::notifyFlushed(registration_uid_t rid, ConnectionStatusHandler* h) {
  ADD_SELECTORS("TcpTransport::notifyFlushed");
  if (h == NULL) {
    Log::err() << "TcpTransport::notifyFlushed: no handler registered with "
	       << rid << Log::endl;
    return;
  }

//   traceout << TRANSPORT_TRACE_FLUSH << rid << Log::endl;
  h->notifyFlushed(rid);
} // notifyFlushed

void TcpTransport::flush() {
  while (hasOutgoingBufferedData()) {
    SysUtil::sleepm(500);
  }
} // flush

void TcpTransport::closeConnections() {
  for (ConnectionMap::const_iterator i = connections.begin();
       i != connections.end(); i++) {
    TcpConnectionPtr c = i->second;
    c->close();
  }

  connections.clear();
  in.clear();
  out.clear();
} // closeConnections

bool TcpTransport::runDeliverCondition(ThreadPoolType* tp, uint threadId) {
  ADD_SELECTORS("TcpTransport::runDeliverCondition");

  //   macedbg(1) << "Called on threadId " << threadId << Log::endl;

  if (deliverState == POST_FINITO) { 
    //     macedbg(1) << "Returning false because all delivery is done and flush() was called." << Log::endl;
    return false;
  }

  if (deliverState != WAITING) { 
    macedbg(1) << "Returning true because deliverState= " << deliverState << " != WAITING (" << WAITING << ")" << Log::endl;
    return true; 
  }
  
  unregisterHandlers();
  for (ConnectionVector::const_iterator i = resend.begin();
      i != resend.end(); i++) {
    sendable.push(*i);
  }
  resend.clear();

  if (!(deliverable.empty() && sendable.empty() && errors.empty() &&
      deliverthr.empty() && pendingFlushedNotifications.empty())) {
    //     macedbg(1) << "Returning true - something's not empty" << Log::endl;
    return true;
  }
  
  if (shuttingDown) {
    deliverState = FINITO;
    //     macedbg(1) << "Set state to FINITO and returning true.  Time to close up shop!" << Log::endl;
    return true;
  }

  //   macedbg(1) << "Nothing to do!" << Log::endl;
  return false;
}

void TcpTransport::runDeliverSetup(ThreadPoolType* tp, uint threadId) {
  ADD_SELECTORS("TcpTransport::runDeliverSetup");

  //   macedbg(1) << "Entering runDeliverSetup( " << threadId << " ) - deliverState: " << deliverState << Log::endl;

  DeliveryData& data = tp->data(threadId);

  switch(deliverState) {
    case WAITING:
    //Check Message Delivery
    //       macedbg(1) << "Case WAITING." << Log::endl;
      if (!deliverable.empty() || !deliverthr.empty()) {
        if (!deliverable.empty()) {
          deliver_c = deliverable.front();
          deliverable.pop();
        }
        else {
          deliver_c = deliverthr.front();
          deliverthr.pop();
        }

        //         macedbg(1) << "Checking Message Delivery - dereferencing connection" << Log::endl;
        if (!deliver_c->rqempty() && !deliver_c->acceptedConnection()) {
          if (!acceptConnection(deliver_c->getTokenMaceAddr(), deliver_c->getToken())) {
            deliver_c->close(TransportError::NOT_ACCEPTED, "connection rejected", true);
          }
          else {
            deliver_c->acceptConnection();
          }
        }

        deliver_dcount = 0;
      }
    case DELIVER:
      //       macedbg(1) << "Case DELIVER." << Log::endl;
      if (deliver_c) {
        //XXX: Is it possible to get here by accident?  I.e. Not either following pulling a ptr from WAITING or frmo being in the DELIVER state?
        if (deliver_c->isDeliverable() && 
            (MAX_CONSECUTIVE_DELIVER == 0 || deliver_dcount < MAX_CONSECUTIVE_DELIVER)) {
          if (shuttingDown && dataHandlers.empty()) {
            // if (!macedbg(1).isNoop()) {
            //   macedbg(1) << "draining messages from " << deliver_c->id() << " because shuttingDown and dataHandlers.empty()" << Log::endl;
            // }
            while (deliver_c->isDeliverable()) {
              deliver_c->dequeue(data.shdr, data.s);
            }
          } else {
            // if (!macedbg(1).isNoop()) {
            //   macedbg(1) << "reading message from " << deliver_c->id() << Log::endl;
            // }
            deliver_c->dequeue(data.shdr, data.s);
            if (deliver_c->remoteKeys().empty() || proxying) {
              //               macedbg(1) << "Setting doAddRemoteKey to true." << Log::endl;
              data.doAddRemoteKey = true; 
              data.ptr = (void*) new TcpConnectionPtr(deliver_c);
            } else {
              data.doAddRemoteKey = false; 
            }
            // Get ticket position.
            deliver_dcount++;
            deliverState = DELIVER;
            data.deliverState = DELIVER;
            deliverDataSetup(tp, data);
            return;
          }
        } else {
          if (deliver_c->isDeliverable()) {
            deliverthr.push(deliver_c);
          }
        }
      }
    case RTS:
      //XXX: Concern - what if it gets pushed back onto the sendable queue
      //before this finishes?  In the old model, a single deliver thread
      //prevented that occurence.
      //
      //Need a test to really test RTS
      //
      //Okay - so I'm going to just pull one item from rts, then on finish, check if I should push onto resend.
      //       macedbg(1) << "Case RTS." << Log::endl;
      while (!sendable.empty()) {
        deliver_c = sendable.front();
        sendable.pop();
        TcpConnection::StatusCallbackArgsQueue& rts = deliver_c->getRTS();
        //doing these in parallel for one socket could be concerning.  Wasted waking.
        if (deliver_c->isOpen() && !rts.empty()) {
          const TcpConnection::StatusCallbackArgs& p = rts.front();
          deliverState = ERROR;
          data.deliverState = RTS;
          data.remoteKey = p.dest;
          data.hdr.rid = p.rid;
          rts.pop();
          //Get Ticket position.
          
          ThreadStructure::newMessageTicket();
          ConnectionHandlerMap::iterator i = connectionHandlers.find(data.hdr.rid);
          if (i == connectionHandlers.end()) { data.connectionStatusHandler = NULL; }
          else { data.connectionStatusHandler = i->second; }

          if (!rts.empty()) {
            resend.push_back(deliver_c);
          }
          return;
        }
      }
    case ERROR:
      //       macedbg(1) << "Case ERROR" << Log::endl;
      deliver_c = TcpConnectionPtr();
      if (!errors.empty() && errorHandlers.empty()) {
        errors.clear(); // Skip notifying these errors with no handlers.
      }
      if (!errors.empty()) {
        data.ptr = (void*) new TcpConnectionPtr(errors.front());
        errors.pop();
        deliverState = FLUSHED;
        data.deliverState = ERROR;
        data.errorHandlers = errorHandlers;
        ThreadStructure::newMessageTicket();
        return;
      }
    case FLUSHED:
      //       macedbg(1) << "Case FLUSHED" << Log::endl;
      deliverState = WAITING;
      if (!pendingFlushedNotifications.empty()) {
        data.deliverState = FLUSHED;
        data.hdr.rid = pendingFlushedNotifications.front();
        pendingFlushedNotifications.pop();
        ConnectionHandlerMap::iterator i = connectionHandlers.find(data.hdr.rid);
        if (i == connectionHandlers.end()) {
          data.connectionStatusHandler = NULL;
        } else {
          data.connectionStatusHandler = i->second;
        }
        ThreadStructure::newMessageTicket();
        return;
      }
      data.deliverState = WAITING;
      break;
    case FINITO:
      //       macedbg(1) << "case FINITO" << Log::endl;
      data.deliverState = FINITO;
      deliverState = POST_FINITO;
      break;
    case POST_FINITO: 
      ABORT("HUH?");
      break;
    default:
      maceerr << "Unrecognized deliverState: " << deliverState << " restoring to WAITING" << Log::endl;
      deliverState = WAITING;
  }

}

void TcpTransport::runDeliverProcessUnlocked(ThreadPoolType* tp, uint threadId) {
  ADD_SELECTORS("TcpTransport::runDeliverProcessUnlocked");

//  macedbg(1) << "Entering runDeliverProcessUnlocked( " << threadId << " )" << Log::endl;

  DeliveryData& data = tp->data(threadId);

  switch(data.deliverState) {
    case WAITING: 
    {
//      macedbg(1) << "Event processing, but no work to do (in waiting state)" << Log::endl;
      break;
    }
    case DELIVER:
      deliverData(data);
      break;
    case RTS:
      clearToSend(data.remoteKey, data.hdr.rid, data.connectionStatusHandler);
      break;
    case ERROR: 
    {
//      macedbg(1) << "Error in delivery!" << Log::endl;
      TcpConnectionPtr ptr = *(TcpConnectionPtr*)data.ptr;
      notifyError(ptr, data.errorHandlers);
      delete (TcpConnectionPtr*)data.ptr;
    }
      break;
    case FLUSHED:
      notifyFlushed(data.hdr.rid, data.connectionStatusHandler);
      break;
    case FINITO:
//       macedbg(1) << "Calling flush()!" << Log::endl;
      flush();
//       macedbg(1) << "Done Calling flush()!" << Log::endl;
      tp->halt();
      break;
    case POST_FINITO:
    default:
      ABORT("Huh?");
      break;
  }
}

void TcpTransport::runDeliverFinish(ThreadPoolType* tp, uint threadId) {
  ADD_SELECTORS("TcpTransport::runDeliverFinish");

  DeliveryData& data = tp->data(threadId);
  //   macedbg(1) << "runDeliverFinish( " << threadId << " ) -- deliverState: " << deliverState << " -- data.deliverState: " << data.deliverState << Log::endl;

  if (data.deliverState == DELIVER) {
    //     macedbg(1) << "doAddRemoteKey: " << data.doAddRemoteKey << Log::endl;
    if (data.doAddRemoteKey) {
      (*(TcpConnectionPtr*)data.ptr)->addRemoteKey(data.remoteKey);
      delete (TcpConnectionPtr*)data.ptr;
      data.doAddRemoteKey = false;
    }
  }
  data.deliverState = WAITING;
}

// void TcpTransport::runDeliverThread() {
//   ADD_SELECTORS("TcpTransport::runDeliverThread");
//   ABORT("UNUSED");
//   typedef std::vector<TcpConnectionPtr> ConnectionVector;
//   MaceKey esrc;
//   ConnectionVector resend;
//   // WARN: Is it hypothetically possible that an error can occur, but new
//   // message arrives before error is signalled?  My glance at this code says
//   // yes!
//   //
//   // To properly mimic this code - use a switch statement with phases to keep
//   // threads coordinated running through this loop.  This is to ensure fairness
//   // - old model made sure to do everything in order.  Alternative approach
//   // would be to designate purposes for threads, but that is limiting in other
//   // ways.
//   while (!shuttingDown || !deliverable.empty() || !errors.empty() ||
// 	 !deliverthr.empty() || !pendingFlushedNotifications.empty()) {
//     // Put this block in the condition check?  Or setup, or finish?  I'll start
//     // with condition check.  Since currently it happens on every signal before
//     // re-determining if there is more work.
//     // Skip this block unless the phase is "waiting"
//     unregisterHandlers();
//     for (ConnectionVector::const_iterator i = resend.begin();
// 	 i != resend.end(); i++) {
//       sendable.push(*i);
//     }
//     resend.clear();
// 
//     if (deliverable.empty() && sendable.empty() && errors.empty() &&
// 	deliverthr.empty() && pendingFlushedNotifications.empty()) {
// //       macedbg(1) << "waiting for deliver signal" << Log::endl;
//       waitForDeliverSignal();
// //       macedbg(1) << "received deliver signal deliverable.size=" << deliverable.size()
// // 		 << " sendable.size=" << sendable.size()
// // 		 << " errors.size=" << errors.size()
// // 		 << " shuttingDown=" << shuttingDown
// // 		 << Log::endl;
//       continue;
//     }
// //     cerr << "deliver thread sendable.size=" << sendable.size() << endl;
// 
//     if (!deliverable.empty() || !deliverthr.empty()) {
//       bool usethr = false;
//       TcpConnectionPtr c;
//       if (!deliverable.empty()) {
// 	c = deliverable.front();
//       }
//       else {
// 	c = deliverthr.front();
// 	usethr = true;
//       }
// 
//       if (!c->rqempty() && !c->acceptedConnection()) {
// 	if (!acceptConnection(c->getTokenMaceAddr(), c->getToken())) {
// 	  c->close(TransportError::NOT_ACCEPTED, "connection rejected", true);
// 	}
// 	else {
// 	  c->acceptConnection();
// 	}
//       }
// 
//       uint32_t dcount = 0;
//       while (c->isDeliverable() &&
// 	     (MAX_CONSECUTIVE_DELIVER == 0 ||
// 	      dcount < MAX_CONSECUTIVE_DELIVER)) {
// 	if (!macedbg(1).isNoop()) {
// 	  macedbg(1) << "reading message from " << c->id() << Log::endl;
// 	}
// 	std::string shdr;
// 	std::string sbuf;
// 	c->dequeue(shdr, sbuf);
// 
//         //maceout << "{{ " << Log::endl;
//         //uint64_t start_time = TimeUtil::timeu();
//         if (c->remoteKeys().empty() || proxying) {
//           deliverSetMessage(shdr, sbuf, this, &BaseTransport::deliverData, &esrc );
//           //deliverData(shdr, sbuf, &esrc);
//           //XXX: Looks like error handling must be broken?  esrc is going to be passed inappropriately into deliverSetMessage, which will not set it until later...
// 	  c->addRemoteKey(esrc); // do the addRemoteKey in finish...  Then, store src in threaded data?
// 	}
// 	else {
//           deliverSetMessage(shdr, sbuf, this, &BaseTransport::deliverData);
// 	  //deliverData(shdr, sbuf);
// 	}
//         //uint64_t delay = TimeUtil::timeu() - start_time;
//         //maceout << "}} Delay : " << delay << Log::endl;
// 	dcount++;
//       }
//       if (usethr) {
// 	deliverthr.pop();
//       }
//       else {
// 	deliverable.pop();
//       }
//       if (c->isDeliverable()) {
// 	deliverthr.push(c);
//       }
// 
//       if (MAX_CONSECUTIVE_DELIVER && dcount == MAX_CONSECUTIVE_DELIVER) {
//         //Consider what happens here.   I think this just goes away...
// 	ThreadUtil::yield();
//       }
// //       if (c->isSuspended() && !c->rqempty()) {
// // 	suspended.push_back(c);
// //       }
//     }
//     if (!sendable.empty()) {
//       TcpConnectionPtr c = sendable.front();
//       uint64_t start = TimeUtil::timeu();
// 
//       TcpConnection::StatusCallbackArgsQueue& rts = c->getRTS();
//       //doing these in parallel for one socket could be concerning.  Wasted waking.
//       while (c->isOpen() && !rts.empty()) {
// 	const TcpConnection::StatusCallbackArgs& p = rts.front();
// 	if ((p.ts < start) && c->canSend()) {
// 	  clearToSend(p.dest, p.rid);
// 	  rts.pop();
// 	}
// 	else {
// 	  if ((p.ts >= start) && c->canSend()) {
// 	    resend.push_back(c);
// 	  }
// 	  break;
// 	}
//       }
// 
//       sendable.pop();
//     }
//     if (!errors.empty()) {
//       TcpConnectionPtr c = errors.front();
//       notifyError(c);
//       errors.pop();
//     }
//     if (!pendingFlushedNotifications.empty()) {
//       notifyFlushed(pendingFlushedNotifications.front());
//       pendingFlushedNotifications.pop();
//     }
//   }
//   flush();
// //   Log::log("TcpTransport::runDeliverThread")
// //     << "exiting connections=" << connections.size() << " in=" << in.size()
// //     << " out=" << out.size() << Log::endl;
// } // runDeliverThread

void TcpTransport::doIO(CONST_ISSET fd_set& rset, CONST_ISSET fd_set& wset, uint64_t selectTime) {
  ADD_SELECTORS("TcpTransport::doIO");
  
//   static uint count = 0;
  ScopedTimer st(doIOTime, TcpConnection::USE_DEBUGGING_TIMERS);
  ScopedLock sl(tlock);
//   cerr << "connections.size=" << connections.size() << endl;
  if (!running) {
    return;
  }

  if (!deferredSendData.empty()) {
    macedbg(1) << "There is deferredSendData, try to send it out!" << Log::endl;
    SendDataList cp = deferredSendData;
    deferredSendData.clear();
    for (SendDataList::const_iterator i = cp.begin(); i != cp.end(); i++) {
      const SendDataStruct& s = *i;
      sendData(s.src, s.dest, s.nextHop, s.rid, s.ph, s.s,
	       s.checkQueueSize, s.rts);
    }
  }
  if (!deferredRTS.empty()) {
    RTSList cp = deferredRTS;
    deferredRTS.clear();
    for (RTSList::const_iterator i = cp.begin(); i != cp.end(); i++) {
      const RequestToSendStruct& s = *i;
      requestToSend(s.dest, s.rid);
    }
  }
  if (!deferredSetQueueSize.empty()) {
    SetQueueSizeList cp = deferredSetQueueSize;
    deferredSetQueueSize.clear();
    for (SetQueueSizeList::const_iterator i = cp.begin(); i != cp.end(); i++) {
      const SetQueueSizeStruct& s = *i;
      setQueueSize(s.peer, s.size, s.threshold);
    }
  }
  if (!deferredSetWindowSize.empty()) {
    SetWindowSizeList cp = deferredSetWindowSize;
    deferredSetWindowSize.clear();
    for (SetWindowSizeList::const_iterator i = cp.begin(); i != cp.end(); i++) {
      const SetWindowSizeStruct& s = *i;
      setWindowSize(s.peer, s.microsec);
    }
  }


  if (FD_ISSET(transportSocket, &rset)) {
    macedbg(1) << "New incomming socket to be accept!" << Log::endl;
    accept();
  }

  for (ConnectionMap::const_iterator i = connections.begin();
       i != connections.end(); i++) {
    socket_t s = i->first;
    TcpConnectionPtr c = i->second;

    if (selectTime < c->timeOpened()) {
      continue;
    }

    if (!macedbg(2).isNoop()) {
      bool r = FD_ISSET(s, &rset);
      bool w = FD_ISSET(s, &wset);
      macedbg(2) << s << " isOpen=" << c->isOpen() << " r=" << r << " w=" << w << " isConnecting=" << c->isConnecting() << Log::endl;
    }

    if (!c->isOpen() && !c->acceptedConnection()) {
      closeBidirectionalConnection(c);
      continue;
    }

    if (c->isOpen() && FD_ISSET(s, &rset)) {
      //macedbg(1) << "There is incoming data!" << Log::endl;
      bool connecting = c->isConnecting();
      try {
        do {
          //macedbg(1) << "Try to read data!" << Log::endl;
          c->read();
        } while (cryptoFlags & TransportCryptoServiceClass::TLS
                 && SSL_pending(c->getSSL()));
      }
//       try {
// 	c->read();
//       }
      catch (const std::length_error& e) {
	       maceerr << e.what() << Log::endl;
	       c->close();
      }
      catch (const std::bad_alloc& e) {
	       maceerr << e.what() << Log::endl;
	       c->close();
      }
      catch (const Exception& e) {
	       maceerr << e << Log::endl;
	       c->close();
      }
      if (connecting && c->isOpen() && !c->isConnecting()) {
	       if (c->directionType() == TransportServiceClass::Connection::INCOMING) {
	         DestinationMap::iterator di = in.find(c->id());
	         if (di != in.end()) {
	           TcpConnectionPtr old = di->second;
	           if (!macedbg(1).isNoop()) {
	             macedbg(1) << "closing old connection with id=" << old->id() << " on " << old->sockfd() << Log::endl;
	           }
	           old->close(TransportError::READ_ERROR, "peer reset");
	           closeBidirectionalConnection(old);
	           if (!macedbg(1).isNoop()) {
	             macedbg(1) << "completed connection to " << c->id() << " " << c->sockfd() << " in[" << c->id() << "]=" << in[c->id()]->sockfd() << Log::endl;
	           }
	         }
	       } else if (c->directionType() == TransportServiceClass::Connection::OUTGOING) {
	         DestinationMap::iterator di = out.find(c->id());
	         if (di != out.end()) {
	           TcpConnectionPtr old = di->second;
	           old->close(TransportError::READ_ERROR, "peer reset");
	           closeBidirectionalConnection(old);
	         }
	       }

// 	ASSERT(!in.containsKey(c->id()));
	       deferredIncomingConnections.push_back(c);

	       if (!macedbg(1).isNoop()) {
	         macedbg(1) << "marking " << c->id() << "=" << c->sockfd() << " as deferredIncomingConnection" << Log::endl;
	       }
      }
    }

    if (c->isOpen() && FD_ISSET(s, &wset)) {
      // macedbg(1) << "There is outgoing data!" << Log::endl;
      c->write();
      if (c->isOpen() && c->hasRTS() && c->canSend()) {
	       sendable.push(c);
	       signalDeliver();
      }
    }

    if (!c->rqempty()) {
      if (c->id().isUnroutable() && !out.containsKey(c->id())) {
	       deferredDeliver.insert(c->id());
      }else {
// 	if (!macedbg(1).isNoop()) {
// 	  macedbg(1) << "adding " << c->id() << " to deliverable" << Log::endl;
// 	}
	       if (!c->isDeliverable()) {
	         c->markDeliverable();
	       }
	       deliverable.push(c);
	       signalDeliver();
      }
    }

    if (!c->isOpen()) {
// 	log << "connection " << s << " closed "
// 	    << Util::getAddrString(c->id()) << Log::endl;
      closeBidirectionalConnection(c);
    }
  }

  removeClosedSockets();

  bool checkDeferredDeliver = false;
  for (ConnectionQueue::const_iterator i = deferredIncomingConnections.begin();
       i != deferredIncomingConnections.end(); i++) {
    TcpConnectionPtr c = *i;
    if (c->directionType() == TransportServiceClass::Connection::OUTGOING) {
      ASSERT(!out.containsKey(c->id()));

//       macedbg(1) << "setting c=" << c->sockfd() << " " << c->id()
// 		 << " as outgoing connection" << Log::endl;

      out[c->id()] = c;
      checkDeferredDeliver = true;
    } else {
      ASSERT(c->directionType() == TransportServiceClass::Connection::INCOMING ||
	     c->directionType() == TransportServiceClass::Connection::NAT);
      in[c->id()] = c;

//       macedbg(1) << "setting c=" << c->sockfd() << " " << c->id()
// 		 << " as incoming connection" << Log::endl;

    }
  }
  deferredIncomingConnections.clear();

  if (checkDeferredDeliver) {
    for (ConnectionMap::const_iterator i = connections.begin();
	       i != connections.end(); i++) {
      TcpConnectionPtr c = i->second;
      if (!c->rqempty() &&
	         deferredDeliver.contains(c->id()) &&
	         out.containsKey(c->id())) {
	       c->markDeliverable();
	       deliverable.push(c);
	       signalDeliver();
	       deferredDeliver.erase(c->id());
      }
    }
  }

//   count++;
} // doIO

void TcpTransport::garbageCollectSockets() {
  uint64_t now = TimeUtil::timeu();

  typedef std::vector<uint64_t> TimeVector;
  TimeVector idles;

  for (ConnectionMap::const_iterator i = connections.begin();
       i != connections.end(); i++) {
    TcpConnectionPtr c = i->second;

    idles.push_back(now - c->lastActivityTime());
  }

  std::stable_sort(idles.begin(), idles.end());
  uint64_t median = idles[idles.size() / 2];

  for (ConnectionMap::const_iterator i = connections.begin();
       i != connections.end(); i++) {
//     int s = i->first;
    TcpConnectionPtr c = i->second;

    if (!c->isOpen()) {
      closeBidirectionalConnection(c);
    }
    else if (c->idle() && ((now - c->lastActivityTime()) >= median)) {
      c->close(TransportError::NON_ERROR, "connection idle", true);
      closeBidirectionalConnection(c);
    }
  }

  removeClosedSockets();
} // garbageCollectSockets

void TcpTransport::closeBidirectionalConnection(TcpConnectionPtr c) {
  ADD_SELECTORS("TcpTransport::closeBidirectionalConnection");
  closed.insert(c->sockfd());
//   maceout << "added " << c->sockfd() << " " << c->id() << " to closed" << Log::endl;
//   maceout << "in.contains=" << in.containsKey(c->id())
// 	  << " out.contains=" << out.containsKey(c->id()) << Log::endl;
  if (in.containsKey(c->id()) && out.containsKey(c->id())) {
// 	  log << "closing both directions of " << s << Log::endl;
// 	ASSERT(connections.containsKey(out.get(c->id())));
    TcpConnectionPtr tmp = out.get(c->id());
    if (tmp == c) {
// 	  ASSERT(connections.containsKey(in.get(c->id())));
      tmp = in.get(c->id());
      ASSERT(tmp != c);
    }
// 	cerr << "transport closing connection with error " << c->getErrorString() << endl;
//     maceout << "closing " << tmp->sockfd() << " (" << c->sockfd() << ")"
// 	    << " with " << c->getError() << " " << c->getErrorString()
// 	    << Log::endl;
    tmp->close(TransportError::NON_ERROR, c->getErrorString(), true);
    closed.insert(tmp->sockfd());
  }
}

void TcpTransport::removeClosedSockets() {
  for (SocketSet::const_iterator i = closed.begin(); i != closed.end(); i++) {
    int s = *i;
    TcpConnectionPtr c = connections[s];
    
    if (c->getError() != TransportError::NON_ERROR &&
	c->getError() != TransportError::NOT_ACCEPTED) {
      errors.push(c);
//       const NodeSet& es = c->remoteKeys();
//       errors.push(ErrorNotification(es, c->getError(), c->getErrorString()));
    }

    in.erase(c->id());
    out.erase(c->id());
    translations.erase(c->id());
    connections.erase(s);
  }

  if (!closed.empty()) {
    signalDeliver();
    closed.clear();
  }
} // removeClosedSockets

void TcpTransport::close(const MaceKey& destk) {
  ScopedLock sl(tlock);
  const SockAddr& dest = getNextHop(destk.getMaceAddr());
  DestinationMap::iterator i = out.find(dest);
  if (i != out.end()) {
    i->second->close();
  }
  i = in.find(dest);
  if (i != in.end()) {
    i->second->close();
  }
} // close

void TcpTransport::accept() {
  ADD_SELECTORS("TcpTransport::accept");

  do {
    sockaddr_in sin;
    socklen_t sinlen = sizeof(sin);
    memset(&sin, 0, sinlen); 

    int s = ::accept(transportSocket, (struct sockaddr*)&sin, &sinlen);
    if (s < 0) {
      int err = SockUtil::getErrno();
      if (SockUtil::errorWouldBlock(err) ||
          SockUtil::errorConnFail(err)) {
	return;
      }
      else if (SockUtil::errorTransient(err)) {
	maceout << "error on accept(): " << Util::getErrorString(err)
		<< Log::endl;
	garbageCollectSockets();
	s = ::accept(transportSocket, (struct sockaddr*)&sin, &sinlen);
	if (s < 0) {
	  if (SockUtil::errorWouldBlock()) {
	    return;
	  }
	  err = SockUtil::getErrno();
	  if (SockUtil::errorTransient(err)) {
	    TransportScheduler::freeSockets();
	    return;
	  }
	  Log::perror("socket");
	  ABORT("socket");
	}
      }
      else {
	Log::perror("accept");
	ABORT("accept");
      }
    }
    ASSERT(s < FD_SETSIZE);

#ifdef USE_TCP_KEEPALIVE
    SockUtil::setKeepalive(s);
#endif
  
    SockUtil::setNonblock(s);
    TcpConnectionPtr c(new TcpConnection(s, TransportServiceClass::Connection::INCOMING, 
					 cryptoFlags, ctx, mace::string(),
					 maxQueueSize, queueThreshold));
    connections[s] = c;

    maceout << "Accepted connection from " << inet_ntoa(sin.sin_addr) << ":"
	    << ntohs(sin.sin_port) << " on " << s
	    << ", " << connections.size() << " connection(s)"
	    << Log::endl;
    
  } while (1);
} // handleAccept

bool TcpTransport::sendData(const MaceAddr& src, const MaceKey& dest,
			    const MaceAddr& nextHop, registration_uid_t rid,
			    const std::string& ph, const std::string& s,
			    bool checkQueueSize, bool rts) {
  ADD_SELECTORS("TcpTransport::sendData");
  static const bool STRIP_PROXY = params::get("MACE_TRANSPORT_STRIP_PROXY", false);
  ScopedTimer st(sendDataTime, TcpConnection::USE_DEBUGGING_TIMERS);

//   if (!macedbg(1).isNoop()) {
//     macedbg(1) << "STARTING(TcpTransport::sendData)" << Log::endl;
//   }
  static Accumulator* sendaccum = Accumulator::Instance(Accumulator::TRANSPORT_SEND);

  const SockAddr& msa = getNextHop(nextHop);

  lock();
  TcpConnectionPtr c;
  c = connect(dest, msa);
  if (c == 0) {
    deferredSendData.push_back(SendDataStruct(src, dest, nextHop, rid,
					      ph, s, checkQueueSize, rts));
    macedbg(1) << "Adding data to deferedSendData" << Log::endl;
  }
  if (!c->isOpen()) {
    unlock();
    return true;
  }

  SourceTranslationMap::const_iterator i;
  if (!disableTranslations) {
    i = translations.find(msa);
  }
  
  bool doEnqueue = !checkQueueSize || canSend(msa);

  if (!doEnqueue && rts) {
    c->addRTS(dest, rid);
  }
  unlock();

  uint32_t sz = ph.size() + s.size();

  if (doEnqueue || !rejectOnRouteRts) {
    const MaceAddr* sp = &src;
    if (!disableTranslations && i != translations.end()) {
      sp = &(i->second);
    }
    const MaceAddr* dp = &(dest.getMaceAddr());
    MaceAddr stripped;
    if (STRIP_PROXY) {
      stripped = MaceAddr(dest.getMaceAddr().local, SockUtil::NULL_MSOCKADDR);
      dp = &stripped;
    }
    /*  set flag if this is an internal message */
    uint8_t flag = 0;
    if( dest.addressFamily() == mace::CONTEXTNODE ){
      flag |= TransportHeader::INTERNALMSG;
    }
    TransportHeader th(*sp, *dp, rid, flag, sz);
    if (!macedbg(1).isNoop()) {
      macedbg(1) << "Sending TCP pkt from src=" << *sp << " to dest=" << dest << " sockaddr=" << msa << " id=" << c->id() << " sock=" 
        << c->sockfd() << " sz=" << sz << Log::endl;
    }
    lockc();
    c->enqueue(th, ph, s);
    unlockc();
    sendaccum->accumulate(sz);
  }

//   if (!macedbg(1).isNoop()) {
//     macedbg(1) << "ENDING(TcpTransport::sendData)" << Log::endl;
//   }
  return doEnqueue;
} // sendData

TcpConnectionPtr TcpTransport::connect(const SockAddr& dest,
				       bool& newConnection) {
  ADD_SELECTORS("TcpTransport::connect");

  newConnection = true;

  DestinationMap::const_iterator i = out.find(dest);
//   if (i == out.end()) {
//     macedbg(1) << "could not find outgoing connection for " << dest << Log::endl;
//   }

  if (i != out.end()) {
    TcpConnectionPtr c = i->second;
    if (c->isOpen()) {
      newConnection = false;
      return c;
    }
    else {
      removeClosedSockets();
    }
  }
  else if (dest.isUnroutable()) {
    TcpConnectionPtr c(new TcpConnection(0, TransportServiceClass::Connection::OUTGOING,
					 cryptoFlags, ctx, mace::string(),
					 0, 0, dest));
    std::ostringstream os;
    os << "no route to host: " << dest;
    c->close(TransportError::CONNECT_ERROR, os.str());
    errors.push(c);
    return c;
  }

  int s = socket();
  if (s < 0) {
    return TcpConnectionPtr();
  }
  int nats = -1;
  if (localAddr.isUnroutable()) {
    nats = socket();
    if (nats < 0) {
      ::close(s);
      return TcpConnectionPtr();
    }
  }

  struct sockaddr_in sa;
  SockUtil::fillSockAddr(dest.addr, dest.port + portOffset, sa);
  if (!macedbg(1).isNoop()) {
    macedbg(1) << "connecting to " << Util::getHostname(dest.addr) << ":"
	       << dest.port + portOffset << " on socket " << s
	       << " dest.port=" << dest.port << " portOffest=" << portOffset
	       << " port=" << port << Log::endl;
  }

  int r = ::connect(s, (struct sockaddr*)&sa, sizeof(sa));
  if (r < 0) {
    if (!SockUtil::errorWouldBlock()) {
      Log::perror("connect");
      //       ABORT("connect");
      TcpConnectionPtr c(new TcpConnection(0, TransportServiceClass::Connection::OUTGOING,
                                           cryptoFlags, ctx, mace::string(),
                                           0, 0, dest));
      std::ostringstream os;
      os << "connect error: " << dest;
      int err = SockUtil::getErrno();
      os << ": " << Util::getErrorString(err);
      c->close(TransportError::CONNECT_ERROR, os.str());
      ::close(s);
      if (nats >= 0) { ::close(nats); }
      errors.push(c);
      return c;
    }
  }

  SockAddr srcid = SockUtil::NULL_MSOCKADDR;
  if (!forwardingHost.isNull() || disableTranslations) {
    srcid = localAddr.local;
  }

//   macedbg(1) << "forwardingHost=" << forwardingHost
// 	     << " disableTranslations=" << disableTranslations
// 	     << " srcid=" << srcid << Log::endl;

  TcpConnectionPtr c(new TcpConnection(s, TransportServiceClass::Connection::OUTGOING,
				       cryptoFlags, ctx, connectionToken,
				       maxQueueSize, queueThreshold,
				       dest, srcid));
  connections[s] = c;
  
//   maceout << "connected: out[" << dest << "]=" << s << Log::endl;
  out[dest] = c;


  if (localAddr.isUnroutable()) {
    SockUtil::fillSockAddr(dest.addr, dest.port + portOffset, sa);
    if (!macedbg(1).isNoop()) {
      macedbg(1) << "making nat connection to" << Util::getHostname(dest.addr)
		 << ":" << dest.port + portOffset << Log::endl;
    }

    r = ::connect(nats, (struct sockaddr*)&sa, sizeof(sa));
    if (r < 0) {
      if (!SockUtil::errorWouldBlock()) {
	Log::perror("connect");
        // 	ABORT("connect");
      }
    }

    TcpConnectionPtr c(new TcpConnection(nats,
					 TransportServiceClass::Connection::NAT,
					 cryptoFlags, ctx, mace::string(),
					 maxQueueSize, queueThreshold));

    connections[nats] = c;
  
//   maceout << "connected: out[" << dest << "]=" << s << Log::endl;
//     out[dest] = c;
    
  }

  TransportScheduler::signal(0, true);
  return c;
} // connect

int TcpTransport::socket() {
  ADD_SELECTORS("TcpTransport::socket");

  int s = ::socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) {
    int err = SockUtil::getErrno();
    if (SockUtil::errorTransient(err)) {
      maceout << "error on socket(): " << Util::getErrorString(err) << Log::endl;
      garbageCollectSockets();
      s = ::socket(AF_INET, SOCK_STREAM, 0);
      if (s < 0) {
	int err = SockUtil::getErrno();
	if (SockUtil::errorTransient(err)) {
	  TransportScheduler::freeSockets();
	  return -1;
	}
// 	if (!SockUtil::errorWouldBlock()) {
	Log::perror("socket");
	ABORT("socket");
// 	}
      }
    }
    else {
      Log::perror("socket");
      ABORT("socket");
    }
  }

  ASSERT(s < FD_SETSIZE);
  SockUtil::setNonblock(s);
  if (setNodelay) {
    SockUtil::setNodelay(s);
  }

  return s;
} // socket

TransportServiceClass* tcpTransportInitFunction() {
  return new TcpTransport_namespace::TcpTransportService();
} // tcpTransportInitFunction

