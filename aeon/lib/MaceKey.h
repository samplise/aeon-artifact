/* 
 * MaceKey.h : part of the Mace toolkit for building distributed systems
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
/**
 * \file MaceKey.h
 * \brief Included in this file are the mace::MaceKey classes.
 *
 * The basic MaceKey interface is specified by the class MaceKey_interface.
 * MaceKey and all its sub-helpers provide that interface.  Each sub-helper is
 * also mapped to a address family, as defined in MaceBasics.h  MaceKey's also throw
 * 3 exceptions:
 *
 * - InvalidMaceKeyException: thrown when a function is called on a 
 *                              MaceKey which does not support the operation
 * - IndexOutOfBoundsException: thrown when trying to index a digit which is out of bounds of the
 *                                MaceKey
 * - DigitBaseException: thrown when a digit base is offered which does not divide 32.
 *
 * To construct most MaceKey's, you can pass in a type (ipv4, sha32, sha160, or
 * string_key), and a string.  The string will be interpreted as appropriate
 * for that MaceKey sub-type.
 *
 * e.g. 
 * \code
 * using mace::MaceKey;
 * MaceKey a = MaceKey(ipv4, "10.0.0.1:12345");
 * MaceKey b = MaceKey(ipv4, "www.google.com");
 * MaceKey c = MaceKey(sha160, "Some string to hash to produce the address");
 * MaceKey d = Macekey(string_key, "a string which serves as an address");
 * \endcode
 *
 * Additionally there is a MaceKey constructor that parses a string with a family prefix, and constructs an appropriate MaceKey:
 * \code
 * mace::MaceKey e = mace::MaceKey("IPV4/www.ibm.com:12345");
 * \endcode
 *
 * There are also constructors which do more specific constructions based on the MaceKey subtype.
 * 
 * Implementation Note: The helper classes are all privately enclosed in
 * MaceKey, to prevent outside code from ever creating or referencing them.
 * Before adding new MaceKey helper classes, take a good look at this code.
 * You'll have to add a new address family, both here and in MaceBasics.h, and
 * there are a number of places code should be added.  In particular, you'll
 * want to note the bitarr_MaceKey, MaceKey_exception, and intarr_MaceKey
 * classes as they may be helpful.
 */

#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include "m_net.h"
#include <iomanip>
#include <cassert>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <string>

// #include "mstring.h"
// #include "mpair.h"
#include "Serializable.h"
#include "Exception.h"
#include "HashUtil.h"
#include "Log.h"
#include "MaceBasics.h"
#include "Collections.h"


#ifndef __MACEKEY_H
#define __MACEKEY_H

#define MACE_KEY_USE_SHARED_PTR

namespace mace {
/**
 * \brief Helper class to represent an IP and port pair
 *
 * Note, for NAT addresses, a sockaddr may have port=0 and addr=id, where the
 * id is the NAT id to compare against for other sockets.  A null SockAddr 
 * will have port=0 and addr=INADDR_NONE.
 */
class SockAddr : public mace::PrintPrintable, public mace::Serializable {
public:
  SockAddr() : addr(INADDR_NONE), port(0) { }
  SockAddr(const SockAddr& s) : addr(s.addr), port(s.port) { }
  SockAddr(uint32_t a, uint16_t p) :  addr(a), port(p) { }
  void print(std::ostream& out) const;
  void printNode(PrintNode& pr, const std::string& name) const;
  void sqlize(LogNode* node) const {
    int index = node->simpleCreate("TEXT", node);
    std::ostringstream out;
    
    out << node->logId << "\t" << index << "\t";
    print(out);
    out << std::endl;
    ASSERT(fwrite(out.str().c_str(), out.str().size(), 1, node->file) > 0);
  }
  void serialize(std::string& s) const {
    s.append((char*)&addr, sizeof(addr));
    mace::serialize(s, &port);
  } // serialize
  int deserialize(std::istream& in) throw (mace::SerializationException) {
    in.read((char*)&addr, sizeof(addr));
    mace::deserialize(in, &port);
    return 6;
  } // deserialize
  bool operator==(const SockAddr& other) const {
    return ((other.addr == addr) && (other.port == port));
  }
  bool operator!=(const SockAddr& other) const {
    return (!(*this == other));
  }
  bool operator<(const SockAddr& other) const {
    if (addr < other.addr) {
      return true;
    }
    else if (addr == other.addr) {
      return port < other.port;
    }
    else {
      return false;
    }
  }
  /// Implies a connection cannot be initiated to this address.  True when non-null and port=0
  bool isUnroutable() const {
    return !isNull() && port == 0;
  }
  /// Implies it does not refer to an actual socket addr.  No port or ip.
  bool isNull() const;

public:
  uint32_t addr; ///< The IP address as a packed int (network order!)
  uint16_t port; ///< A TCP or UDP port port
}; // SockAddr

/**
 * \brief Helper class to represent Mace addresses.
 *
 * Mace addresses are allowed to contain both a proxy address and a local
 * address.  If there is no proxy address, the local address is where
 * connections are initated to.  Proxy addresses may refer either to port
 * forwarding, or to a TcpTranpsort which will forward traffic as an
 * intermediate.  If two nodes share the same proxy address, and their local
 * address is routable, then the assumption is that they can connect directly
 * to each other.
 */
class MaceAddr : public mace::PrintPrintable, public mace::Serializable {
public:
  MaceAddr() : local(SockAddr()), proxy(SockAddr()) { }
  MaceAddr(const MaceAddr& s) : local(s.local), proxy(s.proxy) { }
  MaceAddr(const SockAddr& l, const SockAddr& p) : local(l), proxy(p) { }
  void print(std::ostream& out) const;
  void printNode(PrintNode& pr, const std::string& name) const;
  void sqlize(LogNode* node) const {
    // TODO: sqlize proxy addr?
    local.sqlize(node);
  }
  void serialize(std::string& s) const {
    local.serialize(s);
    proxy.serialize(s);
  } // serialize
  int deserialize(std::istream& in) throw (mace::SerializationException) {
    local.deserialize(in);
    proxy.deserialize(in);
    return 12; //CK: Warning -- this will be wrong if sock addr or mace addr changes.  Changing to constant because this is used so often.
  } // deserialize
  bool operator==(const MaceAddr& other) const {
    return ((other.local == local) && (other.proxy == proxy));
  }
  bool operator!=(const MaceAddr& other) const {
    return (!(*this == other));
  }
  bool operator<(const MaceAddr& other) const {
    if (local < other.local) {
      return true;
    }
    else if (local == other.local) {
      return proxy < other.proxy;
    }
    else {
      return false;
    }
  }
  /// returns true if the local address is unrouteable.
  bool isUnroutable() const {
    return local.isUnroutable();
  }
  bool isNull() const; ///< check to see if the MaceAddr is null

public:
  SockAddr local; ///< the local (normal) address for a node
  SockAddr proxy; ///< the proxy (optional) address for a node, where to open sockets
}; // MaceAddr

} // namespace mace

#ifdef __MACE_USE_UNORDERED__

namespace std {
namespace tr1 {

template<> struct hash<mace::SockAddr> {
  uint32_t operator()(const mace::SockAddr& x) const {
    return hash<uint32_t>()(x.addr * x.port);
  }
};

template<> struct hash<mace::MaceAddr> {
  uint32_t operator()(const mace::MaceAddr& x) const {
    return hash<uint32_t>()((x.local.addr * x.local.port)^
			    (x.proxy.addr * x.proxy.port));
  }
};

} 
}

#else

namespace __gnu_cxx {

template<> struct hash<mace::SockAddr> {
  uint32_t operator()(const mace::SockAddr& x) const {
    return hash<uint32_t>()(x.addr * x.port);
  }
};

template<> struct hash<mace::MaceAddr> {
  uint32_t operator()(const mace::MaceAddr& x) const {
    return hash<uint32_t>()((x.local.addr * x.local.port)^
			    (x.proxy.addr * x.proxy.port));
  }
};

} // namespace __gnu_cxx

#endif

namespace mace {

class InvalidMaceKeyException : public SerializationException {
public:
  InvalidMaceKeyException(const std::string& m) : SerializationException(m) { }
  virtual ~InvalidMaceKeyException() throw() {}
  void rethrow() const { throw *this; }
};

class IndexOutOfBoundsException : public Exception {
public:
  IndexOutOfBoundsException(const std::string& m) : Exception(m) { }
  virtual ~IndexOutOfBoundsException() throw() {}
  void rethrow() const { throw *this; }
};

class DigitBaseException : public Exception {
public:
  DigitBaseException(const std::string& m) : Exception(m) { }
  virtual ~DigitBaseException() throw() {}
  void rethrow() const { throw *this; }
};

class MaceKey;

/// The generic interface for MaceKey and its helper classes.
class MaceKey_interface : public Serializable, virtual public Printable {
    friend class MaceKey;
  public:
    //common: all should implement
    virtual bool isUnroutable() const = 0; ///< Returns true if you cannot open a socket to the address
    virtual bool isNullAddress() const = 0; ///< Returns true if the address is "NULL"
    virtual std::string toHex() const = 0; ///< Returns the address in a hexidecimal string format
    //     virtual std::string toString() const = 0;
    //     virtual void print(std::ostream&) const = 0;
    virtual std::string addressString() const = 0; ///< Returns the string in a canonical string format (IP address not hostname)
    virtual bool isBitArrMaceKey() const = 0; ///< Return true if the address family is a bitstring family, currently SHA32 or SHA160
    virtual size_t hashOf() const = 0; ///< return an integer hash value for the MaceKey
    virtual bool operator==(const MaceKey& right) const = 0; 
    virtual bool operator<(const MaceKey& right) const = 0;

    //ipv4
    virtual const MaceAddr& getMaceAddr() const = 0; ///< If an IPV4 MaceKey, returns the MaceAddr.  Otherwise throws an exception.
    
    //bitarr: e.g. sha160 and sha32
    virtual int bitLength() const = 0; ///< If a BitArr MaceKey, returns the length in bits, otherwise throws an exception.
    // digitBits are the number of bits per digit -- commonly 4
    // dititBits > 32 will produce an exception (?)
    // digitBits which don't divide 32 will produce an exception (?)
    virtual uint32_t getNthDigit(int position, uint32_t digitBits = 4) const = 0; ///< Returns the \a digit of length \c digitBits at \c position
    virtual void setNthDigit(uint32_t value, int position, uint32_t digitBits = 4) = 0; ///< Sets the \a digit at \c position of length \c digitBits to \c value
    virtual int sharedPrefixLength(const MaceKey& key, uint32_t digitBits = 4) const = 0; ///< Returns the number of digits of length \c digitBits in common from the beginning of \c key
  protected:
    virtual uint32_t getNthDigitSafe(int position, uint32_t digitBits) const = 0; ///< \internal bypasses safety checks of getNthDigit().  Caller promises helper is not null and that no exceptions will be thrown
};

/// The base class of Mace addresses.
class MaceKey : public MaceKey_interface, virtual public PrintPrintable {
  public:
    /** \brief An easy reference to \e one null MaceKey.  
     *
     * This is not the only "null" MaceKey.  DO NOT COMPARE TO THIS.  Instead, call MaceKey::isNullAddress().
     */
    static const MaceKey null; 
  private:
    /// Internal.  Adds a clone method for copying helper pointers.
    class MaceKey_interface_helper : public MaceKey_interface {
      public:
        virtual MaceKey_interface_helper* clone() const = 0; //memory to be created by clone function -- needs to be deleted.
    };
#ifndef MACE_KEY_USE_SHARED_PTR
    typedef MaceKey_interface_helper* HelperPtr;
#else
    typedef boost::shared_ptr<MaceKey_interface_helper> HelperPtr;
#endif
    HelperPtr helper; ///< Pointer to a sub-type of MaceKey
    int8_t address_family; ///< The address family of the subtype of MaceKey.  Should be 0 if helper is NULL

  protected:
    uint32_t getNthDigitSafe(int position, uint32_t digitBits) const { return helper->getNthDigitSafe(position, digitBits); }

  public:
    static bool getHostnameOnly();

    int8_t addressFamily() const { return address_family; }
    bool isUnroutable() const { return (helper == NULL ? true : helper->isUnroutable()); }
    bool isNullAddress() const { return (helper==NULL?true:helper->isNullAddress()); }
    std::string toHex() const { 
      std::string hexStr = Log::toHex(std::string((char*)&address_family, sizeof(int8_t )));
      if(helper != NULL) { hexStr += helper->toHex(); }
      return hexStr;
    }
    void print(std::ostream& out) const {
      static bool hostnameOnly = getHostnameOnly();
      if (helper != NULL) {
	if (!hostnameOnly) {
          out << addressFamilyName(address_family) << "/";
	}
        out << (*helper);
      }
      else {
        out << addressFamilyName(address_family) << "/";
      }
    }
  void printNode(PrintNode& pr, const std::string& name) const {
    pr.addChild(PrintNode(name, std::string("MaceKey::") + addressFamilyName(address_family),
			  helper ? helper->toString() : string()));
  }
    std::string addressString() const {
      return addressString(true);
    }
    std::string addressString(bool printFamily) const {
      if (printFamily) {
        std::ostringstream out;
        out << addressFamilyName(address_family) << "/";
        if (helper != 0) {
          out << helper->addressString();
        }
        return out.str();
      }
      if (helper == 0) {
	throw InvalidMaceKeyException("addressString() called with no helper defined");
      }
      return helper->addressString();
    }
    bool isBitArrMaceKey() const { return (helper==NULL?false:helper->isBitArrMaceKey()); }
    size_t hashOf() const { return (helper==NULL?0:helper->hashOf()); }

    //ipv4
    const MaceAddr& getMaceAddr() const throw(InvalidMaceKeyException) { 
      if(helper==NULL) { 
        throw(InvalidMaceKeyException("No Helper Defined for getMaceAddr")); 
      } else { 
        return helper->getMaceAddr(); 
      } 
    }

    //bitarr
    int bitLength() const throw(InvalidMaceKeyException) { if(helper==NULL) { throw(InvalidMaceKeyException("No Helper Defined")); } else { return helper->bitLength(); } }
    uint32_t getNthDigit(int position, uint32_t digitBits = 4) const throw(Exception) 
      { if(helper==NULL) { throw(InvalidMaceKeyException("No helper defined")); } else { return helper->getNthDigit(position, digitBits); } }
    void setNthDigit(uint32_t digit, int position, uint32_t digitBits = 4) throw(Exception) { 
      if(helper==NULL) { 
        throw(InvalidMaceKeyException("No helper defined")); 
      } else { 
#ifdef MACE_KEY_USE_SHARED_PTR
        if(!helper.unique()) {
          helper = HelperPtr(helper->clone());
        }
#endif
        helper->setNthDigit(digit, position, digitBits); 
      } 
    }
    int sharedPrefixLength(const MaceKey& key, uint32_t digitBits = 4) const throw(InvalidMaceKeyException)
      { if(helper==NULL) { throw(InvalidMaceKeyException("No helper defined")); } else { return helper->sharedPrefixLength(key, digitBits); } }

    virtual void sqlize(LogNode* node) const {
      if (address_family != 0 && helper != NULL) {
	helper->sqlize(node);
      }
      else {
	SockAddr addr;
	addr.sqlize(node);
      }
    }
    
    virtual void serialize(std::string& str) const {
      mace::serialize(str, &address_family);
      if(address_family != 0 && helper != NULL) {
        helper->serialize(str);
      }
    }
    virtual int deserialize(std::istream& in) throw(SerializationException) {
      int count = 0;
      count += mace::deserialize(in, &address_family);
#ifndef MACE_KEY_USE_SHARED_PTR
      delete helper;
#endif
      helper = HelperPtr();
      switch(address_family) {
      case UNDEFINED_ADDRESS: return count;
      case IPV4: helper = HelperPtr(new ipv4_MaceKey()); break;
      case SHA160: helper = HelperPtr(new sha160_MaceKey()); break;
      case SHA32: helper = HelperPtr(new sha32_MaceKey()); break;
      case STRING_ADDRESS: helper = HelperPtr(new string_MaceKey()); break;
      case CONTEXTNODE: helper = HelperPtr(new ctxnode_MaceKey()); break;
      case VNODE: helper = HelperPtr(new vnode_MaceKey()); break;
      default: throw(InvalidMaceKeyException("Deserializing bad address family "+boost::lexical_cast<std::string>(address_family)+"!"));
      }
      count += helper->deserialize(in); 
      return count;
    }

  virtual void serializeXML_RPC(std::string& str) const throw(SerializationException) {
    StringMap m;
    //     m["addr"] = this->addressString();
    switch (address_family) {
    case IPV4:
      m["type"] = "ipv4";
      break;
    case CONTEXTNODE:
      m["type"] = "ctxnode";
      break;
    case VNODE:
      m["type"] = "vnode";
      break;
    case STRING_ADDRESS:
      m["type"] = "str";
      break;
    default: throw(InvalidMaceKeyException("Serializing bad address family "+boost::lexical_cast<std::string>(address_family)+"!"));
    }
    mace::string s;
    if (helper) {
    helper->serializeXML_RPC(s);
    }
    m["addr"] = s;
    m.serializeXML_RPC(str);
  }

  virtual int deserializeXML_RPC(std::istream& in) throw(SerializationException) {
    StringMap m;
    int r = m.deserializeXML_RPC(in);
    const StringMap::iterator i = m.find("type");
    if (i == m.end()) {
      throw InvalidMaceKeyException("deserialized MaceKey without type");
    }
    const StringMap::iterator ai = m.find("addr");
    if (ai == m.end()) {
      throw InvalidMaceKeyException("deserialized MaceKey without address");
    }
    //     *this = MaceKey(ai->second);
    const mace::string& t = i->second;
    if (t == "ipv4") {
      address_family = IPV4;
      helper = HelperPtr(new ipv4_MaceKey());
    }
    else if (t == "ctxnode") {
      address_family = CONTEXTNODE;
      helper = HelperPtr(new ctxnode_MaceKey());
    }
    else if (t == "vnode") {
      address_family = VNODE;
      helper = HelperPtr(new vnode_MaceKey());
    }
    else if (t == "str") {
      address_family = STRING_ADDRESS;
      helper = HelperPtr(new string_MaceKey());
    }      
    else {
      throw InvalidMaceKeyException("deserialized MaceKey with unknown type: " + t);
    }
    std::istringstream is(ai->second);
    helper->deserializeXML_RPC(is);
    return r;
  }


    MaceKey() : helper(HelperPtr()), address_family(UNDEFINED_ADDRESS) {}
    virtual ~MaceKey() { 
#ifndef MACE_KEY_USE_SHARED_PTR
      delete helper; 
#endif
    }

    //operators
    bool operator==(const MaceKey& right) const {
      if(helper == right.helper) { return true; }
      if(addressFamily() != right.addressFamily()) { return false; }
      else if(addressFamily() == 0) { return true; } //XXX: Should this be true or false?  cf. in SQL, NULL != NULL
      else { try {return (*helper) == right;} catch (InvalidMaceKeyException e) { ASSERT(false); return false; } }
    }

    bool operator!=(const MaceKey& right) const {
      return !((*this)==right);
    }

    bool operator<(const MaceKey& right) const {
      if(addressFamily() != right.addressFamily()) { return addressFamily() < right.addressFamily(); }
      else if(addressFamily() == 0) { return false; }
      else { try {return (*helper) < right;} catch (InvalidMaceKeyException e) { ASSERT(false); return false; } }
    }

    bool operator<=(const MaceKey& right) const {
      return ( (*this) < right || (*this) == right);
    }
    bool operator>(const MaceKey& right) const {
      return !( (*this) <= right );
    }
    bool operator>=(const MaceKey& right) const {
      return !( (*this) < right );
    }

    //An assignment operator
    MaceKey& operator=(const MaceKey& right) {
#ifndef MACE_KEY_USE_SHARED_PTR
      if(this != &right) {
        delete helper;
        if(right.helper == NULL)
        {
          helper = NULL;
          address_family = 0;
        }
        else
        {
          helper = right.helper->clone();
          address_family = right.address_family;
        }
      }
#else
      helper = right.helper;
      address_family = right.address_family;
#endif
      return *this;
    }

    /// Standard copy constructor
    MaceKey(const MaceKey& right) : helper(), address_family(right.address_family) {
#ifndef MACE_KEY_USE_SHARED_PTR
      if(right.helper != NULL)
      {
	helper = right.helper->clone();
      }
#else
      helper = right.helper;
#endif
    }

    /// Returns a string from an address family number
    static const char* addressFamilyName(int addressFamily) {
      switch(addressFamily) {
        case UNDEFINED_ADDRESS: return "NONE";
        case IPV4: return "IPV4";
        case CONTEXTNODE: return "CONTEXTNODE";
        case VNODE: return "VNODE";
        case SHA160: return "SHA160";
        case SHA32: return "SHA32";
        case STRING_ADDRESS: return "STRING";
        default: return "HUH?";
      }
    }

    /// Returns a MaceKey address prefix based on the addressFamily
    static const char* addressFamilyVar(int addressFamily) {
      switch(addressFamily) {
        case UNDEFINED_ADDRESS: return "UNDEFINED_ADDRESS";
        case IPV4: return "IPV4";
        case CONTEXTNODE: return "CONTEXTNODE";
        case VNODE: return "VNODE";
        case SHA160: return "SHA160";
        case SHA32: return "SHA32";
        case STRING_ADDRESS: return "STRING_ADDRESS";
        default: return "HUH?";
      }
    }

  private:

    /// Internal: provides a class which throws an exception for non-common functions
    class MaceKey_exception : public MaceKey_interface_helper {
      protected:
        virtual uint32_t getNthDigitSafe(int position, uint32_t digitBits) const { abort(); }
      public: 
        //ipv4
        virtual bool isUnroutable() const throw(InvalidMaceKeyException) { throw(InvalidMaceKeyException("Not Implemented For This MaceKey Family")); }
        virtual const MaceAddr& getMaceAddr() const throw(InvalidMaceKeyException) { throw(InvalidMaceKeyException("Not Implemented For This MaceKey Family")); }

        //bitarr: e.g. sha160 and sha32
        virtual int bitLength() const throw(InvalidMaceKeyException) { throw(InvalidMaceKeyException("Not Implemented For This MaceKey Family")); }
        // digitBits are the number of bits per digit -- commonly 4
        // dititBits > 32 will produce an exception (?)
        // digitBits which don't divide 32 will produce an exception (?)
        virtual uint32_t getNthDigit(int position, uint32_t digitBits = 4) const throw(Exception) { throw(InvalidMaceKeyException("Not Implemented For This MaceKey Family")); }
        virtual void setNthDigit(uint32_t digit, int position, uint32_t digitBits = 4) throw(Exception) { throw(InvalidMaceKeyException("Not Implemented For This MaceKey Family")); }
        virtual int sharedPrefixLength(const MaceKey& key, uint32_t digitBits = 4) const throw(Exception) { throw(InvalidMaceKeyException("Not Implemented For This MaceKey Family")); }
    };

    /// Internal: helper class for IPV4 addresses
    class ipv4_MaceKey : public MaceKey_exception, virtual public PrintPrintable {
      protected:
        MaceAddr addr;

      public:
        virtual bool isUnroutable() const throw(InvalidMaceKeyException) { return addr.isUnroutable(); }
        virtual bool isNullAddress() const { return addr.isNull(); }
        virtual std::string toHex() const { return Log::toHex(std::string((char*)&(addr.local.addr), sizeof(uint32_t))); } //Note, currently only does the IP address
        virtual std::string addressString() const;
        virtual void print(std::ostream& out) const;
        virtual bool isBitArrMaceKey() const { return false; }
        virtual size_t hashOf() const { return __MACE_BASE_HASH__<MaceAddr>()(addr); }

        //ipv4
        virtual const MaceAddr& getMaceAddr() const throw(InvalidMaceKeyException) { return addr; }

        //Constructs an ipv4_MaceKey which would return true for "isNullAddress"
        ipv4_MaceKey(): addr() { }
        ipv4_MaceKey(uint32_t ipaddr, uint16_t port = 0, uint32_t proxyIp = INADDR_NONE, uint16_t proxyPort = 0);
        ipv4_MaceKey(const MaceAddr& ma) {
	  addr = ma;
	}
        //address may be either a dotted-string or a hostname
        ipv4_MaceKey(const std::string& address);
        virtual ~ipv4_MaceKey() { } 
        
        virtual void sqlize(LogNode* node) const {
	  addr.sqlize(node);
	}
        
        virtual void serialize(std::string& str) const {
          mace::serialize(str, &addr);
        }
        virtual int deserialize(std::istream& in) throw(SerializationException) {
          return mace::deserialize(in, &addr);
        }
        virtual void serializeXML_RPC(std::string& str) const throw(SerializationException);
        virtual int deserializeXML_RPC(std::istream& in) throw(SerializationException);

        bool operator==(const ipv4_MaceKey& right) const {
          return addr == right.addr;
        }
        bool operator==(const MaceKey& right) const {
          return addr == right.getMaceAddr();
        }
        bool operator<(const MaceKey& right) const {
          return addr < right.getMaceAddr();
        }
        
        MaceKey_interface_helper* clone() const {
          ipv4_MaceKey* key = new ipv4_MaceKey();
          key->addr = addr;
          return key;
        }
    };
    class ctxnode_MaceKey : public ipv4_MaceKey {
      public:
        //Constructs an ipv4_MaceKey which would return true for "isNullAddress"
        ctxnode_MaceKey(): ipv4_MaceKey() { }
        ctxnode_MaceKey(uint32_t ipaddr, uint16_t port = 0, uint32_t proxyIp = INADDR_NONE, uint16_t proxyPort = 0): ipv4_MaceKey(ipaddr,port,proxyIp,proxyPort){ }
        ctxnode_MaceKey(const MaceAddr& ma): ipv4_MaceKey(ma) { }
        ctxnode_MaceKey(const std::string& address): ipv4_MaceKey(address){ }
    };
    /**
     * vnode_MaceKey
     *
     * a MaceKey subclass which addresses a virtual node.
     * The id of the node is assigned by job scheduler when it instiates a new job.
     * */
    class vnode_MaceKey : public MaceKey_exception, virtual public PrintPrintable  {
      protected:
        uint32_t nodeid;

      public:
        virtual bool isUnroutable() const throw(InvalidMaceKeyException) { return lookup().isUnroutable(); }
        virtual bool isNullAddress() const { return lookup().isNull(); }
        virtual std::string toHex() const { return Log::toHex(std::string((char*)&(lookup().local.addr), sizeof(uint32_t))); } //Note, currently only does the IP address
        virtual std::string addressString() const;
        virtual void print(std::ostream& out) const;
        virtual bool isBitArrMaceKey() const { return false; }
        virtual size_t hashOf() const { return __MACE_BASE_HASH__<uint32_t>()(nodeid); }

        //ipv4
        virtual const MaceAddr& getMaceAddr() const throw(InvalidMaceKeyException) { return lookup(); }

        //Constructs a vnode_MaceKey which would return true for "isNullAddress"
        vnode_MaceKey(): nodeid(0) { }
        //ipv4_MaceKey(uint32_t ipaddr, uint16_t port = 0, uint32_t proxyIp = INADDR_NONE, uint16_t proxyPort = 0);
        vnode_MaceKey(const uint32_t n) {
          nodeid = n;
        }
        //address may be either a dotted-string or a hostname
        //vnode_MaceKey(const std::string& address);
        virtual ~vnode_MaceKey() { } 
        
        virtual void sqlize(LogNode* node) const {
          mace::sqlize( &nodeid, node);
        }
        
        virtual void serialize(std::string& str) const {
          mace::serialize(str, &nodeid);
        }
        virtual int deserialize(std::istream& in) throw(SerializationException) {
          return mace::deserialize(in, &nodeid);
        }
        virtual void serializeXML_RPC(std::string& str) const throw(SerializationException);
        virtual int deserializeXML_RPC(std::istream& in) throw(SerializationException);

        bool operator==(const vnode_MaceKey& right) const {
          return nodeid == right.nodeid;
        }
        bool operator==(const MaceKey& right) const {
          return getMaceAddr() == right.getMaceAddr();
        }
        bool operator<(const MaceKey& right) const {
          return getMaceAddr() < right.getMaceAddr();
        }
        
        MaceKey_interface_helper* clone() const {
          vnode_MaceKey* key = new vnode_MaceKey();
          key->nodeid = nodeid;
          return key;
        }
      private:
        const MaceAddr& lookup() const; 
    };

    /// Internal: helper class for the string_key type MaceKey
    class string_MaceKey : public MaceKey_exception, virtual public PrintPrintable {
      protected:
        mace::string s;

      public:
        virtual bool isNullAddress() const { return s == ""; }
        virtual std::string toHex() const { return Log::toHex(s); } 
        virtual std::string addressString() const { return s; }
        virtual void print(std::ostream& out) const { out << "[" << s << "]"; }
        virtual bool isBitArrMaceKey() const { return false; }
        virtual size_t hashOf() const { return __MACE_BASE_HASH__<mace::string>()(s); }

        string_MaceKey(): s() { }
        string_MaceKey(const mace::string& t) : s(t) { }
        virtual ~string_MaceKey() { } 

        virtual void serialize(std::string& str) const {
          mace::serialize(str, &s);
        }
        virtual int deserialize(std::istream& in) throw(SerializationException) {
          return mace::deserialize(in, &s);
        }
      virtual void serializeXML_RPC(std::string& str) const throw(SerializationException) {
	ASSERT(str.empty());
	str = s;
      }
      virtual int deserializeXML_RPC(std::istream& in) throw(SerializationException) {
	in >> s;
	ASSERT(in.eof());
	return s.size();
      }
        bool operator==(const string_MaceKey& right) const {
          return s == right.s;
        }
        bool operator==(const MaceKey& right) const {
          return s == right.addressString(false); //assume the types match
        }
        bool operator<(const MaceKey& right) const {
          return s < right.addressString(false);
        }
        
        MaceKey_interface_helper* clone() const {
          string_MaceKey* key = new string_MaceKey();
          key->s = s;
          return key;
        }
    };

    //These two classes are intended as a helper for creating address types from int or bit arrays,
    //  such as the 160 bit hash key.  They do not define addressFamily() and thus are virtual classes only.
    /// Internal: a virtual base class for bit array helper.  Only implements isBitArrMaceKey()
    class bitarr_MaceKey : public MaceKey_exception {
      public:
        //common functions
        bool isBitArrMaceKey() const { return true; }
    };

    /**
     * \brief A generic bit array implementation class for bit arrays which are a multiple of 32 bits (can be stored as an integer array).
     *
     * NOTE: "null" addresses will return all 0's for bits, and are only 
     *       distinguishable from the address of all 0's by checking for 
     *       isNullAddress()
     */
    template<int SIZE>
    class intarr_MaceKey : public bitarr_MaceKey, virtual public PrintPrintable {
      protected:
        uint32_t data[SIZE];
        bool null;

        void exceptDivides32(uint32_t digitBits) const throw(DigitBaseException) {
          switch(digitBits) {
            case 4:
            case 32: 
            case 1:
            case 2:
            case 8:
            case 16: return;
          }
          throw(DigitBaseException("DigitBits does not divide 32"));
        }
        void exceptDivides32Bounds(int digit, uint32_t digitBits) const throw(DigitBaseException,IndexOutOfBoundsException) {
          if(digit < 0) {
            throw(IndexOutOfBoundsException("Digit is out of bounds (<= 0)"));
          }
          switch(digitBits) {
            case 4:
              if((uint32_t)digit < SIZE << 3) { return; }
              throw(IndexOutOfBoundsException("Digit is out of bounds"));
            case 32: 
              if((uint32_t)digit < SIZE) { return; }
              throw(IndexOutOfBoundsException("Digit is out of bounds"));
            case 1: 
              if((uint32_t)digit < SIZE << 5) { return; }
              throw(IndexOutOfBoundsException("Digit is out of bounds"));
            case 2:
              if((uint32_t)digit < SIZE << 4) { return; }
              throw(IndexOutOfBoundsException("Digit is out of bounds"));
            case 8:
              if((uint32_t)digit < SIZE << 2) { return; }
              throw(IndexOutOfBoundsException("Digit is out of bounds"));
            case 16:
              if((uint32_t)digit < SIZE << 1) { return; }
              throw(IndexOutOfBoundsException("Digit is out of bounds"));
          }
          throw(DigitBaseException("DigitBits does not divide 32"));
        }

        int getArrPos(int position, uint32_t digitBits) const {
          switch(digitBits) {
            case 4:  return position >> 3;
            case 32: return position;
            case 1:  return position >> 5;
            case 2:  return position >> 4;
            case 8:  return position >> 2;
            case 16: return position >> 1;
          }
          abort();
        }
        uint32_t getDigitMask(int digitBits) const {
          switch(digitBits) {
            case 4:  return 0xf;
            case 1:  return 0x1;
            case 2:  return 0x3;
            case 8:  return 0xff;
            case 16: return 0xffff;
            case 32: return 0xffffffff;
          }
          abort();
        }
        int getShift(int position, uint32_t digitBits) const {
          switch(digitBits) {
            case 4:  return (~(position & 0x7)) << 2;
            case 1:  return (~(position & 0x1f));
            case 2:  return (~(position & 0xf)) << 1;
            case 8:  return (~(position & 0x3)) << 3;
            case 16: return (~(position & 0x1)) << 4;
            case 32: return 0;
          }
	  ASSERT(0);
          return 0; //Appease the intel compiler.
        }
        uint32_t getNthDigitSafe(int position, uint32_t digitBits) const {
          if(null) { return 0; }
          else if(digitBits == sizeof(int)*8) {
            return data[position];
          } else {
            return (data[getArrPos(position, digitBits)] >> getShift(position, digitBits)) & getDigitMask(digitBits);
          }
        }

      public:
        //common functions
        bool isNullAddress() const { return null; }
        void printHex(std::ostream& out) const {
          std::ios::fmtflags f(out.flags());
          out << std::hex << std::setfill('0');
          for(int i=0; i<SIZE; i++) {
            out << std::setw(sizeof(uint32_t)*2) << data[i];
          }
          out.flags(f);
        }
        std::string toHex() const { 
          std::ostringstream out;
          printHex(out);
          return out.str();
        }
        std::string addressString() const { return toHex(); }
        void print(std::ostream& out) const { printHex(out); }
        virtual size_t hashOf() const {
          size_t hash = 0;
          for(int i = 0; i < SIZE; i++) {
            hash += __MACE_BASE_HASH__<uint32_t>()(data[i]);
          }
          return hash;
        }
        //bitarr functions
        int bitLength() const throw(InvalidMaceKeyException) { return SIZE * sizeof(uint32_t) * 8; }
        uint32_t getNthDigit(int position, uint32_t digitBits = 4) const throw(Exception) {
          //throw exception if digitBits doesn't divide 32 (sizeof(int)*8)
          //throw exception if position is too big
          //throw exception if position is too small
          exceptDivides32Bounds(position, digitBits);
          return getNthDigitSafe(position, digitBits);
        }
        void setNthDigit(uint32_t digit, int position, uint32_t digitBits = 4) throw(Exception) {
          //throw exception if digitBits doesn't divide 32 (sizeof(int)*8)
          //throw exception if position is too big
          //throw exception if position is too small
          exceptDivides32Bounds(position, digitBits);
          if(null) { throw(InvalidMaceKeyException("Mace Key Array is void")); }
          else if(digitBits == sizeof(int)*8) {
            data[position] = digit;
          } else {
            int arrPos = getArrPos(position, digitBits);
            int shift = getShift(position, digitBits);
            uint32_t DIGIT_MASK = getDigitMask(digitBits) << shift;
            digit &= (1<<digitBits)-1;
            data[arrPos] &= ~DIGIT_MASK;
            data[arrPos] |= (digit << shift);
          }
        }
        int sharedPrefixLength(const MaceKey& key, uint32_t digitBits = 4) const throw(Exception) {
          //throw exception if digitBits doesn't divide 32 (sizeof(int)*8)
          exceptDivides32(digitBits);
          if(null) { return 0; }
          int common;
          unsigned myBitLength, hisBitLength;
          for(common = 0, myBitLength = this->bitLength(), hisBitLength = key.bitLength();
              myBitLength > 0 && hisBitLength > 0 &&
              getNthDigitSafe(common, digitBits) == key.getNthDigitSafe(common, digitBits);
              common++, myBitLength -= digitBits, hisBitLength -= digitBits
              );
          return common;
        }

        virtual void serialize(std::string& str) const {
          mace::serialize(str, &null);
          if(null) {
            return;
          }
          for(int i = 0; i < SIZE; i++) {
            mace::serialize(str, (data+i));
          }
        }
        //XXX Back things out if deserialization fails.
        virtual int deserialize(std::istream& in) throw(SerializationException) {
          int count = 0;
          count += mace::deserialize(in, &null);
          if(!null) {
            for(int i = 0; i < SIZE; i++) {
              count += mace::deserialize(in, (data+i));
            }
          }
          return count;
        }

        bool operator==(const intarr_MaceKey& right) const {
          if(bitLength() != right.bitLength()) { return false; }
          for(int i = 0; i < SIZE; i++) {
            if(data[i] != right.data[i]) {
              return false;
            }
          }
          return true;
        }

        bool operator==(const MaceKey& right) const {
          if(bitLength() != right.bitLength()) { return false; }
          return sharedPrefixLength(right, 32) == SIZE;
        }
        //XXX: Does byte order screw this up?
        bool operator<(const MaceKey& right) const {
          int common = sharedPrefixLength(right, 32);
          if(common == SIZE && bitLength() != right.bitLength()) {
            return true; //shorter string is less.
          } else if(common == SIZE) {
            return false; //they are equal
          } else if(common*8 == right.bitLength()) {
            return false; //this string is longer
          }
          return data[common] < right.getNthDigit(common, 32);
        }

        void setArr(const char carr[SIZE*4]) {
          uint32_t* arr = (uint32_t*)carr;
          if(arr == NULL) { 
            null = true; 
          }
          else { 
            null = false;
            for(uint32_t i = 0; i < SIZE; i++) {
              data[i] = ntohl(arr[i]);
            }
          }
        }

        void setArr(const uint32_t arr[SIZE]) {
          if(arr == NULL) { 
            null = true; 
          }
          else { 
            null = false;
            memcpy(data, arr, SIZE*sizeof(uint32_t));
          }
        }

        intarr_MaceKey(): null(true) { }
        intarr_MaceKey(const char carr[SIZE*4]): null(carr==NULL) { 
          const uint32_t* arr = (uint32_t*)carr;
          if(arr != NULL) { 
            for(uint32_t i = 0; i < SIZE; i++) {
              data[i] = ntohl(arr[i]);
            }
          } 
        }
        intarr_MaceKey(const uint32_t arr[SIZE]): null(arr==NULL) { 
          if(arr != NULL) { 
            memcpy(data, arr, SIZE*sizeof(uint32_t));
          } 
        }
    };

    /// Internal: helper class for 160-bit sha1 addresses.
    class sha160_MaceKey : public intarr_MaceKey<5> {
      public:
        MaceKey_interface_helper* clone() const {
          sha160_MaceKey* key = new sha160_MaceKey();
          memcpy(key->data, data, sizeof(uint32_t)*5);
          key->null = null;
          return key;
        }

        sha160_MaceKey() : intarr_MaceKey<5>() {
// 	  Log::logf("sha160_MaceKey", "empty constructor");
	}
        sha160_MaceKey(int toHash) {
	  sha1 buf;
	  HashUtil::computeSha1(toHash, buf);
// 	  Log::logf("sha160_MaceKey", "sha1(%d)=%s", toHash, Log::toHex(buf).c_str());
	  setArr(buf.data());
	}
        sha160_MaceKey(const uint32_t arr[5]) : intarr_MaceKey<5>(arr) {
// 	  Log::logf("sha160_MaceKey", "initialized with array");
	}
        sha160_MaceKey(const char arr[5*4]) : intarr_MaceKey<5>(arr) {
// 	  Log::logf("sha160_MaceKey", "initialized with array");
	}
        sha160_MaceKey(const std::string& toHash) {
	  sha1 buf;
	  HashUtil::computeSha1(toHash, buf);
          //           Log::logf("sha160_MaceKey", "sha1('%s')=%s", toHash.c_str(), Log::toHex(buf).c_str());
	  setArr(buf.data());
	}
    };

    /// Internal: helper class for 32-bit sha1 addresses
    class sha32_MaceKey : public intarr_MaceKey<1> {
      public:
        MaceKey_interface_helper* clone() const {
          sha32_MaceKey* key = new sha32_MaceKey();
          memcpy(key->data, data, sizeof(uint32_t));
          key->null = null;
          return key;
        }

        sha32_MaceKey() : intarr_MaceKey<1>() {}
        sha32_MaceKey(int toHash) { sha1 buf; HashUtil::computeSha1(toHash, buf); setArr((uint32_t*)buf.data()); }
        sha32_MaceKey(const uint32_t arr[1]) : intarr_MaceKey<1>(arr) { }
        sha32_MaceKey(const char arr[4]) : intarr_MaceKey<1>(arr) { }
        sha32_MaceKey(const std::string& toHash) { sha1 buf; HashUtil::computeSha1(toHash, buf); setArr(buf.data()); }
    };

  public:

    struct ipv4_type {}; ///< used to distinguish helper class type for MaceKey constructor
    struct ctxnode_type {}; ///< used to distinguish helper class type for MaceKey constructor
    struct vnode_type {}; ///< used to distinguish helper class type for MaceKey constructor
    struct sha160_type {}; ///< used to distinguish helper class type for MaceKey constructor
    struct sha32_type {}; ///< used to distinguish helper class type for MaceKey constructor
    struct string_type {}; ///< used to distinguish helper class type for MaceKey constructor

    //ipv4
    /// Creates a "null" IPV4 MaceKey
    MaceKey(ipv4_type t) : helper(new ipv4_MaceKey()), address_family(IPV4) { }
    /// old constructor to pass in each port and ip address separately.
    MaceKey(ipv4_type t, uint32_t ipaddr, uint16_t port = 0, uint32_t pIp = INADDR_NONE, uint16_t pport = 0) : helper(new ipv4_MaceKey(ipaddr, port, pIp, pport)), address_family(IPV4) { }
    /// performs MaceAddr parsing and DNS lookup for string addr.  
    MaceKey(ipv4_type t, const std::string& addr) : helper(new ipv4_MaceKey(addr)), address_family(IPV4) { }
    /// wraps passed MaceAddr in a MaceKey
    MaceKey(ipv4_type t, const MaceAddr& maddr) : helper(new ipv4_MaceKey(maddr)), address_family(IPV4) { }

    //ctxnode
    /// Creates a "null" ContextNode MaceKey
    MaceKey(ctxnode_type t) : helper(new ctxnode_MaceKey()), address_family(CONTEXTNODE) { }
    /// old constructor to pass in each port and ip address separately.
    MaceKey(ctxnode_type t, uint32_t ipaddr, uint16_t port = 0, uint32_t pIp = INADDR_NONE, uint16_t pport = 0) : helper(new ctxnode_MaceKey(ipaddr, port, pIp, pport)), address_family(CONTEXTNODE) { }
    /// performs MaceAddr parsing and DNS lookup for string addr.  
    MaceKey(ctxnode_type t, const std::string& addr) : helper(new ctxnode_MaceKey(addr)), address_family(CONTEXTNODE) { }
    /// wraps passed MaceAddr in a MaceKey
    MaceKey(ctxnode_type t, const MaceAddr& maddr) : helper(new ctxnode_MaceKey(maddr)), address_family(CONTEXTNODE) { }

    //vnode
    MaceKey(vnode_type t) : helper(new vnode_MaceKey()), address_family(VNODE) { }
    //vnode with node id passed it
    MaceKey(vnode_type t, const uint32_t nodeid) : helper(new vnode_MaceKey(nodeid)), address_family(VNODE) { }


    //sha160

    /// Creates a "null" SHA160 MaceKey
    MaceKey(sha160_type t) : helper(new sha160_MaceKey()), address_family(SHA160) { }
    /// Computes the sha1 of \c toHash, and stores that as the address
    MaceKey(sha160_type t, int toHash) : helper(new sha160_MaceKey(toHash)), address_family(SHA160) { }
    /// Sets the int array to the provided int array
    MaceKey(sha160_type t, const uint32_t arr[5]) : helper(new sha160_MaceKey(arr)), address_family(SHA160) { }
    /// Initializes the int array based on the characters given.  Note, often differs from the int array version in byte ordering.
    MaceKey(sha160_type t, const char arr[5*4]) : helper(new sha160_MaceKey(arr)), address_family(SHA160) { }
    /// Computes the sha1 of \c toHash, and stores that as the address
    MaceKey(sha160_type t, const std::string& toHash) : helper(new sha160_MaceKey(toHash)), address_family(SHA160) { }

    //sha32
    
    /// Creates a "null" SHA32 MaceKey
    MaceKey(sha32_type t) : helper(new sha32_MaceKey()), address_family(SHA32) { }
    /// Computes the sha1 of \c toHash, and stores the first 32 bits as the address
    MaceKey(sha32_type t, int toHash) : helper(new sha32_MaceKey(toHash)), address_family(SHA32) { }
    /// Sets the int array to the provided int array
    MaceKey(sha32_type t, const uint32_t arr[1]) : helper(new sha32_MaceKey(arr)), address_family(SHA32) { }
    /// Initializes the int array based on the characters given.  Note, often differs from the int array version in byte ordering.
    MaceKey(sha32_type t, const char arr[4]) : helper(new sha32_MaceKey(arr)), address_family(SHA32) { }
    /// Computes the sha1 of \c toHash, and stores the first 32 bits as the address
    MaceKey(sha32_type t, const std::string& toHash) : helper(new sha32_MaceKey(toHash)), address_family(SHA32) { }

    //string

    /// Createas a "null" string address
    MaceKey(string_type t) : helper(new string_MaceKey()), address_family(STRING_ADDRESS) { }
    /// Sets the address to be string \c u
    MaceKey(string_type t, const mace::string& u) : helper(new string_MaceKey(u)), address_family(STRING_ADDRESS) { }

    /// Parses the address family prefix, and constructs an appropriate MaceKey
    MaceKey(const mace::string& a) {
      if(a.substr(0, 5) == "IPV4/") {
        address_family = IPV4;
        helper = HelperPtr(new ipv4_MaceKey(a.substr(5)));
      } 
      else if(a.substr(0, 5) == "CONTEXTNODE/") {
        address_family = CONTEXTNODE;
        helper = HelperPtr(new ctxnode_MaceKey(a.substr(5)));
      } 
      else if(a.substr(0, 5) == "VNODE/") {
        address_family = VNODE;
        helper = HelperPtr(new vnode_MaceKey( boost::lexical_cast<uint32_t>(  a.substr(5) ) ));
      } 
      else if(a.substr(0,7) == "SHA160/") {
        if(a.size() != 40+7) {
          throw(InvalidMaceKeyException("SHA160 Address must have 40 hexidecmal characters"));
        }
        address_family = SHA160;
        char arr[5*4];
        typedef uint32_t __attribute__((__may_alias__)) uint32_alias;
        uint32_alias* c = (uint32_t*)arr;
        sscanf(a.c_str(), "SHA160/%8"PRIx32"%8"PRIx32"%8"PRIx32"%8"PRIx32"%8"PRIx32, (uint32_t *)(arr), (uint32_t *)(arr+4), (uint32_t *)(arr+8), (uint32_t *)(arr+12), (uint32_t *)(arr+16));
        *( c+0) = htonl(*( c+0));
        *( c+1) = htonl(*( c+1));
        *( c+2) = htonl(*( c+2));
        *( c+3) = htonl(*( c+3));
        *( c+4) = htonl(*( c+4));
        helper = HelperPtr(new sha160_MaceKey(arr));
      }
      else if(a.substr(0,6) == "SHA32/") {
        if(a.size() != 8+6) {
          throw(InvalidMaceKeyException("SHA32 Address must have 8 hexidecmal characters"));
        }
        address_family = SHA32;
        char arr[4];
        typedef uint32_t __attribute__((__may_alias__)) uint32_alias;
        uint32_alias* c = (uint32_t*)arr;
        sscanf(a.c_str(), "SHA32/%8"PRIx32, (uint32_t *)(arr));
        * c = htonl(*c);
        helper = HelperPtr(new sha32_MaceKey(arr));
      }
      else if(a.substr(0,7) == "STRING/") {
        address_family = STRING_ADDRESS;
        helper = HelperPtr(new string_MaceKey(a.substr(7)));
      }
    }
};

const mace::MaceKey::ipv4_type ipv4 = mace::MaceKey::ipv4_type(); ///< defines an object for passing into the Macekey constructor
const mace::MaceKey::ctxnode_type ctxnode = mace::MaceKey::ctxnode_type(); ///< defines an object for passing into the Macekey constructor
const mace::MaceKey::vnode_type vnode = mace::MaceKey::vnode_type(); ///< defines an object for passing into the Macekey constructor
const mace::MaceKey::sha160_type sha160 = mace::MaceKey::sha160_type(); ///< defines an object for passing into the Macekey constructor
const mace::MaceKey::sha32_type sha32 = mace::MaceKey::sha32_type(); ///< defines an object for passing into the Macekey constructor
const mace::MaceKey::string_type string_key = mace::MaceKey::string_type(); ///< defines an object for passing into the Macekey constructor

//Works only with BitArrMaceKey's
/**
 * \brief A class to represent the difference between two bit array MaceKey
 * objects (big integers).  (like TimeDiff in other languages).
 */
class MaceKeyDiff : public mace::PrintPrintable, public mace::Serializable {
  friend MaceKey operator-(const MaceKey& to, const MaceKeyDiff& from) throw(InvalidMaceKeyException);
  friend MaceKey operator+(const MaceKey& to, const MaceKeyDiff& from) throw(InvalidMaceKeyException);
  public:
    enum type_t { ZERO, ___SIGNED, ___UNSIGNED, _INFINITY, _NEG_INFINITY };
  private:
    type_t type;
    int size;
    uint32_t* data;
  public:
    MaceKeyDiff() : type(ZERO), size(0), data(NULL) {}
    MaceKeyDiff(type_t t, int s = 0, uint32_t* d = NULL) : type(t), size(s), data(d) {}
    MaceKeyDiff(const MaceKeyDiff&);
    virtual ~MaceKeyDiff() { delete[] data; }
    void print(std::ostream&) const;
    void printNode(PrintNode& pr, const std::string& name) const;
    
    void serialize(std::string& s) const {
      int itype = (int)type;
      mace::serialize(s, &itype);
      mace::serialize(s, &size);
      for(int i = 0; i < size; i++) {
        mace::serialize(s, (data+i));
      }
    } // serialize

    int deserialize(std::istream& in) throw (mace::SerializationException) {
      int o = 0;
      int itype;
      o += mace::deserialize(in, &itype);
      type = (type_t) itype;
      o += mace::deserialize(in, &size);
      for(int i = 0; i < size; i++) {
        o += mace::deserialize(in, (data+i));
      }
      return o;
    } // deserialize
    /**
     * \brief returns the sign of the MaceKeyDiff
     *
     * Note: unsigned diffs can only return 1 or 0.
     *
     * @return -1, 0, or 1, depending on sign of value.
     */
    int sign();
    /// Returns sign() > 0
    bool isPositive() { return sign() > 0; }
    /// Returns sign() < 0
    bool isNegative() { return sign() < 0; }
    /// Returns sign() == 0
    bool isZero() { return sign() == 0; }
    /**
     * \brief modifies *this to be its unsigned value.
     *
     * No-op for 0, inf, or unsigned diffs. -inf becomes +inf. For signed values, 
     * do nothing if sign() >= 0, otherwise do complement plus one.
     *
     * @return reference to *this
     */
    MaceKeyDiff& abs();
    /**
     * \brief modifies *this to be signed.
     *
     * Note: no-op unless type is unsigned, then set type to signed.
     * No content changes.  It's like a cast.
     *
     * @return reference to *this
     */
    MaceKeyDiff& toSigned() {
      if(type == ___UNSIGNED) { type = ___SIGNED; }
      return *this;
    }
    /**
     * \brief modifies *this to be unsigned.
     *
     * Note: no-op unless type is signed, then set type to unsigned.
     * No content changes.  It's like a cast.
     *
     * \todo Check if -inf should be changed?
     *
     * @return reference to *this
     */
    MaceKeyDiff& toUnsigned() {
      if(type == ___SIGNED) { type = ___UNSIGNED; }
      return *this;
    }
    static MaceKeyDiff NEG_INF; ///< basic helper for a NEG_INF MaceKeyDiff
    static MaceKeyDiff INF; ///< basic helper for an INF MaceKeyDiff
    MaceKeyDiff& operator=(const MaceKeyDiff&); ///< standard assignment
    bool operator<(const MaceKeyDiff&) const; ///< standard less than
    bool operator>(const MaceKeyDiff& right) const { return !(*this<=right); } ///< standard greater than 
    bool operator==(const MaceKeyDiff&) const; ///< standard equality check
    bool operator<=(const MaceKeyDiff& right) const; ///< standard less than or equal
    bool operator>=(const MaceKeyDiff& right) const { return !(*this<right); } ///< standard greater than or equal
    MaceKeyDiff operator>>(int bits) const; ///< standard bit shifting
    MaceKeyDiff operator+(const MaceKeyDiff&) const; ///< add a MaceKeyDiff to another
    MaceKeyDiff operator-(const MaceKeyDiff&) const; ///< subtract a MaceKeyDiff from another
    MaceKeyDiff operator+(uint32_t) const; ///< add an unsigned int to a MaceKeyDiff
    MaceKeyDiff operator-(uint32_t) const; ///< subtract an unsigned int from a MaceKeyDiff
};

MaceKey operator+(const MaceKey&, const MaceKeyDiff&) throw(InvalidMaceKeyException); ///< adds a MaceKeyDiff to a MaceKey
MaceKey operator-(const MaceKey&, const MaceKeyDiff&) throw(InvalidMaceKeyException); ///< subtracts a MaceKeyDiff from a MaceKey
MaceKey operator+(const MaceKey&, uint32_t) throw(InvalidMaceKeyException); ///< adds a 32-bit integer as an implicit MaceKeyDiff
MaceKey operator-(const MaceKey&, uint32_t) throw(InvalidMaceKeyException); ///< subtracts a 32-bit integer as an implicit MaceKeyDiff
MaceKeyDiff operator-(const MaceKey&, const MaceKey&) throw(InvalidMaceKeyException); ///< create a MaceKeyDiff between two MaceKeys
} // namespace mace


/*namespace boost
{
  template<>
  mace::MaceKey lexical_cast<mace::MaceKey, std::string>(const std::string & arg) {
    
    return mace::MaceKey( arg );
  }
}*/

#ifdef __MACE_USE_UNORDERED__

//Defining hash template structs for MaceKey
namespace std {
namespace tr1 {

template<> struct hash<mace::MaceKey>  {
  size_t operator()(const mace::MaceKey& x) const { return x.hashOf(); }
};

template<> struct hash<mace::MaceKey*>  {
  size_t operator()(const mace::MaceKey*& x) {
    return (x==NULL?0:x->hashOf());
  }
};

} 
}

#else 

//Defining hash template structs for MaceKey
namespace __gnu_cxx {

template<> struct hash<mace::MaceKey>  {
  size_t operator()(const mace::MaceKey& x) const { return x.hashOf(); }
};

template<> struct hash<mace::MaceKey*>  {
  size_t operator()(const mace::MaceKey*& x) {
    return (x==NULL?0:x->hashOf());
  }
};

} // namespace __gnu_cxx

#endif

#endif //__MACEKEY_H
