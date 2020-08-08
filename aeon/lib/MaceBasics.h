/* 
 * MaceBasics.h : part of the Mace toolkit for building distributed systems
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
#ifndef MACE_BASICS_H
#define MACE_BASICS_H

/**
 * \file MaceBasics.h
 * \brief 
 * NOTICE: This file shall ONLY be used for things which have to do with
 *         simple constants and types.  
 *
 *  Including this file MUST NOT require inclusion of any STL or other mace
 *  headers.
 */

//#include <values.h>
#include <stdint.h>
#include <sys/types.h>
#include <limits>

/**
 * \brief Namespace for mace-specific types.
 *
 * As many types in Mace collide with types in the global or std namespace, we
 * put ours in the mace namespace.
 */
namespace mace {

/** 
 * \defgroup MaceKey address families
 *
 * \brief This is the set of values valid for return from MaceKey::addressFamily()
 */
/*@{*/

const int8_t UNDEFINED_ADDRESS = 0; ///< MaceKey of this type has no sub-type or helper.  MaceKey::isNullAddress() would return true.
const int8_t IPV4 = 1; ///< MaceKey of this type represents an IPV4 address
const int8_t SHA160 = 2; ///< MaceKey of this type represents a 160-bit address (generally based on the SHA1 of something)
const int8_t SHA32 = 3; ///< MaceKey of this type represents a 32-bit address (generally based on the SHA1 of something)
const int8_t STRING_ADDRESS = 4; ///< MaceKey of this type is a generic string
const int8_t CONTEXTNODE = 5; ///< MaceKey of this type represents an context node address
const int8_t VNODE = 6; ///< MaceKey of this type represents a virtual node
const int8_t NUM_ADDRESSES = 7; ///< The number of MaceKey address families.

/*@}*/

}

typedef int32_t channel_id_t; ///< Type used for sub-dividing APIs into channels.
/**
 * \brief Registration ids are how callers and callees identify each other.  
 *
 * When a higher level registers a Handler with a ServiceClass object, it
 * either passes in or receives a registration uid.  This id serves to identify
 * the \e link between the two objects.  The same registration uid is used
 * whether it is a call down into the ServiceClass, or a call from the
 * ServiceClass into the handler.  When a registration uid is passed into a
 * ServiceClass method, any callbacks or other side-effects as a result of that
 * call will be delivered to the registered Handler with the same registration
 * uid.  Similarly, the Handler can use the registration uid to tell what lower
 * level service delivered the upcall (in the event it is multiply registered).
 */
typedef int32_t registration_uid_t; 

using mace::UNDEFINED_ADDRESS;
using mace::IPV4;
using mace::SHA160;
using mace::SHA32;
using mace::STRING_ADDRESS;
using mace::CONTEXTNODE;
using mace::VNODE;

#endif
