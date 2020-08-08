/* 
 * AddressCache.h : part of the Mace toolkit for building distributed systems
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
#ifndef MACEDON_CACHE_H

#define MACEDON_CACHE_H
#define MAX_CACHE_SIZE 100
#define MAX_CACHE_LIFE 10 * 1000 * 1000 //usec

/**
 * \file AddressCache.h
 * \brief declares the mace::AddressCache class
 */

#include "MaceTypes.h"
#include "NodeCollection.h"
#include "Printable.h"
#include "TimeUtil.h"

namespace mace {

/// Entry in an address cache, contains an _id, a start and end for a range, and a timestamp it was refreshed.
class AddressCacheEntry : public mace::PrintPrintable, public mace::Serializable {
  private:
    MaceKey _id; ///< the address of the node described

  public:
    MaceKey start; ///< beginning of the address range -- inclusive
    MaceKey end; ///< end of the address range -- exclusive
    uint64_t time; ///< the time the entry was created or updated

    void print(std::ostream&) const;
    MaceKey getId() const { return _id; } ///< returns the node identifier
    double getScore() const { return time; } ///< the score is the time, to support a least-recently-used query (for cache eviction)
    AddressCacheEntry() : _id(), start(), end(), time() {}
    AddressCacheEntry(const MaceKey& id) : _id(id), start(), end(), time(TimeUtil::timeu()) {}
    AddressCacheEntry(const MaceKey& id, const MaceKey& b, const MaceKey& e) : _id(id), start(b), end(e), time(TimeUtil::timeu()) {}
    virtual ~AddressCacheEntry() {}
    /// serialization not supported, but required for NodeCollection.
    int deserialize(std::istream& in) throw(SerializationException) {
      ABORT("deserialize not supported");
    }
    /// serialization not supported, but required for NodeCollection.
    void serialize(std::string& str) const {
      ABORT("serialize not supported");
    }
  
};

/**
 * \brief Implements a cache for Bamboo, Pastry, or Chord, which tracks the address range of peers.  
 *
 * Used by the CacheRecursiveOverlayRoute service to remember frequently used
 * node's addresses, to support O(1) node lookups and communication.
 */
class AddressCache
{
public:
  AddressCache () : entries() {} 
  ~AddressCache () {}
  /**
   * \brief inserts entries into the address cache.
   *
   * Note: \f$ start = end \Rightarrow \infty \f$
   *
   * @param start the inclusive beginning of the address space
   * @param end the exclusive end of the address space
   * @param address the node's address
   */
  void insert(const MaceKey& start, const MaceKey& end, const MaceKey& address);
  /// Removes \c address from the cache
  void remove(const MaceKey& address);
  /// Return the address associated with a hash address (if any, MaceKey::null otherwise)
  MaceKey query(const MaceKey&);
  /// Ensure only the range of \c address contains hash \c hash
  void verify(const MaceKey& hash, const MaceKey& address);
private:
  typedef NodeCollection<AddressCacheEntry, MAX_CACHE_SIZE> cachemap;
  void consistency(); ///< removes stale cache entries
  cachemap entries;
};

}

#endif
