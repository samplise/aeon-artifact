/* 
 * params.h : part of the Mace toolkit for building distributed systems
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
#ifndef params_h
#define params_h

// #include <stdio.h>
#include <string>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include "massert.h"
//#include "mdeque.h"
#include "m_map.h"
#include "mset.h"
#include "MaceTypes.h"

/*!
 * \file params.h 
 *
 * \brief This file defines the API for interfacing with the Mace parameter class.
 *
 * The Mace parameter class is an alternative (though may be used in addition
 * to getopt.  I personally do not like the getopt style option parsing,
 * because all option handling must be done in main.  Instead, the params::Params class
 * loads parameters in what amounts to a map, then allows code anywhere in Mace
 * to query the parameter values.
 *
 * This file contains the params::Params class (which should not directly be used), a
 * singleton which implements the parameters, several constants which define
 * standard Mace parameters that various libraries and services use, and the
 * params:: interface.
 */

/// This namespace includes the standard parameter strings and methods for accessing them.
namespace params {
  static const std::string MACE_LOCAL_ADDRESS = "MACE_LOCAL_ADDRESS"; ///< Allows to override the detected local address.
  static const std::string MACE_PORT = "MACE_PORT";  ///< Sets the default port for addresses 
  static const std::string MACE_BIND_LOCAL_ADDRESS = "MACE_BIND_LOCAL_ADDRESS"; ///< 1 or 0, sets whether to additionally bind to the local address rather than inaddr any.
  static const std::string MACE_TRANSPORT_DISABLE_TRANSLATION = "MACE_TRANSPORT_DISABLE_TRANSLATION"; ///< 1 or 0, sets whether to have the transport re-write source addresses for hosts using the auto-forwarding transport features.
  static const std::string MACE_ALL_HOSTS_REACHABLE = "MACE_ALL_HOSTS_REACHABLE"; ///< When set allows the transport to skip overhead when not doing NAT
  static const std::string MACE_CERT_FILE = "MACE_CERT_FILE"; ///< Tell the transport where the certificate file is
  static const std::string MACE_PRIVATE_KEY_FILE = "MACE_PRIVATE_KEY_FILE"; ///< Tell the transport where the private key is
  static const std::string MACE_CA_FILE = "MACE_CA_FILE"; ///< Tell the transport where the CA certificate to trust is
  static const std::string MACE_NO_VERIFY_HOSTNAMES = "MACE_NO_VERIFY_HOSTNAMES"; ///< Tell the transport to ignore hostnames in certificates
  static const std::string MACE_PRINT_HOSTNAME_ONLY = "MACE_PRINT_HOSTNAME_ONLY"; ///< In outputting logs, don't print prefix or IP, just hostname
  static const std::string MACE_LOG_AUTO_SELECTORS = "MACE_LOG_AUTO_SELECTORS"; ///< These will be passed to Log::autoAdd
  static const std::string MACE_LOG_TIMESTAMP_HUMAN = "MACE_LOG_TIMESTAMP_HUMAN"; ///< If true, configured logging will use human timestamps
  static const std::string MACE_LOG_LEVEL = "MACE_LOG_LEVEL"; ///< Calls Log::setLevel
  static const std::string MACE_LOG_STRING_TRUNCATE = "MACE_LOG_STRING_TRUNCATE"; ///< Truncate strings longer than this length
  static const std::string MACE_LOG_AUTO_ALL = "MACE_LOG_AUTO_ALL"; ///< Enables all logging via Log::autoAddAll
  static const std::string MACE_LOG_FILE = "MACE_LOG_FILE"; ///< Default file to write configured logs to
  static const std::string MACE_LOG_PATH = "MACE_LOG_PATH"; ///< Default path for log files
  static const std::string MACE_LOG_PREFIX = "MACE_LOG_PREFIX"; ///< Prefix for log files
  static const std::string MACE_LOG_APPEND = "MACE_LOG_APPEND"; ///< Don't truncate MACE_LOG_FILE
  static const std::string MACE_LOG_ASYNC_FLUSH = "MACE_LOG_ASYNC_FLUSH"; ///< Do not automatically flush the log
  static const std::string MACE_LOG_BUFFERS = "MACE_LOG_BUFFERS"; ///< Log everything to memory buffers
  static const std::string MACE_LOG_BUFFER_SIZE = "MACE_LOG_BUFFER_SIZE"; ///< Size of each memory buffer; one per file
  static const std::string MACE_LOG_BIN_BUFFERS = "MACE_LOG_BIN_BUFFERS"; ///< Log binary logs to memory buffers
  static const std::string MACE_LOG_DISABLE_WARNING = "MACE_LOG_DISABLE_WARNING"; ///< During configuration, disable default warning logs
  static const std::string MACE_LOG_AUTO_BINARY = "MACE_LOG_AUTO_BINARY"; ///< Like MACE_LOG_AUTO_SELECTORS, but adds as binary logs
  static const std::string MACE_LOG_SCOPED_TIMES = "MACE_LOG_SCOPED_TIMES";
  static const std::string MACE_LOG_ACCUMULATOR = "MACE_LOG_ACCUMULATOR";
  static const std::string MACE_LOG_LOADMONITOR = "MACE_LOG_LOADMONITOR";
  static const std::string MACE_PROFILE_SCOPED_TIMES = "MACE_PROFILE_SCOPED_TIMES";
  static const std::string MACE_LOAD_MONITOR_FREQUENCY_MS = "MACE_LOAD_MONITOR_FREQUENCY_MS"; 
  static const std::string MACE_SWAP_MONITOR_FREQUENCY_MS = "MACE_SWAP_MONITOR_FREQUENCY_MS";
  static const std::string MACE_VMRSSLIMIT_FREQUENCY_MS = "MACE_VMRSSLIMIT_FREQUENCY_MS"; 
  static const std::string MACE_TRACE_FILE = "MACE_TRACE_FILE"; 
  static const std::string MACE_BIN_LOG_FILE = "MACE_BIN_LOG_FILE"; ///< File for writing the binary logs
  static const std::string MACE_BOOTSTRAP_PEERS = "MACE_BOOTSTRAP_PEERS"; ///< sets autoBootstrap to false, but will return from params::getBootstrapPeers
  static const std::string MACE_AUTO_BOOTSTRAP_PEERS = "MACE_AUTO_BOOTSTRAP_PEERS"; ///< Will return for getBootstrapPeers, and set autoBootstrap to true.
  static const std::string MACE_ADDRESS_ALLOW_LOOPBACK = "MACE_ADDRESS_ALLOW_LOOPBACK"; ///< Enable the transport to use the loopback as a local address.  Disabled by default to prevent bad address detection.
  static const std::string MACE_WARN_LOOPBACK = "MACE_WARN_LOOPBACK";
  static const std::string MACE_SUPPRESS_REVERSE_DNS = "MACE_SUPPRESS_REVERSE_DNS"; ///< Reverse DNS mapping for IP addresses.
  static const std::string MACE_PROPERTY_CHECK_SERVER= "MACE_PROPERTY_CHECK_SERVER";

  typedef mace::map<std::string, std::string> StringMap;

/// Do not use directly.  Use \c params:: methods instead.
class Params {
public:
  
  bool containsKey(const std::string& key) {
    return data.containsKey(key);
  }

  void set(const std::string& key, const std::string& value) {
    data[key] = value;
  }

  void setAppend(const std::string& key, const std::string& value) {
    data[key].append(value);
  }

  void setAppendNewline(const std::string& key, const std::string& value) {
    if (data.containsKey(key)) {
      data[key].append("\n");
    }
    data[key].append(value);
  }

  template <typename T>
  T get(const std::string& key) {
    if (watch) {
      touched.insert(key);
    }
    StringMap::const_iterator i = data.find(key);
    if (i == data.end()) {
      std::cerr << "no param defined for " << key << std::endl;
      ABORT("undefined param");
    }
    try {
      return boost::lexical_cast<T>(i->second);
    }
    catch (const boost::bad_lexical_cast& e) {
      std::cerr << "bad cast reading param " << key << ": " << " "
		<< i->second << std::endl;
      throw e;
    }
  }

  template <typename T>
  T get(const std::string& key, const T& def) {
    if (watch) {
      touched.insert(key);
    }
    StringMap::const_iterator i = data.find(key);
    if (i != data.end()) {
      try {
	return boost::lexical_cast<T>(i->second);
      }
      catch (const boost::bad_lexical_cast& e) {
	std::cerr << "bad cast reading param " << key << ": " << " "
		  << i->second << std::endl;
	throw e;
      }
    }
    else {
      return def;
    }
  }

  void erase(const std::string& key) {
    data.erase(key);
  }

  int loadparams(int& argc, char**& argv, bool requireFile = false);

  int loadfile(const char* filename, bool allRequired = false);

  void addRequired(const std::string& name, const std::string& desc = "");
    
  int print(FILE* ostream); // Prints the parameters to the FILE specified.
  int print(std::ostream& os);
  void log();
  
  void getBootstrapPeers(const mace::MaceKey& laddr, NodeSet& peers, bool& autoBootstrap);

  void printUnusedParams(FILE* ostream);

  void watchParams(bool w) { watch = w; }

  const StringMap& getParams() const {
    return data;
  }

  void setParams(const StringMap& p) { data = p; }

  Params();

  static Params* Instance() { 
    if (_inst == NULL) {
      _inst = new Params();
    }
    return _inst;
  }
  
private:  
  static Params *_inst;

  bool watch;
  StringMap data;
  StringMap required;
  typedef mace::set<std::string> StringSet;
  StringSet touched;

  std::string trim(const std::string& s);
  void verifyRequired();

}; // class Params

  inline const StringMap& getParams() {
    return Params::Instance()->getParams();
  }

  inline void setParams(const StringMap& p) {
    Params::Instance()->setParams(p);
  }

  inline void watchParams(bool t) {
    Params::Instance()->watchParams(t);
  }

  template <typename T> inline T get(const std::string& key) {
    return Params::Instance()->get<T>(key);
  }

  template <typename T> inline T get(const std::string& key, const T& def) {
    return Params::Instance()->get<T>(key, def);
  }

  inline bool containsKey(const std::string& key) {
    return Params::Instance()->containsKey(key);
  }

  template <> inline MaceKey get<MaceKey>(const std::string& key) {
    std::string s = get<std::string>(key);
    return MaceKey(s);
  }

  template <> inline MaceKey get<MaceKey>(const std::string& key, const MaceKey& def) {
    if (!containsKey(key)) {
      return def;
    }
    return get<MaceKey>(key);
  }

  template <typename T> inline mace::deque<T> getList(const std::string& key) {
    mace::deque<T, mace::SoftState> r;
    if (containsKey(key)) {
      std::string s = get<std::string>(key);
      mace::deque<std::string, mace::SoftState> l;
      std::istringstream in(s);
      while (!in.eof()) {
	std::string n;
	in >> n;
	l.push_back(n);
      }
      for (size_t i = 0; i < l.size(); i++) {
	r.push_back(boost::lexical_cast<T>(l[i]));
      }
    }
    return r;
  }

  template <> inline NodeSet get<NodeSet>(const std::string& key) {
    std::string s = get<std::string>(key);
    NodeSet ns;
    std::istringstream in(s);
    while (!in.eof()) {
      std::string n;
      in >> n;
      ns.insert(MaceKey(n));
    }
    return ns;
  }

  template <> inline NodeSet get<NodeSet>(const std::string& key, const NodeSet& def) {
    if (!containsKey(key)) { 
      return def;
    }
    return get<NodeSet>(key);
  }

  inline void set(const std::string& key, const std::string& value) {
    Params::Instance()->set(key, value);
  }

  inline void setAppend(const std::string& key, const std::string& value) {
    Params::Instance()->setAppend(key, value);
  }

  inline void setAppendNewline(const std::string& key,
			       const std::string& value) {
    Params::Instance()->setAppendNewline(key, value);
  }

  inline void erase(const std::string& key) {
    Params::Instance()->erase(key);
  }

  /** 
   * \brief Loads the parameters as though from the command line.  
   *
   * Follows this method:
   * 1 - Ignores argv[0], as this is the program executable
   * 2 - If no option file is specified, defaults are read in from params.default
   * 3 - If argv[1] is a file -- parameters are read in from it
   * 4 - Remaining arguments are read and parsed as command-line [potentially] overriding values.
   * 5 - Returns the number of parameters set.
   *
   * @param argc the number of arguments in argv
   * @param argv the arguments
   * @param requireFile whether there must be a file parameter
   * @return the number of parameters set
   */
  inline int loadparams(int& argc, char**& argv, bool requireFile = false) {
    return Params::Instance()->loadparams(argc, argv, requireFile);
  }
  /**
   * Files are read in as follows:
   * # is the comment delimiter.  Lines beginning with it are ignored.  
   *   The remainder of any line containing it, unless a quoted string are ignored.
   * parameters are set by parameter = value, or parameter = "value"
   *
   * @param filename the file to read parameters from 
   * @param allRequired whether all required parameters must be in the file
   * @return the number of parameters set
   */
  inline int loadfile(const char* filename, bool allRequired = false) {
    return Params::Instance()->loadfile(filename, allRequired);
  }
  /// Adds a required parameter of name \c name and description \c desc
  inline void addRequired(const std::string& name, const std::string& desc = "") {
    Params::Instance()->addRequired(name, desc);
  }
  /// Prints all the parameters to a \c FILE* such as \c stdout
  inline int print(FILE* ostream) { // Prints the parameters to the FILE specified.
    return Params::Instance()->print(ostream);
  }
  /// Prints all the parameters to a \c ostream such as \c cout
  inline int print(std::ostream& os) {
    return Params::Instance()->print(os);
  }
  
  inline void log() {
    Params::Instance()->log();
  }
  /**
   * \brief Returns either MACE_AUTO_BOOTSTRAP_PEERS or MACE_BOOTSTRAP_PEERS, parsed
   * as a set of IPV4 nodes.
   *
   * \warning Throws an exception if neither parameter is set.
   *
   * @param laddr the local address, excluded from the parsed nodes
   * @param peers the set to store the nodes in
   * @param autoBootstrap set to true on return if MACE_AUTO_BOOTSTRAP_PEERS was set.
   */
  inline void getBootstrapPeers(const mace::MaceKey& laddr, NodeSet& peers, bool& autoBootstrap) {
    Params::Instance()->getBootstrapPeers(laddr, peers, autoBootstrap);
  }
  inline void printUnusedParams(FILE* ostream) {
    Params::Instance()->printUnusedParams(ostream);
  }

} //namespace params

#endif
