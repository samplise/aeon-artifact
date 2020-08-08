/* 
 * TransportHeader.h : part of the Mace toolkit for building distributed systems
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
#ifndef TRANSPORT_HEADER_H
#define TRANSPORT_HEADER_H

#include <deque>

#include "SockUtil.h"
#include "Serializable.h"

class TransportHeader : public mace::Serializable {
public:
  typedef std::deque<TransportHeader> HeaderList;

  static const uint8_t NAT_CONNECTION = 0x1;
  static const uint8_t ACCEPTANCE_TOKEN = 0x2;
  static const uint8_t SOURCE_ID = 0x4;
  static const uint8_t INTERNALMSG = 0x8;

public:
  TransportHeader() :
    src(SockUtil::NULL_MACEADDR), dest(SockUtil::NULL_MACEADDR),
    rid(0), flags(0), size(0) { }

  TransportHeader(const MaceAddr& src, const MaceAddr& dest,
		  registration_uid_t rid, uint8_t flags,
		  uint32_t size) :
    src(src), dest(dest), rid(rid), flags(flags), size(size)
    { }
  virtual ~TransportHeader() { }

  void serialize(std::string& s) const {
    serialize(s, src, dest, rid, flags, size);
  } // serialize

  static void serialize(std::string& s, const MaceAddr& src, const MaceAddr& dest,
			registration_uid_t rid, uint8_t flags, uint32_t size) {
    mace::serialize(s, &src);
    mace::serialize(s, &dest);
    mace::serialize(s, &rid);
    mace::serialize(s, &flags);
    mace::serialize(s, &size);
  } // serialize

  int deserialize(std::istream& in) throw (mace::SerializationException) {
    int o = 0;
    o += mace::deserialize(in, &src);
    o += mace::deserialize(in, &dest);
    o += mace::deserialize(in, &rid);
    o += mace::deserialize(in, &flags);
    o += mace::deserialize(in, &size);
    return o;
  } // deserialize

  // return the serialized size in bytes
  static size_t ssize() {
    static size_t ths = 0;
    if (ths == 0) {
      TransportHeader t;
      std::string s;
      t.serialize(s);
      ths = s.size();
    }
    return ths;
  } // ssize

  static uint8_t deserializeFlags(const std::string& s) {
    uint8_t f = 0;
    memcpy(&f, s.data() + ssize() - sizeof(uint32_t) - sizeof(f), sizeof(f));
    return f;
  } // deserializeFlags

  static uint32_t deserializeSize(const std::string& s) { //total size past the transport header
    uint32_t ns = 0;
    memcpy(&ns, s.data() + ssize() - sizeof(ns), sizeof(ns));
    return ntohl(ns);
  } // deserializeSize

  static void serialize(const MaceAddr& a, std::string& s) {
    a.serialize(s);
  } // serialize

  MaceAddr src;
  MaceAddr dest;
  registration_uid_t rid;
  uint8_t flags;
  uint32_t size;
}; // TransportHeader

#endif // TRANSPORT_HEADER_H
