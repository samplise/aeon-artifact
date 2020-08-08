/* 
 * Util.cc : part of the Mace toolkit for building distributed systems
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
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <errno.h>
#include <fcntl.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <boost/lexical_cast.hpp>
#include <pthread.h>

#include <math.h>
#include "Util.h"
// #include "RandomUtil.h"
#include "hash_string.h"
#include "LRUCache.h"
#include "params.h"
#include "SockUtil.h"
#include "ScopedLock.h"
#include "mace-macros.h"
#include "m_net.h"

using std::cerr;
using std::endl;
using std::string;
using boost::lexical_cast;

const size_t Util::DNS_CACHE_TIMEOUT;
const unsigned Util::DNS_CACHE_SIZE;
const uint16_t Util::DEFAULT_MACE_PORT;
const bool Util::REPLAY = false;


uint16_t Util::baseport = 0;
static pthread_mutex_t ulock = PTHREAD_MUTEX_INITIALIZER;

std::string Util::getErrorString(int err) {
  char* errbuf = strerror(err);
#if (defined _WINSOCK2_H || defined _WINSOCK_H)
  if (err > 10000 && err < 11032) {
    switch(err) {
      case WSAEINTR : // 10004
        errbuf = "Interrupted function call.";
        break;

        //     A blocking operation was interrupted by a call to WSACancelBlockingCall.

      case WSAEBADF : // 10009
        errbuf = "File handle is not valid.";
        break;

        //     The file handle supplied is not valid.

      case WSAEACCES : // 10013
        errbuf = "Permission denied.";
        break;

        //     An attempt was made to access a socket in a way forbidden by its access permissions. An example is using a broadcast address for sendto without broadcast permission being set using setsockopt(SO_BROADCAST).

        //     Another possible reason for the WSAEACCES error is that when the bind function is called (on Windows NT 4 SP4 or later), another application, service, or kernel mode driver is bound to the same address with exclusive access. Such exclusive access is a new feature of Windows NT 4 SP4 and later, and is implemented by using the SO_EXCLUSIVEADDRUSE option.

      case WSAEFAULT : // 10014
        errbuf = "Bad address.";
        break;

        //     The system detected an invalid pointer address in attempting to use a pointer argument of a call. This error occurs if an application passes an invalid pointer value, or if the length of the buffer is too small. For instance, if the length of an argument, which is a sockaddr structure, is smaller than the sizeof(sockaddr).

      case WSAEINVAL : // 10022
        errbuf = "Invalid argument.";
        break;

        //     Some invalid argument was supplied (for example, specifying an invalid level to the setsockopt function). In some instances, it also refers to the current state of the socketfor instance, calling accept on a socket that is not listening.

      case WSAEMFILE : // 10024
        errbuf = "Too many open files.";
        break;

        //     Too many open sockets. Each implementation may have a maximum number of socket handles available, either globally, per process, or per thread.

      case WSAEWOULDBLOCK : // 10035
        errbuf = "Resource temporarily unavailable.";
        break;

        //     This error is returned from operations on nonblocking sockets that cannot be completed immediately, for example recv when no data is queued to be read from the socket. It is a nonfatal error, and the operation should be retried later. It is normal for WSAEWOULDBLOCK to be reported as the result from calling connect on a nonblocking SOCK_STREAM socket, since some time must elapse for the connection to be established.

      case WSAEINPROGRESS : // 10036
        errbuf = "Operation now in progress.";
        break;

        //     A blocking operation is currently executing. Windows Sockets only allows a single blocking operationper- task or threadto be outstanding, and if any other function call is made (whether or not it references that or any other socket) the function fails with the WSAEINPROGRESS error.

      case WSAEALREADY : // 10037
        errbuf = "Operation already in progress.";
        break;

        //     An operation was attempted on a nonblocking socket with an operation already in progressthat is, calling connect a second time on a nonblocking socket that is already connecting, or canceling an asynchronous request (WSAAsyncGetXbyY) that has already been canceled or completed.

      case WSAENOTSOCK : // 10038
        errbuf = "Socket operation on nonsocket.";
        break;

        //     An operation was attempted on something that is not a socket. Either the socket handle parameter did not reference a valid socket, or for select, a member of an fd_set was not valid.

      case WSAEDESTADDRREQ : // 10039
        errbuf = "Destination address required.";
        break;

        //     A required address was omitted from an operation on a socket. For example, this error is returned if sendto is called with the remote address of ADDR_ANY.

      case WSAEMSGSIZE : // 10040
        errbuf = "Message too long.";
        break;

        //     A message sent on a datagram socket was larger than the internal message buffer or some other network limit, or the buffer used to receive a datagram was smaller than the datagram itself.

      case WSAEPROTOTYPE : // 10041
        errbuf = "Protocol wrong type for socket.";
        break;

        //     A protocol was specified in the socket function call that does not support the semantics of the socket type requested. For example, the ARPA Internet UDP protocol cannot be specified with a socket type of SOCK_STREAM.

      case WSAENOPROTOOPT : // 10042
        errbuf = "Bad protocol option.";
        break;

        //     An unknown, invalid or unsupported option or level was specified in a getsockopt or setsockopt call.

      case WSAEPROTONOSUPPORT : // 10043
        errbuf = "Protocol not supported.";
        break;

        //     The requested protocol has not been configured into the system, or no implementation for it exists. For example, a socket call requests a SOCK_DGRAM socket, but specifies a stream protocol.

      case WSAESOCKTNOSUPPORT : // 10044
        errbuf = "Socket type not supported.";
        break;

        //     The support for the specified socket type does not exist in this address family. For example, the optional type SOCK_RAW might be selected in a socket call, and the implementation does not support SOCK_RAW sockets at all.

      case WSAEOPNOTSUPP : // 10045
        errbuf = "Operation not supported.";
        break;

        //     The attempted operation is not supported for the type of object referenced. Usually this occurs when a socket descriptor to a socket that cannot support this operation is trying to accept a connection on a datagram socket.

      case WSAEPFNOSUPPORT : // 10046
        errbuf = "Protocol family not supported.";
        break;

        //     The protocol family has not been configured into the system or no implementation for it exists. This message has a slightly different meaning from WSAEAFNOSUPPORT. However, it is interchangeable in most cases, and all Windows Sockets functions that return one of these messages also specify WSAEAFNOSUPPORT.

      case WSAEAFNOSUPPORT : // 10047
        errbuf = "Address family not supported by protocol family.";
        break;

        //     An address incompatible with the requested protocol was used. All sockets are created with an associated address family (that is, AF_INET for Internet Protocols) and a generic protocol type (that is, SOCK_STREAM). This error is returned if an incorrect protocol is explicitly requested in the socket call, or if an address of the wrong family is used for a socket, for example, in sendto.

      case WSAEADDRINUSE : // 10048
        errbuf = "Address already in use.";
        break;

        //     Typically, only one usage of each socket address (protocol/IP address/port) is permitted. This error occurs if an application attempts to bind a socket to an IP address/port that has already been used for an existing socket, or a socket that was not closed properly, or one that is still in the process of closing. For server applications that need to bind multiple sockets to the same port number, consider using setsockopt (SO_REUSEADDR). Client applications usually need not call bind at all connect chooses an unused port automatically. When bind is called with a wildcard address (involving ADDR_ANY), a WSAEADDRINUSE error could be delayed until the specific address is committed. This could happen with a call to another function later, including connect, listen, WSAConnect, or WSAJoinLeaf.

      case WSAEADDRNOTAVAIL : // 10049
        errbuf = "Cannot assign requested address.";
        break;

        //     The requested address is not valid in its context. This normally results from an attempt to bind to an address that is not valid for the local computer. This can also result from connect, sendto, WSAConnect, WSAJoinLeaf, or WSASendTo when the remote address or port is not valid for a remote computer (for example, address or port 0).

      case WSAENETDOWN : // 10050
        errbuf = "Network is down.";
        break;

        //     A socket operation encountered a dead network. This could indicate a serious failure of the network system (that is, the protocol stack that the Windows Sockets DLL runs over), the network interface, or the local network itself.

      case WSAENETUNREACH : // 10051
        errbuf = "Network is unreachable.";
        break;

        //     A socket operation was attempted to an unreachable network. This usually means the local software knows no route to reach the remote host.

      case WSAENETRESET : // 10052
        errbuf = "Network dropped connection on reset.";
        break;

        //     The connection has been broken due to keep-alive activity detecting a failure while the operation was in progress. It can also be returned by setsockopt if an attempt is made to set SO_KEEPALIVE on a connection that has already failed.

      case WSAECONNABORTED : // 10053
        errbuf = "Software caused connection abort.";
        break;

        //     An established connection was aborted by the software in your host computer, possibly due to a data transmission time-out or protocol error.

      case WSAECONNRESET : // 10054
        errbuf = "Connection reset by peer.";
        break;

        //     An existing connection was forcibly closed by the remote host. This normally results if the peer application on the remote host is suddenly stopped, the host is rebooted, the host or remote network interface is disabled, or the remote host uses a hard close (see setsockopt for more information on the SO_LINGER option on the remote socket). This error may also result if a connection was broken due to keep-alive activity detecting a failure while one or more operations are in progress. Operations that were in progress fail with WSAENETRESET. Subsequent operations fail with WSAECONNRESET.

      case WSAENOBUFS : // 10055
        errbuf = "No buffer space available.";
        break;

        //     An operation on a socket could not be performed because the system lacked sufficient buffer space or because a queue was full.

      case WSAEISCONN : // 10056
        errbuf = "Socket is already connected.";
        break;

        //     A connect request was made on an already-connected socket. Some implementations also return this error if sendto is called on a connected SOCK_DGRAM socket (for SOCK_STREAM sockets, the to parameter in sendto is ignored) although other implementations treat this as a legal occurrence.

      case WSAENOTCONN : // 10057
        errbuf = "Socket is not connected.";
        break;

        //     A request to send or receive data was disallowed because the socket is not connected and (when sending on a datagram socket using sendto) no address was supplied. Any other type of operation might also return this errorfor example, setsockopt setting SO_KEEPALIVE if the connection has been reset.

      case WSAESHUTDOWN : // 10058
        errbuf = "Cannot send after socket shutdown.";
        break;

        //     A request to send or receive data was disallowed because the socket had already been shut down in that direction with a previous shutdown call. By calling shutdown a partial close of a socket is requested, which is a signal that sending or receiving, or both have been discontinued.

      case WSAETOOMANYREFS : // 10059
        errbuf = "Too many references.";
        break;

        //     Too many references to some kernel object.

      case WSAETIMEDOUT : // 10060
        errbuf = "Connection timed out.";
        break;

        //     A connection attempt failed because the connected party did not properly respond after a period of time, or the established connection failed because the connected host has failed to respond.

      case WSAECONNREFUSED : // 10061
        errbuf = "Connection refused.";
        break;

        //     No connection could be made because the target computer actively refused it. This usually results from trying to connect to a service that is inactive on the foreign hostthat is, one with no server application running.

      case WSAELOOP : // 10062
        errbuf = "Cannot translate name.";
        break;

        //     Cannot translate a name.

      case WSAENAMETOOLONG : // 10063
        errbuf = "Name too long.";
        break;

        //     A name component or a name was too long.

      case WSAEHOSTDOWN : // 10064
        errbuf = "Host is down.";
        break;

        //     A socket operation failed because the destination host is down. A socket operation encountered a dead host. Networking activity on the local host has not been initiated. These conditions are more likely to be indicated by the error WSAETIMEDOUT.

      case WSAEHOSTUNREACH : // 10065
        errbuf = "No route to host.";
        break;

        //     A socket operation was attempted to an unreachable host. See WSAENETUNREACH.

      case WSAENOTEMPTY : // 10066
        errbuf = "Directory not empty.";
        break;

        //     Cannot remove a directory that is not empty.

      case WSAEPROCLIM : // 10067
        errbuf = "Too many processes.";
        break;

        //     A Windows Sockets implementation may have a limit on the number of applications that can use it simultaneously.WSAStartup may fail with this error if the limit has been reached.

      case WSAEUSERS : // 10068
        errbuf = "User quota exceeded.";
        break;

        //     Ran out of user quota.

      case WSAEDQUOT : // 10069
        errbuf = "Disk quota exceeded.";
        break;

        //     Ran out of disk quota.

      case WSAESTALE : // 10070
        errbuf = "Stale file handle reference.";
        break;

        //     The file handle reference is no longer available.

      case WSAEREMOTE : // 10071
        errbuf = "Item is remote.";
        break;

        //     The item is not available locally.

      case WSASYSNOTREADY : // 10091
        errbuf = "Network subsystem is unavailable.";
        break;

        //     This error is returned by WSAStartup if the Windows Sockets implementation cannot function at this time because the underlying system it uses to provide network services is currently unavailable. Users should check:

        //     * That the appropriate Windows Sockets DLL file is in the current path.
        //     * That they are not trying to use more than one Windows Sockets implementation simultaneously. If there is more than one Winsock DLL on your system, be sure the first one in the path is appropriate for the network subsystem currently loaded.
        //     * The Windows Sockets implementation documentation to be sure all necessary components are currently installed and configured correctly.

      case WSAVERNOTSUPPORTED : // 10092
        errbuf = "Winsock.dll version out of range.";
        break;

        //     The current Windows Sockets implementation does not support the Windows Sockets specification version requested by the application. Check that no old Windows Sockets DLL files are being accessed.

      case WSANOTINITIALISED : // 10093
        errbuf = "Successful WSAStartup not yet performed.";
        break;

        //     Either the application has not called WSAStartup or WSAStartup failed. The application may be accessing a socket that the current active task does not own (that is, trying to share a socket between tasks), or WSACleanup has been called too many times.

      case WSAEDISCON : // 10101
        errbuf = "Graceful shutdown in progress.";
        break;

        //     Returned by WSARecv and WSARecvFrom to indicate that the remote party has initiated a graceful shutdown sequence.

      case WSAENOMORE : // 10102
        errbuf = "No more results.";
        break;

        //     No more results can be returned by the WSALookupServiceNext function.

      case WSAECANCELLED : // 10103
        errbuf = "Call has been canceled.";
        break;

        //     A call to the WSALookupServiceEnd function was made while this call was still processing. The call has been canceled.

      case WSAEINVALIDPROCTABLE : // 10104
        errbuf = "Procedure call table is invalid.";
        break;

        //     The service provider procedure call table is invalid. A service provider returned a bogus procedure table to Ws2_32.dll. This is usually caused by one or more of the function pointers being NULL.

      case WSAEINVALIDPROVIDER : // 10105
        errbuf = "Service provider is invalid.";
        break;

        //     The requested service provider is invalid. This error is returned by the WSCGetProviderInfo and WSCGetProviderInfo32 functions if the protocol entry specified could not be found. This error is also returned if the service provider returned a version number other than 2.0.

      case WSAEPROVIDERFAILEDINIT : // 10106
        errbuf = "Service provider failed to initialize.";
        break;

        //     The requested service provider could not be loaded or initialized. This error is returned if either a service provider's DLL could not be loaded (LoadLibrary failed) or the provider's WSPStartup or NSPStartup function failed.

      case WSASYSCALLFAILURE : // 10107
        errbuf = "System call failure.";
        break;

        //     A system call that should never fail has failed. This is a generic error code, returned under various conditions.

        //     Returned when a system call that should never fail does fail. For example, if a call to WaitForMultipleEvents fails or one of the registry functions fails trying to manipulate the protocol/namespace catalogs.

        //     Returned when a provider does not return SUCCESS and does not provide an extended error code. Can indicate a service provider implementation error.

      case WSASERVICE_NOT_FOUND : // 10108
        errbuf = "Service not found.";
        break;

        //     No such service is known. The service cannot be found in the specified name space.

      case WSATYPE_NOT_FOUND : // 10109
        errbuf = "Class type not found.";
        break;

        //     The specified class was not found.

      case WSA_E_NO_MORE : // 10110
        errbuf = "No more results.";
        break;

        //     No more results can be returned by the WSALookupServiceNext function.

      case WSA_E_CANCELLED : // 10111
        errbuf = "Call was canceled.";
        break;

        //     A call to the WSALookupServiceEnd function was made while this call was still processing. The call has been canceled.

      case WSAEREFUSED : // 10112
        errbuf = "Database query was refused.";
        break;

        //     A database query failed because it was actively refused.

      case WSAHOST_NOT_FOUND : // 11001
        errbuf = "Host not found.";
        break;

        //     No such host is known. The name is not an official host name or alias, or it cannot be found in the database(s) being queried. This error may also be returned for protocol and service queries, and means that the specified name could not be found in the relevant database.

      case WSATRY_AGAIN : // 11002
        errbuf = "Nonauthoritative host not found.";
        break;

        //     This is usually a temporary error during host name resolution and means that the local server did not receive a response from an authoritative server. A retry at some time later may be successful.

      case WSANO_RECOVERY : // 11003
        errbuf = "This is a nonrecoverable error.";
        break;

        //     This indicates that some sort of nonrecoverable error occurred during a database lookup. This may be because the database files (for example, BSD-compatible HOSTS, SERVICES, or PROTOCOLS files) could not be found, or a DNS request was returned by the server with a severe error.

      case WSANO_DATA : // 11004
        errbuf = "Valid name, no data record of requested type.";
        break;

        //     The requested name is valid and was found in the database, but it does not have the correct associated data being resolved for. The usual example for this is a host name-to-address translation attempt (using gethostbyname or WSAAsyncGetHostByName) which uses the DNS (Domain Name Server). An MX record is returned but no A recordindicating the host itself exists, but is not directly reachable.

      case WSA_QOS_RECEIVERS : // 11005
        errbuf = "QOS receivers.";
        break;

        //     At least one QOS reserve has arrived.

      case WSA_QOS_SENDERS : // 11006
        errbuf = "QOS senders.";
        break;

        //     At least one QOS send path has arrived.

      case WSA_QOS_NO_SENDERS : // 11007
        errbuf = "No QOS senders.";
        break;

        //     There are no QOS senders.

      case WSA_QOS_NO_RECEIVERS : // 11008
        errbuf = "QOS no receivers.";
        break;

        //     There are no QOS receivers.

      case WSA_QOS_REQUEST_CONFIRMED : // 11009
        errbuf = "QOS request confirmed.";
        break;

        //     The QOS reserve request has been confirmed.

      case WSA_QOS_ADMISSION_FAILURE : // 11010
        errbuf = "QOS admission error.";
        break;

        //     A QOS error occurred due to lack of resources.

      case WSA_QOS_POLICY_FAILURE : // 11011
        errbuf = "QOS policy failure.";
        break;

        //     The QOS request was rejected because the policy system couldn't allocate the requested resource within the existing policy.

      case WSA_QOS_BAD_STYLE : // 11012
        errbuf = "QOS bad style.";
        break;

        //     An unknown or conflicting QOS style was encountered.

      case WSA_QOS_BAD_OBJECT : // 11013
        errbuf = "QOS bad object.";
        break;

        //     A problem was encountered with some part of the filterspec or the provider-specific buffer in general.

      case WSA_QOS_TRAFFIC_CTRL_ERROR : // 11014
        errbuf = "QOS traffic control error.";
        break;

        //     An error with the underlying traffic control (TC) API as the generic QOS request was converted for local enforcement by the TC API. This could be due to an out of memory error or to an internal QOS provider error.

      case WSA_QOS_GENERIC_ERROR : // 11015
        errbuf = "QOS generic error.";
        break;

        //     A general QOS error.

      case WSA_QOS_ESERVICETYPE : // 11016
        errbuf = "QOS service type error.";
        break;

        //     An invalid or unrecognized service type was found in the QOS flowspec.

      case WSA_QOS_EFLOWSPEC : // 11017
        errbuf = "QOS flowspec error.";
        break;

        //     An invalid or inconsistent flowspec was found in the QOS structure.

      case WSA_QOS_EPROVSPECBUF : // 11018
        errbuf = "Invalid QOS provider buffer.";
        break;

        //     An invalid QOS provider-specific buffer.

      case WSA_QOS_EFILTERSTYLE : // 11019
        errbuf = "Invalid QOS filter style.";
        break;

        //     An invalid QOS filter style was used.

      case WSA_QOS_EFILTERTYPE : // 11020
        errbuf = "Invalid QOS filter type.";
        break;

        //     An invalid QOS filter type was used.

      case WSA_QOS_EFILTERCOUNT : // 11021
        errbuf = "Incorrect QOS filter count.";
        break;

        //     An incorrect number of QOS FILTERSPECs were specified in the FLOWDESCRIPTOR.

      case WSA_QOS_EOBJLENGTH : // 11022
        errbuf = "Invalid QOS object length.";
        break;

        //     An object with an invalid ObjectLength field was specified in the QOS provider-specific buffer.

      case WSA_QOS_EFLOWCOUNT : // 11023
        errbuf = "Incorrect QOS flow count.";
        break;

        //     An incorrect number of flow descriptors was specified in the QOS structure.

      //case WSA_QOS_EUNKOWNPSOBJ : // 11024
      //  errbuf = "Unrecognized QOS object.";
      //  break;

      //  //     An unrecognized object was found in the QOS provider-specific buffer.

      case WSA_QOS_EPOLICYOBJ : // 11025
        errbuf = "Invalid QOS policy object.";
        break;

        //     An invalid policy object was found in the QOS provider-specific buffer.

      case WSA_QOS_EFLOWDESC : // 11026
        errbuf = "Invalid QOS flow descriptor.";
        break;

        //     An invalid QOS flow descriptor was found in the flow descriptor list.

      case WSA_QOS_EPSFLOWSPEC : // 11027
        errbuf = "Invalid QOS provider-specific flowspec.";
        break;

        //     An invalid or inconsistent flowspec was found in the QOS provider-specific buffer.

      case WSA_QOS_EPSFILTERSPEC : // 11028
        errbuf = "Invalid QOS provider-specific filterspec.";
        break;

        //     An invalid FILTERSPEC was found in the QOS provider-specific buffer.

      case WSA_QOS_ESDMODEOBJ : // 11029
        errbuf = "Invalid QOS shape discard mode object.";
        break;

        //     An invalid shape discard mode object was found in the QOS provider-specific buffer.

      case WSA_QOS_ESHAPERATEOBJ : // 11030
        errbuf = "Invalid QOS shaping rate object.";
        break;

        //     An invalid shaping rate object was found in the QOS provider-specific buffer.

      case WSA_QOS_RESERVED_PETYPE : // 11031
        errbuf = "Reserved policy QOS element type.";
        break;

        //     A reserved policy element was found in the QOS provider-specific buffer.

    }
  }
#endif
  return std::string(errbuf);
} // getErrorString

std::string Util::getSslErrorString() {
  unsigned long e = ERR_get_error();
  if (e) {
    size_t len = 1024;
    char buf[len];
    ERR_error_string_n(e, buf, len);
    return string(buf);
  }
  else {
    return "";
  }
} // getSslErrorString

MaceAddr Util::getMaceAddr(const std::string& hostname) throw (AddressException) {
  size_t i = hostname.find('/');
  std::string pub = hostname;
  SockAddr proxyaddr;
  if (i != std::string::npos) {
    proxyaddr = SockUtil::getSockAddr(hostname.substr(i + 1));
    pub = hostname.substr(0, i);
  }
//   else if (hostname.find(SockUtil::NAT_STRING) == 0) {
//     return MaceAddr(SockAddr(getAddr(), getPort()), SockUtil::getSockAddr(hostname));
//   }
  return MaceAddr(SockUtil::getSockAddr(pub), proxyaddr);
} // getMaceAddr

uint16_t Util::getPort() {
  if (baseport == 0) {
    ScopedLock sl(ulock);
    if (params::containsKey(params::MACE_LOCAL_ADDRESS)) {
      std::string hostname = params::get<string>(params::MACE_LOCAL_ADDRESS);
      if (hostname.find('/') != std::string::npos) {
	hostname = hostname.substr(0, hostname.find('/'));
      }
      size_t i = hostname.find(':');
      if (i != std::string::npos) {
	baseport = lexical_cast<int16_t>(hostname.substr(i + 1));
      }
    }
    if (baseport == 0) {
      baseport = params::get(params::MACE_PORT, DEFAULT_MACE_PORT);
    }
  }
  return baseport;
} // getPort

const MaceAddr& Util::getMaceAddr() throw (AddressException) {
  static MaceAddr maddr = SockUtil::NULL_MACEADDR;

  if (maddr != SockUtil::NULL_MACEADDR) {
    return maddr;
  }

  if (params::containsKey(params::MACE_LOCAL_ADDRESS)) {
    maddr = getMaceAddr(params::get<string>(params::MACE_LOCAL_ADDRESS));
    if (!maddr.isUnroutable()) {
      baseport = maddr.local.port;
    }
  }
  else {
    maddr.local.addr = getAddr();
    maddr.local.port = getPort();
  }

  return maddr;
} // getMaceAddr

int Util::getAddr() throw (AddressException) {
  static int ipaddr = 0;
  if (ipaddr) {
    return ipaddr;
  }

  ipaddr = getAddr(getHostname());
  return ipaddr;
} // getAddr

// uint16_t Util::getPort() {
//   static uint16_t port = 0;
//   if (port) {
//     return port;
//   }
//   if (params::isset("MACE_PORT")) {
//     port = (uint16_t)params::getint("MACE_PORT");
//   }
//   else {
//     port = DEFAULT_MACE_PORT;
//   }
//   return port;
// } // getPort

int Util::getAddr(const std::string& hostname) throw (AddressException) {
  ADD_SELECTORS("Util::getAddr");
//   typedef mace::LRUCache<std::string, int, hash_string> DnsCache;
  typedef mace::LRUCache<std::string, int> DnsCache;
  static DnsCache dnsCache(DNS_CACHE_SIZE, DNS_CACHE_TIMEOUT);
  static bool allowLoopback = params::get<bool>(params::MACE_ADDRESS_ALLOW_LOOPBACK, false);
  static bool warnLoopback = params::get(params::MACE_WARN_LOOPBACK, true);
  int r = 0;
  ScopedLock sl(ulock);
  if (dnsCache.containsKey(hostname)) {
    r = dnsCache.get(hostname);
    sl.unlock();
  }
  else {
    sl.unlock();
    struct addrinfo hints, *res;
    hints.ai_flags = (AI_V4MAPPED | AI_ADDRCONFIG);
    hints.ai_family = AF_INET; // Specifically request IPv4
    hints.ai_socktype = hints.ai_protocol = 0;

    // getaddrinfo is re-entrant (gethostbyname is not)
    int err = getaddrinfo(hostname.c_str(), NULL, &hints, &res);
    if (err == EAI_ADDRFAMILY) { // No IPv4 addr for this host
      ;//return r;
    }
    if (err != 0 || !res) {
      if (hostname.find(':') != std::string::npos) {
	throw AddressException("Util::getAddr: bad hostname: " + hostname);
      }
      throw AddressException("getaddrinfo " + hostname + ": " + gai_strerror(err));
    }

    r = ((struct sockaddr_in*)(res->ai_addr))->sin_addr.s_addr;
    freeaddrinfo(res);
    
    dnsCache.add(hostname, r);
  }
  //sl.unlock();

  if ((r & 0x000000ff) == 0x7f) {
    if (warnLoopback) {
      macewarn << "hostname " << hostname << " is the loopback." << Log::endl;
    }
    if (!allowLoopback) {
      ABORT("Loopback address detected, but parameter MACE_ADDRESS_ALLOW_LOOPBACK not set to true");
    }
  }

  return r;
} // getAddr

std::string Util::getAddrString(const MaceAddr& addr, bool resolve) {
  std::ostringstream out;
  printAddrString(out, addr, resolve);
  return out.str();
} // getAddrString

void Util::printAddrString(std::ostream& out, const MaceAddr& addr, bool resolve) {
  printAddrString(out, addr.local, resolve);
  if(addr.proxy != SockUtil::NULL_MSOCKADDR) {
    out << "/";
    printAddrString(out, addr.proxy, resolve);
  };
} // printAddrString

std::string Util::getAddrString(const SockAddr& addr, bool resolve) {
  std::ostringstream out;
  printAddrString(out, addr, resolve);
  return out.str();
} // getAddrString

void Util::printAddrString(std::ostream& r, const SockAddr& addr, bool resolve) {
  if (addr.port == 0) {
    r << "(NAT):" << addr.addr;
    return;
  }
  if (resolve) {
    try {
      r << getHostname(addr.addr);
    }
    catch (const AddressException& e) {
      printAddrString(r, addr.addr);
    }
  }
  else {
    printAddrString(r, addr.addr);
  }
  r << ":" << addr.port;
} // getAddrString

std::string Util::getAddrString(const std::string& hostname) throw (AddressException) {
  std::ostringstream out;
  printAddrString(out, hostname);
  return out.str();
} // getAddrString

void Util::printAddrString(std::ostream& out, const std::string& hostname) throw (AddressException) {
  printAddrString(out, getAddr(hostname));
} // printAddrString

std::string Util::getAddrString(int ipaddr) {
  struct sockaddr_in in;
  in.sin_addr.s_addr = ipaddr;
  std::string r(inet_ntoa(in.sin_addr));
  return r; 
} // getAddrString

void Util::printAddrString(std::ostream& out, int ipaddr) {
  struct sockaddr_in in;
  in.sin_addr.s_addr = ipaddr;
  out << inet_ntoa(in.sin_addr);
} // getAddrString

int Util::getHostByName(const std::string& hostname) throw (AddressException) {
  return getAddr(hostname);
} // getHostByName

std::string Util::getHostByAddr(int ipaddr) {
  return getHostname(ipaddr);
} // getHostByAddr

std::string Util::getHostname() {
  static std::string s;
  if (s.empty()) {
    ScopedLock sl(ulock);
    char hostname[NI_MAXHOST];
    ASSERT(gethostname(hostname, sizeof(hostname)) == 0);
    s = hostname;
  }
  return s;
} // getHostname

bool Util::isPrivateAddr(uint32_t a) {
//      10.0.0.0        -   10.255.255.255  (10/8 prefix)
//      172.16.0.0      -   172.31.255.255  (172.16/12 prefix)
//      192.168.0.0     -   192.168.255.255 (192.168/16 prefix)
  static uint32_t s8 = ntohl(inet_addr("10.0.0.1"));
  static uint32_t s12b = ntohl(inet_addr("172.16.0.0"));
  static uint32_t s12e = ntohl(inet_addr("172.31.255.255"));
  static uint32_t s16 = ntohl(inet_addr("192.168.0.0"));

  a = ntohl(a);

  uint32_t t = a & s8;
  if (t == s8) {
    return true;
  }

  t = a & s16;
  if (t == s16) {
    return true;
  }

  t = a;
  if ((t >= s12b) && (t <= s12e)) {
    return true;
  }
  
  return false;
} // isPrivateAddr

bool Util::isHostReachable(const MaceKey& k) {
  static bool isReachableSet = false;
  static bool isReachable = false;

  if (!isReachableSet &&
      params::containsKey(params::MACE_ALL_HOSTS_REACHABLE)) {
    isReachable = params::get<int>(params::MACE_ALL_HOSTS_REACHABLE);
    isReachableSet = true;
  }

  if (isReachableSet && isReachable) {
    return true;
  }
  
  const MaceAddr& ma = k.getMaceAddr();
  if ((ma.proxy != SockUtil::NULL_MSOCKADDR) && (ma.proxy != getMaceAddr().local)) {
    // a proxy is set but we are not the proxy, so we cannot directly reach this host
    return false;
  }
  return true;
} // isHostReachable

std::string Util::getHostname(const MaceKey& k) throw (AddressException) {
  const MaceAddr& ma = k.getMaceAddr();
  uint32_t h = ma.local.addr;
  if (isHostReachable(k)) {
    if ((ma.proxy != SockUtil::NULL_MSOCKADDR) && (ma.proxy != getMaceAddr().local)) {
      // a proxy is set and we are not the proxy, so go to the proxy
      h = ma.proxy.addr;
    }
  }
  else {
    throw UnreachablePrivateAddressException(k.addressString());
  }

  return getHostname(h);
} // getHostname

std::string Util::getHostname(const std::string& ipaddr) throw (AddressException) {
  in_addr_t a = inet_addr(ipaddr.c_str());
  if (a == INADDR_NONE) {
    throw AddressException("getHostname passed bad address: " + ipaddr);
  }

  return Util::getHostname(a);
} // getHostname

std::string Util::getHostname(int ipaddr) {
  typedef mace::LRUCache<int, std::string> ReverseDnsCache;
  static ReverseDnsCache dnsCache(DNS_CACHE_SIZE, DNS_CACHE_TIMEOUT);
  static const bool SUPPRESS_REVERSE_DNS = params::get(params::MACE_SUPPRESS_REVERSE_DNS, true);

  string r;
  ScopedLock sl(ulock);

  if (dnsCache.containsKey(ipaddr)) {
    r = dnsCache.get(ipaddr);
    sl.unlock();
  }
  else {
    sl.unlock();

    char hostname[NI_MAXHOST];
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET; // IPv4
    addr.sin_addr.s_addr = ipaddr;
    
    int err = getnameinfo((struct sockaddr *)&addr, sizeof(addr),
        hostname, sizeof(hostname), NULL, 0,
        SUPPRESS_REVERSE_DNS ? NI_NUMERICHOST : 0);
    
    if (err != 0)
      r = getAddrString(ipaddr);
    else
      r = hostname;
  }

  return r;
} // getHostname

mace::map<int, int> Util::makeMap(int first ...) {
  mace::map<int, int> r;
  va_list ap;
  va_start(ap, first);
  do {
    int x = va_arg(ap, int);
    r[first] = x;
    first = va_arg(ap, int);
  } while (first);
  va_end(ap);

  return r;
} // makeMap

// unsigned Util::randInt(unsigned max, unsigned first ...) {
//   va_list ap;
//   va_start(ap, first);
//   unsigned r = RandomUtil::vrandInt(max, first, ap);
//   va_end(ap);
//   return r;
// }

void Util::nodeSetDiff(const NodeSet& prev, const NodeSet& cur,
		       NodeSet& added, NodeSet& removed) {
  NodeSet::const_iterator ai = cur.begin();
  NodeSet::const_iterator ri = prev.begin();
  while (ai != cur.end() || ri != prev.end()) {
    if (ai == cur.end()) {
      removed.insert(*ri);
      ri++;
    }
    else if (ri == prev.end()) {
      added.insert(*ai);
      ai++;
    }
    else if (*ai == *ri) {
      ai++;
      ri++;
    }
    else if (*ai < *ri) {
      added.insert(*ai);
      ai++;
    }
    else {
      ASSERT(*ri < *ai);
      removed.insert(*ri);
      ri++;
    }
  }
} // nodeSetDiff

mace::string Util::generateContextName( mace::string const& context_type, const uint32_t& context_id) {
  std::ostringstream oss;
  oss << context_type <<"["<<context_id<<"]";
  return oss.str();
}

mace::string Util::extractContextType( mace::string const& context_name ) {
  size_t pos = context_name.find_first_of("[");
  if( pos == std::string::npos ){
    return context_name;
  } else {
    return context_name.substr(0, pos);
  }
}

bool Util::isContextType( mace::string const& context_name, mace::string const& context_type) {
  mace::string ctx_type = Util::extractContextType( context_name );
  if( ctx_type == context_type ){
    return true;
  } else {
    return false;
  }

}

uint32_t Util::extractContextID( mace::string const& context_name) {
  size_t pos = context_name.find("[");
  if( pos == string::npos ){
    return 0;
  }

  mace::string sub_name = context_name.substr(pos+1);
  pos = sub_name.find("]");
  if( pos == string::npos ){
    return 0;
  }

  mace::string id_str = sub_name.substr(0, pos);
  uint32_t ctx_id = (uint32_t)atoi(id_str.c_str());
  return ctx_id;
}
