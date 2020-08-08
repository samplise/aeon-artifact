/* 
 * BinaryLogObject.h : part of the Mace toolkit for building distributed systems
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

#ifndef __BASEBINARYOBJECT_H
#define __BASEBINARYOBJECT_H

typedef uint32_t log_id_t;

/**
 * \file BinaryLogObject.h
 * \brief declares the mace::BaseBinaryObject class
 */

namespace mace {

/**
 * \brief Defines a base class for all extensible binary logs for use with logreader.
 *
 * Initially conceived as the generic base class for all "Dummy services",
 * which are constructed along side each service, but without all the
 * functionality, extra services, and such.  It's primary purpose was to be
 * able to take the serialized state for a service, and deserialize it into an
 * object which was structured like a service.  It is registered with the log
 * reader class with a string that links the type of binary log to the
 * mace::BinaryLogObject object which knows how to deserialize the data.
 *
 * Now its use has been extended to supporting extensible binary logs other
 * than services, as a generic mechanism for implementing new structured logs.
 *
 * It derives from Serializable, so it can deserialize the log, and Printable,
 * so the binary logreader can print the state once deserialized.
 */
class BinaryLogObject : virtual public Serializable, virtual public Printable {
  public:
    enum LogClass { STATE_DUMP, SERVICE_BINLOG, STRUCTURE, STRING, PARAM, MATCHING, OTHER };
//    double timestamp;
//     unsigned int tid;
//     std::string selector;
public:
  uint64_t timestamp;
  log_id_t selector;
  uint32_t tid;
  
public:
  BinaryLogObject() { }
  ~BinaryLogObject() { }

  public: 
    virtual const std::string& getLogType() const = 0; ///< returns a unique short string - log type for including in binary log (e.g. "S" for string log)
    virtual const std::string& getLogDescription() const = 0; ///< returns a longer string description (e.g. "String" on string logs)
    virtual LogClass getLogClass() const = 0;
    virtual LogNode*& getChildRoot() const = 0;
//     virtual void openLogFiles() = 0;
};

}
#endif
