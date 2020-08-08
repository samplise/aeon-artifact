/* 
 * MaceKey.cc : part of the Mace toolkit for building distributed systems
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
#include <limits.h>
#include "MaceKey.h"
#include "KeyRange.h"
#include "Util.h"
#include "SockUtil.h"
#include "params.h"
#include "Log.h"
#include "mace-macros.h"
#include "ContextMapping.h"

namespace mace {
  bool SockAddr::isNull() const {
    return *this == SockUtil::NULL_MSOCKADDR;
  } // isNull

  void SockAddr::print(std::ostream& out) const {
    Util::printAddrString(out, *this);
  } // print

  void SockAddr::printNode(PrintNode& pr, const std::string& name) const {
    pr.addChild(PrintNode(name, "SockAddr", toString()));
  }

  void MaceAddr::print(std::ostream& out) const {
    Util::printAddrString(out, *this);
  } // print

  void MaceAddr::printNode(PrintNode& pr, const std::string& name) const {
    pr.addChild(PrintNode(name, "MaceAddr", toString()));
  }

  bool MaceAddr::isNull() const {
    return *this == SockUtil::NULL_MACEADDR;
  } // isNull

  MaceKey::ipv4_MaceKey::ipv4_MaceKey(uint32_t ipaddr, uint16_t port,
				      uint32_t proxyIp, uint16_t proxyPort) { 
    addr.local.addr = ipaddr;
    if (port) {
      addr.local.port = port;
    }
    else {
      addr.local.port = Util::getPort();
    }
    if(proxyIp == INADDR_NONE) { 
      addr.proxy = SockUtil::NULL_MSOCKADDR;
    } else {
      addr.proxy.addr = proxyIp;
      addr.proxy.port = proxyPort;
      if (proxyPort == 0) {
        addr.proxy.port = Util::getPort();
      }
    }
  } // ipv4_MaceKey

  void MaceKey::ipv4_MaceKey::serializeXML_RPC(std::string& str) const throw(SerializationException){
    ASSERT(str.empty());
    str = Util::getAddrString(addr);
  } // serializeXML_RPC

  int MaceKey::ipv4_MaceKey::deserializeXML_RPC(std::istream& in) throw(SerializationException) {
    mace::string s;
    in >> s;
    ASSERT(in.eof());
    addr = Util::getMaceAddr(s);
    return s.size();
  } // deserializeXML_RPC

  std::string MaceKey::ipv4_MaceKey::addressString() const {
    return Util::getAddrString(addr, false);
  }

  void MaceKey::ipv4_MaceKey::print(std::ostream& out) const {
    Util::printAddrString(out, addr);
  }

  // vnode_MaceKey
  void MaceKey::vnode_MaceKey::serializeXML_RPC(std::string& str) const throw(SerializationException){
    ASSERT(str.empty());
    str = Util::getAddrString( lookup() );
  } // serializeXML_RPC

  int MaceKey::vnode_MaceKey::deserializeXML_RPC(std::istream& in) throw(SerializationException) {
    mace::string s;
    in >> s;
    ASSERT(in.eof());
    nodeid = boost::lexical_cast<uint32_t>( s );
    return s.size();
  } // deserializeXML_RPC

  void MaceKey::vnode_MaceKey::print(std::ostream& out) const {
    Util::printAddrString(out, lookup() );
  }
  std::string MaceKey::vnode_MaceKey::addressString() const {
    return Util::getAddrString( lookup(), false);
  }
    const MaceAddr& MaceKey::vnode_MaceKey::lookup() const{
        const MaceAddr& addr = ContextMapping::lookupVirtualNode( nodeid );
        return addr;
    }
  ////////////////

  MaceKey::ipv4_MaceKey::ipv4_MaceKey(const std::string& address) { addr = Util::getMaceAddr(address); }

  bool MaceKey::getHostnameOnly() {
    static bool hostnameOnly = (params::containsKey(params::MACE_PRINT_HOSTNAME_ONLY) && params::get<int>(params::MACE_PRINT_HOSTNAME_ONLY));
    return hostnameOnly;
  }

  /**
   * \brief returns the length between two points
   *  on the circle (i.e. between the two ids in id-space).
   *
   *  distance can be interpreted either signed or unsigned
   *  positive values: 'to' comes after 'from'
   *  negative values: 'to' comes before 'from'
   *  in absolute terms, the signed distance can be as large as half the circle
   *  
   *  NOTE: subtraction defaults to UNSIGNED
   */
  MaceKeyDiff operator-(const MaceKey& to, const MaceKey& from) throw(InvalidMaceKeyException)
  {
    if(from.isNullAddress() || to.isNullAddress()) { throw(InvalidMaceKeyException("arc_distance not valid on null mace keys")); }
    if(!from.isBitArrMaceKey() || !to.isBitArrMaceKey()) { throw(InvalidMaceKeyException("arc_distance not valid on non-bitarr mace keys")); }
    if(from.bitLength() != to.bitLength()) { throw(InvalidMaceKeyException("bit lengths must be the same when computing distance")); }
    if (from==to) // Same point, zero distance
      return MaceKeyDiff();

    if(from.bitLength() % 8*sizeof(uint32_t)) {
      ABORT("Exception: MaceKeyDiff presently only supports bit arrays which are divisible in length by 32 bits");
    }

    int ints = from.bitLength() / (8*sizeof(uint32_t));
    uint32_t* data = new uint32_t[ints];
    memset(data, 0, ints*sizeof(uint32_t)); //to make valgrind unhappy.  completely unconvinced its necessary.

    for(int i = 0; i < ints; i++) {
      uint32_t a = to.getNthDigit(i,32);
      uint32_t b = from.getNthDigit(i,32);
      if( a >= b ) {
        data[i] = a - b;
      } else {
        int j = i;
        do { data[j]--; } while(data[j--] == UINT_MAX && j>=0); //Perform borrow operation.  
        data[i] = a - b; //NOTE: This is actually (UINT_MAX+1) - (b-a)
      }
    }
    return MaceKeyDiff(MaceKeyDiff::___UNSIGNED, ints, data);
  }
  MaceKeyDiff MaceKeyDiff::operator-(const MaceKeyDiff& from) const
  {
    ADD_FUNC_SELECTORS;
    if (*this==from) // Same point, zero distance
      return MaceKeyDiff();

    if(type == _INFINITY) { return *this; } // inf - anything == inf
    if(type == _NEG_INFINITY) { return *this; } // -inf - anything == -inf
    if(from.type == _INFINITY) { return NEG_INF; } // anything else - inf == -inf
    if(from.type == _NEG_INFINITY) { return INF; } // anything else - -inf == inf
    if(from.type == ZERO) { return *this; } // anything - 0 == anything
    if(size != from.size && type != ZERO) {
      ABORT("Mismatched sizes of MaceKeyDiff.  Returning 0");
      return MaceKeyDiff();
    }

    //     maceout << "this " << *this << " from " << from << Log::endl;

    //At this point -- the arrays are the same size.  We perform a subtraction like normal, and 
    //return a MaceKeyDiff with the same signage as this one
    uint32_t* diffdata = new uint32_t[from.size];
    memset(diffdata, 0, from.size*sizeof(uint32_t));

    for(int i = 0; i < from.size; i++) {
      uint32_t a = (type == ZERO ? 0 : data[i]);
      uint32_t b = from.data[i];
      //       maceout << "a " << std::setw(8) << std::setfill('0') << std::hex << a << " b " << std::setw(8) << std::setfill('0') << std::hex << b << " a >= b " << ( a >= b ) << Log::endl;
      if( a >= b ) {
        diffdata[i] = a - b;
      } else {
        int j = i;
        do { diffdata[j]--; } while(diffdata[j--] == UINT_MAX && j>=0); //Perform borrow operation.  
        diffdata[i] = a - b; //NOTE: This is actually (UINT_MAX+1) - (b-a)
      }
    }
    //     maceout << "first digit " << diffdata[0] << " signed " << ((int)diffdata[0]) << " hex " << std::setw(8) << std::setfill('0') << std::hex << diffdata[0] << Log::endl;
    return MaceKeyDiff((type==ZERO?___SIGNED:type), from.size, diffdata);
  }
  MaceKeyDiff MaceKeyDiff::operator+(const MaceKeyDiff& from) const
  {
    ADD_FUNC_SELECTORS;
    if(type == _INFINITY) { return *this; } // inf + anything == inf
    if(type == _NEG_INFINITY) { return *this; } // -inf + anything == -inf
    if(from.type == _INFINITY) { return INF; } // anything else + inf == inf
    if(from.type == _NEG_INFINITY) { return NEG_INF; } // anything else + -inf == -inf
    if(from.type == ZERO) { return *this; } 
    if(type == ZERO) { return from; }
    if(size != from.size) {
      ABORT("Mismatched sizes of MaceKeyDiff.  Returning 0");
      return MaceKeyDiff();
    }

    //At this point -- the arrays are the same size.  We perform an addition like normal, and 
    //return a MaceKeyDiff with the same signage as this one
    uint32_t* adddata = new uint32_t[size];
    memset(adddata, 0, size*sizeof(uint32_t));

    int carry = 0;
    for(int i = size-1; i >= 0; i--) {
      adddata[i] = from.data[i]+data[i]+carry;
      if(from.data[i] > UINT_MAX-data[i] || (carry == 1 && (data[i] + from.data[i] == UINT_MAX))) {
        carry = 1;
      } else {
        carry = 0;
      }
    }
    return MaceKeyDiff(type, size, adddata);
  }
  MaceKeyDiff MaceKeyDiff::operator+(uint32_t from) const
  {
    if(type == _INFINITY) { return *this; } // inf + anything == inf
    if(type == _NEG_INFINITY) { return *this; } // -inf + anything == -inf
    if(from == 0) { return *this; } 
    if(type == ZERO) { ABORT("Exception: ERROR -- don't know how to set 'size' adding ZERO to an int.  Report if this occurs"); }

    if(size == 0) { return *this; }

    //At this point -- the arrays are the same size.  We perform an addition like normal, and 
    //return a MaceKeyDiff with the same signage as this one
    uint32_t* adddata = new uint32_t[size];
    memcpy(adddata, data, size*sizeof(uint32_t));
    adddata[size-1] += from;

    if(data[size-1] > UINT_MAX - from) {
      int j = size-2;
      do { data[j]++; } while(data[j--]==0 && j>=0); //Perform increment operation.  
    }
    return MaceKeyDiff(type, size, adddata);
  }
  MaceKeyDiff MaceKeyDiff::operator-(uint32_t from) const
  {
    if(type == _INFINITY) { return *this; } // inf + anything == inf
    if(type == _NEG_INFINITY) { return *this; } // -inf + anything == -inf
    if(from == 0) { return *this; } 
    if(type == ZERO) { ABORT("Exception: ERROR -- don't know how to set 'size' adding ZERO to an int.  Report if this occurs"); }

    if(size == 0) { return *this; }

    //At this point -- the arrays are the same size.  We perform an addition like normal, and 
    //return a MaceKeyDiff with the same signage as this one
    uint32_t* adddata = new uint32_t[size];
    memcpy(adddata, data, size*sizeof(uint32_t));
    adddata[size-1] -= from;

    if(data[size-1] < from) {
      int j = size-2;
      do { data[j]--; } while(data[j--] == UINT_MAX && j>=0); //Perform borrow operation.  
    }
    return MaceKeyDiff(type, size, adddata);
  }
  MaceKeyDiff& MaceKeyDiff::operator=(const MaceKeyDiff& d) 
  {
    if(this == &d) { return *this; }
    this->type = d.type;
    this->size = d.size;
    if(this->size > 0) {
      delete[] data;
      this->data = new uint32_t[this->size];
      memcpy(this->data, d.data, d.size*sizeof(uint32_t));
    } else {
      delete[] data;
      this->data = NULL;
    }
    return *this;
  }
  MaceKeyDiff::MaceKeyDiff(const MaceKeyDiff& d) : type(d.type), size(d.size), data(NULL)
  {
    if(this->size > 0) {
      this->data = new uint32_t[this->size];
      memcpy(this->data, d.data, d.size*sizeof(uint32_t));
    }
  }
  int MaceKeyDiff::sign() 
  {
    if(type == ZERO) { return 0; }
    if(type == _INFINITY) { return 1; }
    if(type == _NEG_INFINITY) { return -1; }
    //else type is signed or unsigned.
    int i;
    for(i = 0; i < size && !data[i]; i++);
    if(i == size) { /*FIXME: Should we set type to ZERO? */ return 0; }
    if(type == ___UNSIGNED) { return 1; }
    if((int)data[0] < 0) { return -1; }
    return 1;
  }
  MaceKeyDiff& MaceKeyDiff::abs() 
  {
    switch(type) {
      case ZERO: 
      case _INFINITY: 
      case ___UNSIGNED: break;
      case _NEG_INFINITY: type=_INFINITY; break;
      case ___SIGNED: {
                    if(sign() >= 0) { break; }
                    for(int i = 0; i < size; i++) {
                      data[i] = ~data[i];
                    }
                    int j = size-1;
                    do { data[j]++; } while(data[j--]==0 && j>=0); //Perform increment operation.  
                   }
    }
    return *this;
  }
  bool MaceKeyDiff::operator==(const MaceKeyDiff& right) const 
  {
    //Nothing is equal to +/- inf
    if(right.type == _INFINITY) { return false; }
    if(type == _NEG_INFINITY) { return false; } 
    if(type == _INFINITY) { return false; } 
    if(right.type == _NEG_INFINITY) { return false; } 

    if(type == ZERO || right.type == ZERO) { return type == right.type; }
    if(size != right.size) { return false; } //Note, this is seemingly inconsistent with the subtraction -- which returns 0.
    
    for(int i=0; i<size; i++) {
      if(data[i] != right.data[i]) {
        return false;
      }
    }
    return true; //Note, this makes things equal if one is signed and the other isn't.  
  }
  bool MaceKeyDiff::operator<(const MaceKeyDiff& right) const 
  {
    if(right.type == _INFINITY) { return true; } //infinity is bigger than everything (including inf)
    if(type == _NEG_INFINITY) { return true; } //neg inf is less than everything (including neg inf)
    if(type == _INFINITY) { return false; } //inf is less than nothing but inf
    if(right.type == _NEG_INFINITY) { return false; } //neg inf is greater than nothing except neg inf
    if(type == ___UNSIGNED && right.type == ___UNSIGNED) {
      for(int i=0; i < size; i++) {
        if(data[i] < right.data[i]) { return true; }
        else if(data[i] > right.data[i]) { return false; }
      }
      return false;
    }
    MaceKeyDiff diff = *this - right;
    diff.toSigned();
    //     maceout << diff << " sign " << diff.sign() << Log::endl;
    return diff.sign() < 0;
  }
  bool MaceKeyDiff::operator<=(const MaceKeyDiff& right) const 
  {
    if(right.type == _INFINITY) { return true; } //infinity is bigger than everything (including inf)
    if(type == _NEG_INFINITY) { return true; } //neg inf is less than everything (including neg inf)
    if(type == _INFINITY) { return false; } //inf is less than nothing but inf
    if(right.type == _NEG_INFINITY) { return false; } //neg inf is greater than nothing except neg inf
    if(type == ___UNSIGNED && right.type == ___UNSIGNED) {
      for(int i=0; i < size; i++) {
        if(data[i] < right.data[i]) { return true; }
        else if(data[i] > right.data[i]) { return false; }
      }
      return true;
    }
    MaceKeyDiff diff = *this - right;
    //XXX: This will be true on size mismatch
    return diff.toSigned().sign() <= 0;
  }
  MaceKeyDiff MaceKeyDiff::operator>>(int bits) const
  {
    uint32_t* adddata = new uint32_t[size];
    memcpy(adddata, data, size*sizeof(uint32_t));
    while(bits > 0) {
      for(int i=size-1; i>0; i--) {
        int shift = adddata[i-1]%2;
        shift = shift << (8*sizeof(uint32_t)-1);
        adddata[i] = adddata[i] >> 1;
        adddata[i] += shift;
      }
      if(size > 0) {
        adddata[0] = adddata[0]>>1;
      }
      bits--;
    }
    return MaceKeyDiff(type, size, adddata);
  }
  MaceKey operator-(const MaceKey& to, const MaceKeyDiff& from) throw(InvalidMaceKeyException)
  {
    if(!to.isBitArrMaceKey()) { throw(InvalidMaceKeyException("Diff subtraction only valid on BitArr MaceKeys")); }
    if(from.type == MaceKeyDiff::_NEG_INFINITY || from.type == MaceKeyDiff::_INFINITY) {
      throw(InvalidMaceKeyException("Cannot add inf or -inf to a mace key"));
    }
    if(from.type == MaceKeyDiff::ZERO) { return to; }

    if(to.bitLength() != (int)(from.size*8*sizeof(uint32_t))) {
      throw(InvalidMaceKeyException("Cannot add diff to mace key of different size"));
    }

    //At this point -- the arrays are the same size.  We perform a subtraction like normal 
    uint32_t* diffdata = new uint32_t[from.size];
    memset(diffdata, 0, from.size*sizeof(uint32_t));

    for(int i = 0; i < from.size; i++) {
      uint32_t a = to.getNthDigit(i,32);
      uint32_t b = from.data[i];
      if( a >= b ) {
        diffdata[i] = a - b;
      } else {
        int j = i;
        do { diffdata[j]--; } while(diffdata[j--] == UINT_MAX && j>=0); //Perform borrow operation.  
        diffdata[i] = a - b; //NOTE: This is actually (UINT_MAX+1) - (b-a)
      }
    }
    switch(to.addressFamily()) {
      case SHA32: { MaceKey m(sha32, diffdata); delete[] diffdata; return m; }
      case SHA160: { MaceKey m(sha160, diffdata); delete[] diffdata; return m; }
      default: ABORT("Exception: need to update MaceKey operators to handle new address families");
    }
  }
  MaceKey operator+(const MaceKey& from, const MaceKeyDiff& to) throw(InvalidMaceKeyException)
  {
    if(!from.isBitArrMaceKey()) { throw(InvalidMaceKeyException("Diff addition only valid on BitArr MaceKeys")); }
    if(to.type == MaceKeyDiff::_NEG_INFINITY || to.type == MaceKeyDiff::_INFINITY) {
      throw(InvalidMaceKeyException("Cannot add inf or -inf to a mace key"));
    }
    if(to.type == MaceKeyDiff::ZERO) { return from; }

    if(from.bitLength() != (int)(to.size*8*sizeof(uint32_t))) {
      throw(InvalidMaceKeyException("Cannot add diff to mace key of different size"));
    }

    //At this point -- the arrays are the same size.  We perform a subtraction like normal 
    uint32_t* adddata = new uint32_t[to.size];

    int carry = 0;
    for(int i = to.size-1; i >= 0; i--) {
      adddata[i] = from.getNthDigit(i,32)+to.data[i]+carry;
      if(to.data[i] > UINT_MAX-from.getNthDigit(i,32) || (carry == 1 && (from.getNthDigit(i,32) + to.data[i] == UINT_MAX))) {
        carry = 1;
      } else {
        carry = 0;
      }
    }
    switch(from.addressFamily()) {
      case SHA32: { MaceKey m(sha32, adddata); delete[] adddata; return m; }
      case SHA160: { MaceKey m(sha160, adddata); delete[] adddata; return m; }
      default: ABORT("Exception: need to update MaceKey operators to handle new address families");
    }
  }
  MaceKey operator+(const MaceKey& from, uint32_t to) throw(InvalidMaceKeyException)
  {
    if(!from.isBitArrMaceKey()) { throw(InvalidMaceKeyException("uint32_t addition only valid on BitArr MaceKeys")); }
    if(to == 0) { return from; }

    //At this point -- the arrays are the same size.  We perform a subtraction like normal 
    int size = from.bitLength()/32;
    ASSERT(size > 0);
    uint32_t* adddata = new uint32_t[size];
    for(int i=0; i<size; i++) {
      adddata[i] = from.getNthDigit(i, 32);
    }
    adddata[size-1] += to;
    if(from.getNthDigit(size-1,32) > UINT_MAX - to) {
      int j = size-2;
      do { adddata[j]++; } while(adddata[j--]==0 && j>=0); //Perform increment operation.  
    }

    switch(from.addressFamily()) {
      case SHA32: { MaceKey m(sha32, adddata); delete[] adddata; return m; }
      case SHA160: { MaceKey m(sha160, adddata); delete[] adddata; return m; }
      default: ABORT("Exception: need to update MaceKey operators to handle new address families");
    }
  }
  MaceKey operator-(const MaceKey& from, uint32_t to) throw(InvalidMaceKeyException)
  {
    if(!from.isBitArrMaceKey()) { throw(InvalidMaceKeyException("uint32_t addition only valid on BitArr MaceKeys")); }
    if(to == 0) { return from; }

    //At this point -- the arrays are the same size.  We perform a subtraction like normal 
    int size = from.bitLength()/32;
    uint32_t* diffdata = new uint32_t[size];
    for(int i=0; i<size; i++) {
      diffdata[i] = from.getNthDigit(i, 32);
    }
    diffdata[size-1] -= to;

    if(from.getNthDigit(size-1,32) < to) {
      int j = size-2;
      do { diffdata[j]--; } while(diffdata[j--] == UINT_MAX && j>=0); //Perform borrow operation.  
    }
    switch(from.addressFamily()) {
      case SHA32: { MaceKey m(sha32, diffdata); delete[] diffdata; return m; }
      case SHA160: { MaceKey m(sha160, diffdata); delete[] diffdata; return m; }
      default: ABORT("Exception: need to update MaceKey operators to handle new address families");
    }
  }
  void MaceKeyDiff::print(std::ostream& out) const {
    switch(type) {
      case ZERO: out << "ZERO"; return;
      case _INFINITY: out << "INF"; return;
      case _NEG_INFINITY: out << "-INF"; return;
      case ___SIGNED: out << "(signed)"; break;
      case ___UNSIGNED: out << "(unsigned)"; break;
    }
    std::ios::fmtflags f(out.flags());
    out << std::hex << std::setfill('0');
    for(int i = 0; i < size; i++) {
      out << std::setw(8) << data[i];
    }
    out.flags(f);
  }
  void MaceKeyDiff::printNode(PrintNode& pr, const std::string& name) const {
    pr.addChild(PrintNode(name, "MaceKeyDiff", toString()));
  }
}

mace::MaceKeyDiff mace::MaceKeyDiff::NEG_INF = mace::MaceKeyDiff(_NEG_INFINITY);
mace::MaceKeyDiff mace::MaceKeyDiff::INF = mace::MaceKeyDiff(_INFINITY);
const mace::MaceKey mace::MaceKey::null = mace::MaceKey();
const mace::KeyRange mace::KeyRange::null = mace::KeyRange();
