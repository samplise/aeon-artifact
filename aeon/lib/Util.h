/* 
 * Util.h : part of the Mace toolkit for building distributed systems
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
#ifndef __UTIL_H
#define __UTIL_H

// #include <pthread.h>
// #include <sys/time.h>
#include <errno.h>
#include <string>

#include "MaceTypes.h"
#include "Exception.h"

#include "mstring.h"
// #include "RandomUtil.h"
// #include "TimeUtil.h"

/**
 * \file Util.h
 * \brief defines the Util utility methods
 */

/**
 * \addtogroup Utils
 * \brief Mace contains a variety of Util classes.  Each contains a set of
 * static public methods to help with a variety of tasks.  The functions are
 * broken up into categories, each implemented in a separate class and file.
 * @{
 */

/**
 * \brief a set of basic non-categories utilities.
 *
 * Most of these have to do with DNS, MaceAddr, SockAddr, and error strings
 */
class Util {
public:
  static const uint16_t DEFAULT_MACE_PORT = 5377; ///< The default port if none is provided
  static const bool REPLAY; ///< UNUSED.  Designed for a flag indicating replay.

public:
  static const MaceAddr& getMaceAddr() throw (AddressException); ///< method to get the local MaceAddr
  static int getAddr() throw (AddressException); ///< method to get the local ipaddress as a packed int
  // this will return the packed ip address of the hostname, which
  // could also be an ip address in dotted notation
  static int getAddr(const std::string& hostname) throw (AddressException); ///< method to convert a string (ip or hostname) into a packed int IP.
  static uint16_t getPort(); ///< return the default port (params::MACE_PORT, the port from params::MACE_LOCAL_ADDRESS, or the DEFAULT_MACE_PORT otherwise)
  static int getHostByName(const std::string& hostname) throw (AddressException); ///< equivalent to getAddr(const std::string&)
  static MaceAddr getMaceAddr(const std::string& hostname) throw (AddressException); ///< translate the address string into a MaceAddr
  static std::string getHostname(); ///< return the local hostname
  /**
   * Private IPs are one of
   * - 10.0.0.0        -   10.255.255.255  (10/8 prefix)
   * - 172.16.0.0      -   172.31.255.255  (172.16/12 prefix)
   * - 192.168.0.0     -   192.168.255.255 (192.168/16 prefix)
   */
  static bool isPrivateAddr(uint32_t a); ///< takes a packed int, determines if it is a private IP.
  static bool isHostReachable(const MaceKey& k); ///< either params::MACE_ALL_HOSTS_REACHABLE, or address has either a valid proxy or local address
  /// returns the hostname of the key if the host can be directly routed to.  
  /**
   * throws an UnreachablePrivateAddressException if the
   * host can only be reached through the proxy address
   */
  static std::string getHostname(const MaceKey& k) throw (AddressException);
  static std::string getHostname(const std::string& ipaddr) throw (AddressException); ///< return the hostname of the IP address
  static std::string getHostname(int ipaddr); ///< return the hostname of the IP address
  static std::string getHostByAddr(int ipaddr); ///< equivalent to getHostname(int)
  static std::string getAddrString(int ipaddr); ///< returns the IP address as a string
  static std::string getAddrString(const std::string& hostname) throw (AddressException); ///< returns the IP address as a string
  /**
   * Address string format is one of
   * - IP
   * - IP:port
   * - IP:port/IP:port
   * - (NAT):id
   * - (NAT):id/IP:port
   */
  static std::string getAddrString(const MaceAddr& addr, bool resolve = true); ///< prints the address string of the MaceAddr, optionally use DNS
  /**
   * Address string format is one of
   * - IP
   * - IP:port
   * - IP:port/IP:port
   * - (NAT):id
   * - (NAT):id/IP:port
   */
  static std::string getAddrString(const SockAddr& addr, bool resolve = true); ///< prints the address string of the SockAddr, optionally use DNS
  /**
   * Address string format is one of
   * - IP
   * - IP:port
   * - IP:port/IP:port
   * - (NAT):id
   * - (NAT):id/IP:port
   */
  static void printAddrString(std::ostream& out, const MaceAddr& addr, bool resolve = true); ///< prints the address string to an ostream optionally using DNS
  /**
   * Address string format is one of
   * - IP
   * - IP:port
   * - IP:port/IP:port
   * - (NAT):id
   * - (NAT):id/IP:port
   */
  static void printAddrString(std::ostream& out, const SockAddr& addr, bool resolve = true); ///< prints the address string to an ostream optionally using DNS
  /**
   * Address string format is one of
   * - IP
   * - IP:port
   * - IP:port/IP:port
   * - (NAT):id
   * - (NAT):id/IP:port
   */
  static void printAddrString(std::ostream& out, const std::string& hostname) throw (AddressException); ///< prints the address string to an ostream optionally using DNS
  /**
   * Address string format is one of
   * - IP
   * - IP:port
   * - IP:port/IP:port
   * - (NAT):id
   * - (NAT):id/IP:port
   */
  static void printAddrString(std::ostream& out, int ipaddr); ///< prints the address string to an ostream optionally using DNS

  static std::string getErrorString(int err); ///< return an error string for an error code (a la strerror())
  static std::string getSslErrorString(); ///< return the SSL last error string

  static mace::map<int, int> makeMap(int first ...);

  static void nodeSetDiff(const NodeSet& prev, const NodeSet& cur,
			  NodeSet& added, NodeSet& removed);

  static mace::string generateContextName( mace::string const& context_type, const uint32_t& context_id);
  static mace::string extractContextType( mace::string const& context_name );
  static bool isContextType( mace::string const& context_name, mace::string const& context_type);
  static uint32_t extractContextID( mace::string const& context_name);

private:
  static const size_t DNS_CACHE_TIMEOUT = 10 * 60;
  static const unsigned DNS_CACHE_SIZE = 64;
  static uint16_t baseport;
};

/** @} */

#endif // __UTIL_H
