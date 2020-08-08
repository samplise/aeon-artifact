/* 
 * BaseTransport.h : part of the Mace toolkit for building distributed systems
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
#ifndef BASE_TRANSPORT_H
#define BASE_TRANSPORT_H

#include <boost/shared_ptr.hpp>

#include "LockedSignal.h"
#include "SockUtil.h"
#include "ScopedTimer.h"
#include "CircularQueue.h"

#include "TransportHeader.h"
#include "BufferedTransportServiceClass.h"
#include "ConnectionAcceptanceServiceClass.h"
#include "Collections.h"

#include "Pipeline.h"
#include "ThreadPool.h"

#include "mace-macros.h"
#include "ThreadStructure.h"
#include "ScopedContextRPC.h"
#include "ContextMapping.h"

#ifdef STUPID_FD_SET_CONST_HACK
#define CONST_ISSET 
#else
#define CONST_ISSET const
#endif

class BaseTransport : public virtual TransportServiceClass,
		      public virtual ConnectionAcceptanceServiceClass {
public:
  class SocketException : public Exception {
  public:
    SocketException(const std::string& m) : Exception(m) { }
    virtual ~SocketException() throw() {}
    void rethrow() const { throw *this; }
  }; // SocketException

  class BindException : public SocketException {
  public:
    BindException(const std::string& m) : SocketException(m) { }
    virtual ~BindException() throw() {}
    void rethrow() const { throw *this; }
  }; // BindException

public:
  typedef mace::map<registration_uid_t, ReceiveDataHandler*, mace::SoftState> DataHandlerMap;
  typedef mace::map<registration_uid_t, NetworkErrorHandler*, mace::SoftState> NetworkHandlerMap;
  typedef mace::map<registration_uid_t, ConnectionStatusHandler*, mace::SoftState> ConnectionHandlerMap;
  typedef mace::map<registration_uid_t, ConnectionAcceptanceHandler*, mace::SoftState> AcceptanceHandlerMap;
  typedef CircularQueue<registration_uid_t> RegIdQueue;
  enum DeliverState { WAITING, DELIVER, RTS, ERROR, FLUSHED, FINITO, POST_FINITO };

  class DeliveryData
  {
    public:
      DeliverState deliverState;

      std::string shdr;
      std::string s;

      TransportHeader hdr; // includes src as MaceAddr, not MaceKey
      bool doAddRemoteKey; // on finish

      MaceKey remoteKey;

      NodeSet suspended; //Used in TCP only.
      ReceiveDataHandler* dataHandler;

      ConnectionStatusHandler* connectionStatusHandler;

      NetworkHandlerMap errorHandlers;

      void* ptr;


      DeliveryData() : deliverState(WAITING) {
      }

      ~DeliveryData() {}
  };

protected:
  typedef mace::map<SockAddr, MaceAddr> SourceTranslationMap;
  enum transport_upcall_t { UPCALL_DELIVER, UPCALL_ERROR, UPCALL_CTS };
  static const int TRANSPORT_TRACE_DELIVER = 1;
  static const int TRANSPORT_TRACE_ERROR = 2;
  static const int TRANSPORT_TRACE_CTS = 3;
  static const int TRANSPORT_TRACE_FLUSH = 4;

  DeliverState deliverState;

public:
  BaseTransport(int portoff, MaceAddr addr = SockUtil::NULL_MACEADDR,
		int backlog = SOMAXCONN,
		SockAddr forwarder = SockUtil::NULL_MSOCKADDR,
		SockAddr local = SockUtil::NULL_MSOCKADDR,
		int numDeliveryThreads = 1);
  virtual ~BaseTransport();

  virtual bool isRunning() {
    if (!running && doClose) {
      closeSockets();
    }
    return starting || running;
  } // isRunning

  virtual void addSockets(fd_set& rset, fd_set& wset, socket_t& selectMax) {
    //     Log::log("BaseTransport::addSockets") << "Considering Adding " << transportSocket << " to sockets." << Log::endl;
    if (running) {
      //       Log::log("BaseTransport::addSockets") << "Yes, Adding " << transportSocket << " to sockets." << Log::endl;
      FD_SET(transportSocket, &rset);
      selectMax = std::max(transportSocket, selectMax);
    }
  } // addSockets


  const SockAddr& getNextHop(const MaceAddr& dest) const {
    if (!forwardingHost.isNull()) {
      return forwardingHost;
    }

    if ((!dest.proxy.isNull()) &&
	(dest.proxy != localAddr.local)) {
      if (!disableTranslations) {
	proxying = true;
      }
      // send it to the proxy
      return dest.proxy;
    }
    return dest.local;
  } // getNextHop

  virtual const MaceKey& localAddress() const {
    return srcKey;
  } // localAddress

  virtual bool route(const MaceKey& src, const MaceKey& dest,
		     const std::string& s, bool rts, registration_uid_t rid) {
    return route(src.getMaceAddr(), dest, s, rts, rid);
  } // route

  virtual bool route(const MaceKey& dest, const std::string& s, bool rts,
		     registration_uid_t rid) {
    return route(srcAddr, dest, s, rts, rid);
  } // route

  virtual void setAuthoritativeConnectionAcceptor(registration_uid_t rid = -1);
  virtual void setConnectionToken(const mace::string& token,
				  registration_uid_t rid = -1);
  virtual void doIO(CONST_ISSET fd_set& rset, CONST_ISSET fd_set& wset,
		    uint64_t selectTime) = 0;

  virtual void run() throw(SocketException);
  virtual void run(uint32_t i);
  virtual void shutdown();
  virtual void maceInit();
  virtual void maceExit();
  virtual void freeSockets() = 0;

  virtual registration_uid_t registerHandler(ReceiveDataHandler& h,
					     registration_uid_t rid = -1, bool isAppHandler = true);
  virtual registration_uid_t registerHandler(NetworkErrorHandler& h,
					     registration_uid_t rid = -1, bool isAppHandler = true);
  virtual registration_uid_t registerHandler(ConnectionStatusHandler& h,
					     registration_uid_t rid = -1, bool isAppHandler = true);
  virtual registration_uid_t registerHandler(ConnectionAcceptanceHandler& h,
					     registration_uid_t rid = -1, bool isAppHandler = true);
  virtual void unregisterHandler(ReceiveDataHandler& h,
				 registration_uid_t rid = -1);
  virtual void unregisterHandler(NetworkErrorHandler& h,
				 registration_uid_t rid = -1);
  virtual void unregisterHandler(ConnectionStatusHandler& h,
				 registration_uid_t rid = -1);
  virtual void unregisterHandler(ConnectionAcceptanceHandler& h,
				 registration_uid_t rid = -1);
  virtual void registerUniqueHandler(ReceiveDataHandler& h);
  virtual void registerUniqueHandler(NetworkErrorHandler& h);
  virtual void registerUniqueHandler(ConnectionStatusHandler& h);
  virtual void registerUniqueHandler(ConnectionAcceptanceHandler& h);

  virtual void requestContextMigration(const uint8_t serviceID, const mace::string& contextID, const MaceAddr& destNode, const bool rootOnly){
    // dummy method. migration activity shouldn't affect Transport services
  }

  void setPipeline(PipelineElement* p) {
    pipeline = p;
  }

public:
  static const uint64_t DEFAULT_WINDOW_SIZE = 5*1000*1000;

protected:
  static void* startDeliverThread(void* arg);
  virtual int getSockType() = 0;
  //   virtual void runDeliverThread() = 0;
  virtual void closeConnections() = 0;
     
public:
  typedef mace::ThreadPool<BaseTransport,DeliveryData> ThreadPoolType;
  //   virtual bool deliverData(const std::string& shdr, mace::string& s,
  // 			   MaceKey* src = 0, NodeSet* suspended = 0);
  virtual void deliverDataSetup(ThreadPoolType* tp, DeliveryData& data);
  virtual void deliverData(DeliveryData& data);
protected:
  virtual bool acceptConnection(const MaceAddr& maddr, const mace::string& t);
  virtual bool sendData(const MaceAddr& src, const MaceKey& dest,
			const MaceAddr& nextHop, registration_uid_t rid,
			const std::string& ph, const std::string& s, bool checkQueueSize, bool rts) = 0;
  virtual void unregisterHandlers();

  void lock() const { ASSERT(pthread_mutex_lock(&tlock) == 0); }
  void unlock() const { ASSERT(pthread_mutex_unlock(&tlock) == 0); }
  void lockd() { ASSERT(pthread_mutex_lock(&dlock) == 0); }
  void unlockd() { ASSERT(pthread_mutex_unlock(&dlock) == 0); }
  void lockc() const { ASSERT(pthread_mutex_lock(&conlock) == 0); } // lock for connection
  void unlockc() const { ASSERT(pthread_mutex_unlock(&conlock) == 0); }
  void waitForDeliverSignal() {
//     ADD_SELECTORS("BaseTransport::waitForDeliverSignal");
//     maceout << "waiting for deliver signal" << Log::endl;
    ABORT("UNUSED");
    dsignal.wait();
//     maceout << "received signal" << Log::endl;
  }
  void signalDeliver() {
//     ADD_SELECTORS("BaseTransport::signalDeliver");
//     maceout << "signaling deliver" << Log::endl;
    if (tpptr != NULL) {
      tpptr->signal(); 
    }
    //     dsignal.signal();
  }
  virtual bool route(const MaceAddr& src, const MaceKey& dest,
		     const std::string& s, bool rts, registration_uid_t rid) {
      ADD_SELECTORS("BaseTransport::route");
    ScopedTimer st(routeTime);
    std::string str = s;
    std::string ph;

//     ASSERT(running);
    if (!running) { return false; }

    if( dest.addressFamily() != mace::CONTEXTNODE && 
      ThreadStructure::getCurrentContext() != mace::ContextMapping::getHeadContextID()  ){
      // chuangw: I assume this means it is an external message
      return ThreadStructure::deferExternalMessage( dest, s, rid );
    }

    if (pipeline) {
      pipeline->routeData(dest, ph, str, rid);
      if (str.empty()) {
        return false;
      }
    }

    macedbg(1) << "Routing Message of size " << s.size() << " from " << localKey << " to " << dest << " rid " << rid << Log::endl;
    // SANITY_CHECK
    DataHandlerMap::iterator i = dataHandlers.find(rid);
    if (i == dataHandlers.end()) {
        maceerr << "no handler registered with " << rid << Log::endl;
        return false;
    }
  
    bool r = sendData(src, dest, dest.getMaceAddr(), rid, ph, str, true, rts);
//     traceout << r << Log::end;
    return r;
  } // route

private:

  void setupSocket();
  void closeSockets();

private:
  uint initCount;

protected:
  //Data which can only be set in the constructor.
  const uint16_t portOffset;
  const uint16_t port; // baseport
  const MaceAddr localAddr;
  const uint32_t saddr;
  const MaceKey localKey;
  const SockAddr forwardingHost;
  const SockAddr localHost;
  const MaceAddr srcAddr;
  const MaceKey srcKey;
  const int numDeliveryThreads;
  const int backlog;

  socket_t transportSocket;

  bool starting;
  bool running;
  bool shuttingDown;
  bool doClose;
  mutable bool proxying;

  mutable pthread_mutex_t tlock;

  mutable pthread_mutex_t conlock;    // lock for connection

  LockedSignal dsignal;
  pthread_t deliverThread;
  pthread_mutex_t dlock;

  SourceTranslationMap translations;
  bool disableTranslations;

  mace::string connectionToken;
  ConnectionAcceptanceHandler* authoritativeConnectionAcceptor;
  registration_uid_t authoritativeConnectionAcceptorRid;

  DataHandlerMap dataHandlers;
  RegIdQueue dataUnreg;
  NetworkHandlerMap errorHandlers;
  RegIdQueue errorUnreg;
  ConnectionHandlerMap connectionHandlers;
  RegIdQueue connectionUnreg;
  AcceptanceHandlerMap acceptanceHandlers;
  RegIdQueue acceptanceUnreg;

  //   TransportHeader hdr;

  uint64_t routeTime;
  uint64_t sendDataTime;
  uint64_t doIOTime;

  PipelineElement* pipeline;

public:
  //   class DeliveryTransport
  //   {
  //     private:
  //       uint current_id;
  //       uint threadCount;
  //       std::vector<DeliveryData> data;
  //       std::vector<BaseTransport*> obj;
  //       std::vector<bool> is_idle;
  //       std::vector<bool> is_message;
  //       std::vector<pthread_mutex_t> lock;
  //       pthread_mutex_t lock_dt;
  //       bool (BaseTransport::**pFunc)( const std::string&, mace::string&, MaceKey*, NodeSet* );

  //   public:
  //     DeliveryTransport(int numThreads = 1)
  //       : threadCount(numThreads)
  //     {
  //       ADD_SELECTORS("DeliveryTransport::DeliveryTransport");
  //       maceout << "DeliveryTransport called." << Log::endl;
  //       lock.reserve(numThreads);

  //       pFunc = new (bool (BaseTransport::*[numThreads])( const std::string&, mace::string&, MaceKey*, NodeSet* ));

  //       for(uint16_t i=0; i<numThreads; i++ )
  //       {
  //         //Looks like a prime candidate for thread-specific data...
  //         data.push_back(DeliveryData());
  //         is_idle.push_back(true);
  //         is_message.push_back(false);
  //         obj.push_back(0);
  //         pthread_mutex_init(&lock[i], 0);
  //       }

  //       pthread_mutex_init(&lock_dt, 0);
  //       current_id = 0;
  //     }

  //     ~DeliveryTransport()
  //     {
  //       ADD_SELECTORS("DeliveryTransport::~DeliveryTransport");
  //       maceout << "~DeliveryTransport called." << Log::endl;
  //       delete [] pFunc;

  //       for(uint16_t i=0; i<threadCount; i++ )
  //       {
  //         pthread_mutex_destroy(&lock[i]);
  //       }
  //       pthread_mutex_destroy(&lock_dt);
  //     }

  //     // Looks like setMessage is assigning messages to individual threads
  //     // round robin...  This is totally at odds with how the thread pool is
  //     // designed.  It also means that the origin deliverThread will block if
  //     // this thread is currently busy delivering a message.
  //     void setMessage( const std::string& shdr, mace::string& s, BaseTransport *base_obj, bool(BaseTransport::*fun)(const std::string&, mace::string&, MaceKey*, NodeSet*), MaceKey* src = 0, NodeSet* suspended = 0)
  //     {
  //       ADD_SELECTORS("DeliveryTransport::setMessage");
  //       maceout << "setMessage called. current_id = " << current_id << "("<< threadCount <<")" << Log::endl;
  //       locks();
  //       ASSERT(threadCount > 0);
  //       ASSERT(current_id < threadCount);
  //       ASSERT(current_id >= 0 );

  //       //maceout << "set lock id = " << current_id << Log::endl;
  //       setLock(current_id);    // set lock so to guarantee existing runDeliver() to be exited earlier.

  //       data[current_id].shdr = shdr;
  //       data[current_id].s = s;
  //       data[current_id].remoteKey = remoteKey;
  //       data[current_id].suspended = suspended;
  //       obj[current_id] = base_obj;
  //       pFunc[current_id] = fun;
  //       is_message[current_id] = true;

  //       current_id = (current_id+1)%threadCount;
  //       unlocks();
  //     }

  //     bool isReady( uint id )
  //     {
  //       //A lot of optimizable debugging...
  //       ADD_SELECTORS("DeliveryTransport::isReady");
  //       if( is_idle[id] && is_message[id] )
  //         maceout << "isReady called. id = " << id << "("<< threadCount<<") is_ready = TRUE";
  //       else
  //         maceout << "isReady called. id = " << id << "("<< threadCount<<") is_ready = FALSE";

  //       if( is_idle[id] )
  //         maceout << "  is_idle = TRUE";
  //       else
  //         maceout << "  is_idle = FALSE";

  //       if( is_message[id] )
  //         maceout << "  is_message = TRUE" << Log::endl;
  //       else
  //         maceout << "  is_message = FALSE" << Log::endl;

  //       return is_idle[id] && is_message[id];
  //     }

  //     void runDeliver( uint id )
  //     {
  //       //locks();
  //       ADD_SELECTORS("DeliveryTransport::runDeliver");
  //       maceout << "runDeliver called. id = " << id << Log::endl;

  //       is_idle[id] = false;

  //       /*bool ret = */(obj[id]->*pFunc[id])( data[id].shdr, data[id].s, data[id].src, data[id].suspended );

  //       is_message[id] = false;
  //       releaseLock(id);
  //       is_idle[id] = true;

  //       // CHECKME : ret is not returned. Please check whether the returned value is used or not.
  //       //unlocks();
  //     }

  //     protected:
  //       void setLock( uint16_t id ) { ASSERT(pthread_mutex_lock(&lock[id]) == 0); }
  //       void releaseLock( uint16_t id ) { ASSERT(pthread_mutex_unlock(&lock[id]) == 0); }
  //       void locks() { ASSERT(pthread_mutex_lock(&lock_dt) == 0); }
  //       void unlocks() { ASSERT(pthread_mutex_unlock(&lock_dt) == 0); }
  //   };
  //   

  // DeliveryTransport *dt;
  
  ThreadPoolType *tpptr;

  // void deliverSetMessage( const std::string& shdr, mace::string& s, BaseTransport *base_obj, bool(BaseTransport::*fun)(const std::string&, mace::string&, MaceKey*, NodeSet*), MaceKey* src = 0, NodeSet* suspended = 0)
  // {
  //   //No need to pass the transport into deliverSetThread, as this is a method of the base transport itself...
  //   ADD_SELECTORS("BaseTransport::deliverSetMessage");
  //   dt->setMessage(shdr, s, base_obj, &BaseTransport::deliverData, src, suspended );
  //   assert( tp != NULL );
  //   maceout << "unlock signal() to ("<<tp->getThreadCount()<<") threads" << Log::endl;
  //   tp->signal();
  // }

protected:
  void setupThreadPool()
  {
    // Current design is a thread pool per transport. (for better or worse)
    ADD_SELECTORS("BaseTransport::setupThreadPool");
    const uint32_t maxThreadSize = params::get<uint32_t>( "MAX_TRANSPORT_THREADS", 8 );
    mace::ScopedContextRPC::setTransportThreads(numDeliveryThreads);
    maceout << "num Threads = " << numDeliveryThreads << ", max threads = "<< maxThreadSize << Log::endl;
#if __GNUC__ > 4 || \
    (__GNUC__ == 4 && (__GNUC_MINOR__ >= 5 ))
    // Works only for g++ >= 4.5
    tpptr = new typename ThreadPoolType::ThreadPool(*this, &BaseTransport::runDeliverCondition, &BaseTransport::runDeliverProcessUnlocked,&BaseTransport::runDeliverSetup,&BaseTransport::runDeliverFinish,ThreadStructure::TRANSPORT_THREAD_TYPE,numDeliveryThreads, maxThreadSize);
#else    
    tpptr = new ThreadPoolType::ThreadPool(*this, &BaseTransport::runDeliverCondition, &BaseTransport::runDeliverProcessUnlocked,&BaseTransport::runDeliverSetup,&BaseTransport::runDeliverFinish,ThreadStructure::TRANSPORT_THREAD_TYPE,numDeliveryThreads, maxThreadSize);
#endif    
  }

  void killThreadPool()
  {
    tpptr->waitForEmpty();
  }

public:
  //Called by ThreadPool -- needs to be public.
  virtual bool runDeliverCondition(ThreadPoolType* tp, uint threadId) = 0;
  virtual void runDeliverSetup(ThreadPoolType* tp, uint threadId) = 0;
  virtual void runDeliverProcessUnlocked(ThreadPoolType* tp, uint threadId) = 0;
  virtual void runDeliverFinish(ThreadPoolType* tp, uint threadId) = 0;

}; // BaseTransport

typedef boost::shared_ptr<BaseTransport> BaseTransportPtr;

#endif // BASE_TRANSPORT_H
