/* 
 * Collections.h : part of the Mace toolkit for building distributed systems
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
#ifndef COLLECTIONS_H
#define COLLECTIONS_H

#include <boost//algorithm/string.hpp>

#include "mstring.h"
#include "mpair.h"
#include "mset.h"
#include "m_map.h"
#include "mdeque.h"
#include "mhash_set.h"
#include "mhash_map.h"
#include "hash_string.h"

/**
 * \file Collections.h
 * \brief defines a variety of common instantiated template types.
 */

/**
 * \addtogroup Collections
 * @{
 */

typedef mace::map<mace::string, mace::string> StringMap; ///< Sorted map from string to string
typedef mace::hash_map<mace::string, mace::string> StringHMap; ///< Hash map from string to string
typedef mace::hash_map<mace::string, int, mace::SerializeMap<mace::string, int>, hash_string> StringIntHMap; ///< Hash map from string to int
typedef mace::hash_map<int, mace::string> IntStringHMap; ///< Hash map from int to string
// typedef mace::hash_set<mace::string> StringSet;
typedef mace::set<mace::string> StringSet; ///< Set of strings
typedef mace::deque<mace::string> StringList; ///< Deque of strings
typedef mace::pair<mace::string, mace::string> StringPair; ///< Pair of strings
// #define StringMap mace::map<mace::string, mace::string>
// #define StringHMap mace::hash_map<mace::string, mace::string>
// #define StringIntHMap mace::hash_map<mace::string, int, hash_string>
// #define IntStringHMap mace::hash_map<int, mace::string>
// #define StringSet mace::hash_set<mace::string>
// #define SStringSet mace::set<mace::string>
// #define StringList mace::deque<mace::string>
// #define StringPair mace::pair<mace::string, mace::string>

namespace mace {
    inline StringSet makeStringSet(const std::string& input, const std::string& delim) {
        StringSet s;
        if (!input.empty()) {
            //         std::set<std::string> s;
            boost::split(s, input, boost::is_any_of(delim));
        }
        return s;
    }
}

/** @} */

#endif // COLLECTIONS_H
