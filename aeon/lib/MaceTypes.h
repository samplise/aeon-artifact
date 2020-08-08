/* 
 * MaceTypes.h : part of the Mace toolkit for building distributed systems
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
#ifndef _MACE_TYPES_H
#define _MACE_TYPES_H

/**
 * \file MaceTypes.h
 * \brief some common includes, and placing some types in the global namespace
 */

#include "MaceBasics.h"

#include "MaceKey.h"
#include "KeyRange.h"
// #include "mhash_set.h"
// #include "mset.h"
// #include "mpair.h"

using mace::MaceAddr;
using mace::SockAddr;
using mace::MaceKey;
using mace::ipv4;
using mace::sha32;
using mace::sha160;
using mace::string_key;
using mace::KeyRange;
using mace::vnode;
using mace::ctxnode;



/**
 * \addtogroup Collections
 * @{
 */
// typedef mace::hash_set<MaceKey> NodeSet;
typedef mace::set<MaceKey> NodeSet; ///< basic type for a set of MaceKey objects, commonly to represent a set of nodes
// #define NodeSet mace::set<MaceKey>
typedef mace::deque<MaceKey> NodeList;
/** @} */

#endif // _MACE_TYPES_H
