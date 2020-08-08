/* 
 * TcpTransport-init.h : part of the Mace toolkit for building distributed systems
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
#ifndef TcpTransport_init_h
#define TcpTransport_init_h

#include "TransportServiceClass.h"
#include "BufferedTransportServiceClass.h"
#include "TransportCryptoServiceClass.h"
#include "SockUtil.h"

namespace TcpTransport_namespace {

  static const int UPCALL_MESSAGE_ERROR = 1;
  static const int QUEUE_SIZE = 2;
  static const int THRESHOLD = 3;
  static const int PORT_OFFSET = 4;
  static const int BACKLOG = 5;
  static const int CRYPTO = 6;
  static const int SET_NO_DELAY = 7;
  static const int FORWARD_HOST = 8;
  static const int FORWARD_PORT = 9;
  static const int REJECT_ON_ROUTERTS = 10;
  static const int MAX_CONSECUTIVE_DELIVER = 11;
  static const int LOCAL_HOST = 12;
  static const int LOCAL_PORT = 13;
  static const int NUM_DELIVERY_THREADS = 14;

  typedef mace::map<int, int> OptionsMap;

TransportServiceClass& new_TcpTransport_Transport(
  TransportCryptoServiceClass::type cryptoFlags = TransportCryptoServiceClass::NONE,
  bool upcallMessageErrors = false,
  uint32_t queueSize = std::numeric_limits<uint32_t>::max(),
  uint32_t threshold = std::numeric_limits<uint32_t>::max(),
  int portoffset = std::numeric_limits<int32_t>::max(),
  int numDeliveryThreads = std::numeric_limits<int32_t>::max()
  );

// You can easily specify number of delivery threads with TransportEx(num_of_threads).
TransportServiceClass& new_TcpTransport_TransportEx(
  int numDeliveryThreads = std::numeric_limits<int32_t>::max(),
  TransportCryptoServiceClass::type cryptoFlags = TransportCryptoServiceClass::NONE,
  bool upcallMessageErrors = false,
  uint32_t queueSize = std::numeric_limits<uint32_t>::max(),
  uint32_t threshold = std::numeric_limits<uint32_t>::max(),
  int portoffset = std::numeric_limits<int32_t>::max()
  );

BufferedTransportServiceClass& new_TcpTransport_BufferedTransport(
  TransportCryptoServiceClass::type cryptoFlags = TransportCryptoServiceClass::NONE,
  bool upcallMessageErrors = false,
  uint32_t queueSize = std::numeric_limits<uint32_t>::max(),
  uint32_t threshold = std::numeric_limits<uint32_t>::max(),
  int portoffset = std::numeric_limits<int32_t>::max(),
  int numDeliveryThreads = std::numeric_limits<int32_t>::max()
  );

TransportServiceClass& private_new_TcpTransport_Transport(
  TransportCryptoServiceClass::type cryptoFlags = TransportCryptoServiceClass::NONE,
  bool upcallMessageErrors = false,
  uint32_t queueSize = std::numeric_limits<uint32_t>::max(),
  uint32_t threshold = std::numeric_limits<uint32_t>::max(),
  int portoffset = std::numeric_limits<int32_t>::max(),
  int numDeliveryThreads = std::numeric_limits<int32_t>::max()
  );

BufferedTransportServiceClass& private_new_TcpTransport_BufferedTransport(
  TransportCryptoServiceClass::type cryptoFlags = TransportCryptoServiceClass::NONE,
  bool upcallMessageErrors = false,
  uint32_t queueSize = std::numeric_limits<uint32_t>::max(),
  uint32_t threshold = std::numeric_limits<uint32_t>::max(),
  int portoffset = std::numeric_limits<int32_t>::max(),
  int numDeliveryThreads = std::numeric_limits<int32_t>::max()
  );

TransportServiceClass& new_TcpTransport_Transport(const OptionsMap& m);
BufferedTransportServiceClass& new_TcpTransport_BufferedTransport(const OptionsMap& m);

} // TcpTransport_namespace

#endif
