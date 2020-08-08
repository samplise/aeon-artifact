/* 
 * SockUtil.h : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, Charles Killian, Dejan Kostic, Ryan Braud, James W. Anderson, John Fisher-Ogden, Calvin Hubble, Duy Nguyen, Justin Burke, David Oppenheimer, Amin Vahdat, Adolfo Rodriguez, Sooraj Bhat
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
#ifndef __SOCK_UTIL_H
#define __SOCK_UTIL_H

#include "Exception.h"
#include "MaceTypes.h"
#include "maceConfig.h"

/**
 * \file SockUtil.h
 * \brief defines the SockUtil utilities
 */

/**
 * \addtogroup Utils
 * @{
 */

/**
 * \brief helper methods for dealing with sockets.  Use instead of direct methods for portability.
 */
class SockUtil {
public:
  static void init(); ///< automatically called.  Added for WinSock, which needs to be initialized
  static void setNonblock(socket_t s); ///< Make socket s be non-blocking
  static void setNodelay(socket_t s);
  static void setKeepalive(socket_t s); ///< Make socket s to use keepalive
  static void fillSockAddr(const std::string& host, uint16_t port,
			   struct sockaddr_in& sa) throw (AddressException); ///< fill contents of the sockaddr_in from the host and port
  static void fillSockAddr(uint32_t addr, uint16_t port, struct sockaddr_in& sa); ///< fill contents of the sockaddr_in from the addr and port
  static void fillSockAddr(const SockAddr& addr, struct sockaddr_in& sa); ///< fill a sockaddr_in from a SockAddr
  static SockAddr getSockAddr(const std::string& hostname) throw (AddressException); ///< return a SockAddr from a hostname

  static int getErrno(); ///< Use instead of the variable "errno"
  static bool errorWouldBlock(int err = -1); ///< Check of the err (or getErrno()) is because call would block (EAGAIN, EWOULDBLOCK, EINTR, or EINPROGRESS)
  static bool errorInterrupted(int err = -1); ///< Check if the err (or getErrno()) is interrupted (EINTR)
  static bool errorBadF(int err = -1); ///< Check if the err (or getErrno()) is a bad file descriptor (EBADF)
  static bool errorConnFail(int err = -1); ///< Check if the err (or getErrno()) is a connection failure (ECONNABORTED or EPROTO)
  static bool errorTransient(int err = -1); ///< Check if the err (or getErrno()) is a transient system error (ENFILE, EMFILE, ENOBUFS, ENOMEM)
  static bool errorNotConnected(int err = -1); ///< Check if the err (or getErrno()) is not connected (ENOTCONN or EINVAL)

  static const SockAddr NULL_MSOCKADDR; ///< convenience object for a "null" SockAddr
  static const MaceAddr NULL_MACEADDR; ///< convenience object for a "null" MaceAddr
  static const std::string NAT_STRING; ///< reference "NAT" string (presently "(NAT)"
  static const int _TCP_KEEPCNT = 3;
  static const int _TCP_KEEPINTVL = 10;
  static const int _TCP_KEEPIDLE = 10;
}; // SockUtil

/** @} */

#endif // __SOCK_UTIL_H
