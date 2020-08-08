/* 
 * Message.h : part of the Mace toolkit for building distributed systems
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
#include "Serializable.h"

#ifndef __MESSAGE_H
#define __MESSAGE_H

/**
 * \file Message.h
 * \brief defines a basic type for a Message in Mace
 */

namespace mace {

/**
 * \brief base class for all Messages in Mace
 *
 * essentially states that messages are just Serializable and Printable, and
 * have a message number.
 */
struct InternalMessage_type{};
class Message : public Serializable, virtual public Printable {
  public:
  virtual uint8_t getType() const = 0; ///< return the message type number

  static uint8_t getType(const std::string& sm) {
    uint8_t type;
    memcpy(&type, sm.data(), sizeof(uint8_t));
    return type;
  }

  Message(): serializedByteSize(0){

  }

  size_t getSerializedSize() const {
    if (serializedByteSize == 0 && serializedCache.empty()) {
      serialize(serializedCache);
    }
    return serializedByteSize;
  }
    
  std::string serializeStr() const {
    if (serializedCache.empty()) {
      serialize(serializedCache);
    }
    return serializedCache;
  }
  void deserializeStr(const std::string& __s) throw (mace::SerializationException) {
    serializedCache = __s;
    Serializable::deserializeStr(__s);
  }
  protected:
  mutable size_t serializedByteSize;
  mutable std::string serializedCache;
};

const mace::InternalMessage_type imsg = mace::InternalMessage_type();

}
#endif
