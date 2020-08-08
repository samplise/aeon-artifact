/* 
 * SockUtil.cc : part of the Mace toolkit for building distributed systems
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
#include <boost/lexical_cast.hpp>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <iostream>
#include <errno.h>
#include "maceConfig.h"
#include "m_net.h"
#include <fcntl.h>

#include "SockUtil.h"
#include "Util.h"
#include "Log.h"

#if !(defined _WINSOCK2_H || defined _WINSOCK_H)
#define SOCKOPT_VOID_CAST
#else
#define SOCKOPT_VOID_CAST (char*)
// #define SHUT_RDWR SD_BOTH
#endif

// const msockaddr SockUtil::NULL_MSOCKADDR = msockaddr(INADDR_NONE, 0);
// const MaceAddr SockUtil::NULL_MACEADDR = MaceAddr(NULL_MSOCKADDR, NULL_MSOCKADDR);
const SockAddr SockUtil::NULL_MSOCKADDR = SockAddr();
const MaceAddr SockUtil::NULL_MACEADDR = MaceAddr();
const std::string SockUtil::NAT_STRING = "(NAT)";

using std::cerr;
using std::endl;

class InitSockUtil {
  public:
    bool initialized;

    InitSockUtil() : initialized(false) 
    {
      SockUtil::init();
      initialized = true;
    }
} winSockInit;

void SockUtil::init() {
  if (!winSockInit.initialized) {
#if (defined _WINSOCK2_H || defined _WINSOCK_H)
    WSADATA data;
    WORD wVersionRequested = MAKEWORD( 2, 2 );
    ASSERTMSG(WSAStartup(wVersionRequested, &data) == 0, "Could not initialize WinSock");
#endif
  }
}

void SockUtil::setNonblock(socket_t s) {
  #ifdef HAVE_FCNTL
  int f;
  if (((f = fcntl(s, F_GETFL)) < 0) ||
      (fcntl(s, F_SETFL, f | O_NONBLOCK) < 0)) {
    Log::perror("fcntl");
    ABORT("fcntl: cannot set nonblock");
  }
  #else
  unsigned long flags = 1;
  if (ioctlsocket(s, FIONBIO, &flags) == SOCKET_ERROR) {
    int er = WSAGetLastError();
    std::cerr << "WSAGetLastError: " << er << std::endl;
    Log::perror("ioctlsocket");
    ABORT("ioctlsocket: cannot set nonblock");
  }
  #endif
} // setNonblock

void SockUtil::setNodelay(socket_t s) {
  struct protoent *p;
  p = getprotobyname("tcp");
  if (!p) {
    Log::perror("getprotobyname");
    ABORT("no tcp proto");
  }
  int one = 1;
  if (setsockopt(s, p->p_proto, TCP_NODELAY, SOCKOPT_VOID_CAST &one, sizeof(one)) < 0) {
    Log::perror("setsockopt");
    ABORT("setsockopt: cannot set TCP_NODELAY");
  }
} // setNodelay

void SockUtil::setKeepalive(socket_t s) {
  /* Set the KEEPALIVE option active */
  int n = 1;
  //int optval = 0;
  //socklen_t optlen;
  int r;
  r = setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char*)&n, sizeof(n));
  if( r ) {
    Log::perror("setsockopt");
    ABORT("setsockopt: cannot set SO_KEEPALIVE");
  }

# ifdef TCP_KEEPCNT
#   ifdef TCP_KEEPIDLE
#     ifdef TCP_KEEPINTVL
        /* set up TCP_KEEPCNT */
        n = _TCP_KEEPCNT;
        r = setsockopt(s, SOL_TCP, TCP_KEEPCNT, (char*)&n, sizeof(n));
        if( r ) {
          Log::perror("setsockopt");
          ABORT("setsockopt: cannot set TCP_KEEPCNT");
        }

        /* set up TCP_KEEPIDLE */
        n = _TCP_KEEPIDLE;
        r = setsockopt(s, SOL_TCP, TCP_KEEPIDLE, (char*)&n, sizeof(n));
        if( r ) {
          Log::perror("setsockopt");
          ABORT("setsockopt: cannot set TCP_KEEPIDLE");
        }

        /* set up TCP_KEEPINTVL */
        n = _TCP_KEEPINTVL;
        r = setsockopt(s, SOL_TCP, TCP_KEEPINTVL, (char*)&n, sizeof(n));
        if( r ) {
          Log::perror("setsockopt");
          ABORT("setsocktopt:IDLE cannot set TCP_KEEPINTVL");
        }
#     endif //TCP_KEEPINTVL
#   endif //TCP_KEEPIDLE
# endif //TCP_KEEPCNT

} // setKeepalive

void SockUtil::fillSockAddr(const std::string& host, uint16_t port,
			    struct sockaddr_in& sa) throw (AddressException) {
  fillSockAddr((uint32_t)Util::getAddr(host), port, sa);
} // fillSockAddr

void SockUtil::fillSockAddr(uint32_t addr, uint16_t port, struct sockaddr_in& sa) {
  memset(&sa, 0, sizeof(sa));
  struct in_addr ina;
  ina.s_addr = addr;
  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  sa.sin_addr = ina;
} // fillSockAddr

void SockUtil::fillSockAddr(const SockAddr& addr, struct sockaddr_in& sa) {
  fillSockAddr(addr.addr, addr.port, sa);
} // fillSockAddr

SockAddr SockUtil::getSockAddr(const std::string& hostname) throw (AddressException) {
  ASSERT(winSockInit.initialized);
  uint16_t port = Util::getPort();
  size_t i = hostname.find(':');
  std::string host = hostname;
  if (i != std::string::npos) {
    host = hostname.substr(0, i);
    if (host == NAT_STRING) {
      return SockAddr(boost::lexical_cast<uint32_t>(hostname.substr(i + 1)), 0);
    }
    port = boost::lexical_cast<uint16_t>(hostname.substr(i + 1));
  }
  else if (host == NAT_STRING) {
    static uint32_t randIp = RandomUtil::randInt();
    return SockAddr(randIp, 0);
  }

  uint32_t ip = Util::getAddr(host);
  return SockAddr(ip, port);
} // getSockAddr

int SockUtil::getErrno() {
#if !(defined _WINSOCK2_H || defined _WINSOCK_H)
  return errno;
#else
  return WSAGetLastError();
#endif
}
bool SockUtil::errorWouldBlock(int err) {
  if (err == -1) { err = SockUtil::getErrno(); }
#if !(defined _WINSOCK2_H || defined _WINSOCK_H)
  return (err == EAGAIN || err == EWOULDBLOCK || err == EINTR || err == EINPROGRESS);
#else
  return (err == WSAEWOULDBLOCK || err == WSAEINPROGRESS || err == WSAEINTR);
#endif
}
bool SockUtil::errorInterrupted(int err) {
  if (err == -1) { err = SockUtil::getErrno(); }
#if !(defined _WINSOCK2_H || defined _WINSOCK_H)
  return (err == EINTR);
#else
  return (err == WSAEINTR);
#endif
}
bool SockUtil::errorBadF(int err) {
  if (err == -1) { err = SockUtil::getErrno(); }
#if !(defined _WINSOCK2_H || defined _WINSOCK_H)
  return (err == EBADF);
#else
  return (err == WSAEBADF);
#endif
}
bool SockUtil::errorConnFail(int err) {
  if (err == -1) { err = SockUtil::getErrno(); }
#if !(defined _WINSOCK2_H || defined _WINSOCK_H)
  return (err == ECONNABORTED || err == EPROTO);
#else
  return (err == WSAECONNABORTED || err == WSAEPROTONOSUPPORT || err == WSAEPROTOTYPE || err == WSAENOPROTOOPT || err == WSAESOCKTNOSUPPORT);
#endif
}
bool SockUtil::errorTransient(int err) {
  if (err == -1) { err = SockUtil::getErrno(); }
#if !(defined _WINSOCK2_H || defined _WINSOCK_H)
  return (err == ENFILE || err == EMFILE || err == ENOBUFS || err == ENOMEM); 
#else
  return (err == WSAENOBUFS || err == WSAEMFILE || err == WSAEINTR);
#endif
}
bool SockUtil::errorNotConnected(int err) {
  if (err == -1) { err = SockUtil::getErrno(); }
#if !(defined _WINSOCK2_H || defined _WINSOCK_H)
#  ifdef EINVAL_IS_NOTCONN
  return (err == ENOTCONN || err == EINVAL); 
#  else
  return (err == ENOTCONN); 
#  endif
#else
  return (err == WSAENOTCONN);
#endif
}
