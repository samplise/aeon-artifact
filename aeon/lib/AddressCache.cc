/* 
 * AddressCache.cc : part of the Mace toolkit for building distributed systems
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
#include <ostream>
#include "AddressCache.h"
#include "KeyRange.h"
// #include "mace.h"

namespace mace {

void AddressCacheEntry::print(std::ostream& out) const 
{
  out << "ACE{" << _id << " = [ " << start << " , " << end << " )}";
}

void AddressCache::consistency()
{
  uint64_t now = TimeUtil::timeu();
  cachemap::map_iterator iter = entries.mapIterator();
  while(iter.hasNext()) {
    AddressCacheEntry& ent = iter.next();
    if (now > ent.time + MAX_CACHE_LIFE) {
#ifdef CACHE_TRACE
      Log::log("cache_trace") << "Nuking stale " << ent << Log::endl;;
#endif
      iter.remove();
    }
  }
  return;
}

void AddressCache::insert(const MaceKey& start, const MaceKey& end, const MaceKey& address) 
{
  consistency();

  //Delete overlapping entries
  cachemap::map_iterator iter = entries.mapIterator();
  while(iter.hasNext()) {
    AddressCacheEntry& ent = iter.next();
    if(KeyRange::containsKey(start, ent.start, ent.end) || KeyRange::containsKey(ent.start, start, end)) {
#ifdef CACHE_TRACE
      Log::log("cache_trace") << "(Nuking) Overlapping mappings " << ent << Log::endl;
#endif
      iter.remove();
    }
  }

  if(entries.contains(address)) {
    AddressCacheEntry& ent = entries.get(address);
    if(ent.start != start || ent.end != end) {
      ent.start = start; 
      ent.end = end;
      ent.time = TimeUtil::timeu();
#ifdef CACHE_TRACE
      Log::log("cache_trace") << "Updating " << address << " = [ " << start << " , " << end << " )" << Log::endl;
#endif
      return;
    }
  }
  if (!entries.space()) {
    //Throw out oldest
    entries.erase(entries.leastScore<double>());
  }
#ifdef CACHE_TRACE
  Log::log("cache_trace") << "Adding " << address << " = [ " << start << " , " << end << " )" << Log::endl;
#endif
  entries.add(AddressCacheEntry(address, start, end));
  return;
}

MaceKey AddressCache::query(const MaceKey& query)
{
  //   static MaceKey null;
  //   return null;
  consistency();
  cachemap::map_iterator iter = entries.mapIterator();
  while(iter.hasNext()) {
    AddressCacheEntry& ent = iter.next();
    if(KeyRange::containsKey(query, ent.start, ent.end)) {
#ifdef CACHE_TRACE
      Log::log("cache_trace") << "Using mapping " << ent << Log::endl;
#endif
      return ent.getId();
    }
  }
  return MaceKey::null;
}

void AddressCache::remove(const MaceKey& address)
{
  entries.erase(address);
}

void AddressCache::verify(const MaceKey& hash, const MaceKey& address)
{
  consistency();
  cachemap::map_iterator iter = entries.mapIterator();
  while(iter.hasNext()) {
    AddressCacheEntry& ent = iter.next();
    if(KeyRange::containsKey(hash, ent.start, ent.end)) {
      if(ent.getId() != address) {
        iter.remove();
      }
      return;
    }
  }
}

}
