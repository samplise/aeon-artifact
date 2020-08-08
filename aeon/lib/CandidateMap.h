/* 
 * CandidateMap.h : part of the Mace toolkit for building distributed systems
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
#include "mhash_map.h"
#include "RandomUtil.h"
#include "Log.h"

/**
 * \file CandidateMap.h
 * \brief implements the mace::CandidateMap template class for use with RanSub (random summary population)
 */

#ifndef CANDIDATE_MAP_H
#define CANDIDATE_MAP_H 1

namespace mace {

/**
 * \addtogroup Collections
 * \brief Mace collections include a set of additions to and extensions of C++
 * STL collections.  
 *
 * The collections are in the mace:: namespace to avoid
 * colliding with the C++ collections.  They generally add serializability,
 * printability, random element selection, and a containsKey() method.
 * @{
 */

/**
 * \brief Provides support for candidate sets as described in the AMMO/RanSub papers.
 *
 * This is a Mace-specific collection type derived from mace::hash_map<>.
 *
 * A CandidateMap has functions for generating a population which is a
 * uniformly random subset of a larger population.  It does this by compacting
 * with other candidate maps, and using the size of the population represented
 * by each one to weigh in randomly to determine how likely it is to retain a
 * member of a given populaiton.  The result is that a properly constructed
 * CandidateMap should be a uniformly random subset of the populations of all
 * maps compacted with it.
 */
template<class Key, class Data, class HashFcn = __MACE_BASE_HASH__<Key>, class EqualKey = std::equal_to<Key>, class Alloc = std::allocator<Data> >
class CandidateMap : public mace::hash_map<Key, Data, SerializeMap<Key, Data>, HashFcn, EqualKey, Alloc> {

public:
  typedef typename mace::hash_map<Key, Data, SerializeMap<Key, Data>, HashFcn, EqualKey, Alloc> base_hash_map; ///< type of the map derived from (to actually implement functionality)
  typedef typename base_hash_map::iterator iterator;

  CandidateMap() : base_hash_map(), maxSize((uint32_t)-1), n(0) {
  }

  CandidateMap(uint32_t s) : base_hash_map(), maxSize(s), n(0) {
  } // CandidateMap

  virtual ~CandidateMap() { }

  /// the maximum number of candidates allowed in the map
  virtual void setMaxSize(uint32_t size) {
    maxSize = size;
  }

  /// reset the population size represented to 0
  void reset() {
    n = 0;
  } // reset

  /// returns the population size represented by the map
  uint32_t aggregateCount() {
    if (n < this->size()) {
      n = this->size();
    }
    return n;
  } // aggregateCount

  /// combine elements of \c cs into this map, keeping a weighted random subset of them both
  void compact(CandidateMap<Key, Data, HashFcn, EqualKey, Alloc> cs) {
    base_hash_map tmp;

    n += cs.size();

    // make sure that we are < maxSize
    while (this->size() > maxSize) {
      randErase();
    }

    while (cs.size() > cs.maxSize) {
      cs.randErase();
    }

    // adjust n
    if (n < this->size()) {
      n = this->size();
    }

    if (cs.n < cs.size()) {
      cs.n = cs.size();
    }

    uint32_t accum = 0;
    std::pair<Key, Data> v;
    int baseweight = n;
    int weight = cs.n;

    uint32_t total = this->size() + cs.size();
    if (total > maxSize) {
      total = maxSize;
    }

    while (accum < total) {
      bool rnd = RandomUtil::randInt(2, baseweight, weight);
      
      if (cs.empty() || (!this->empty() && !rnd)) {
	// BUG: Tsunami #1
	if (this->empty()) {
	  break;
	}
	v = randErase();
	baseweight--;
      }
      else {
	v = cs.randErase();
	weight--;
      }

      if (tmp.find(v.first) == tmp.end()) {
	accum++;
      }
      tmp[v.first] = v.second;
    }

    this->clear();

    for (iterator i = tmp.begin();
	 i != tmp.end(); i++) {
      this->operator[](i->first) = i->second;
    }
  } // compact

  void serialize(std::string &s) const throw(SerializationException) {
    base_hash_map::serialize(s);
    mace::serialize(s, &maxSize);
    mace::serialize(s, &n);
  } // serialize

  int deserialize(std::istream& in) throw(SerializationException) {
    unsigned offset = base_hash_map::deserialize(in);
    offset += mace::deserialize(in, &maxSize);
    offset += mace::deserialize(in, &n);
    return offset;
  } // deserialize

private:
  /// actually implements the functionality to randomly erase a single element in the set.
  std::pair<Key, Data> randErase() {
    uint32_t pos = RandomUtil::randInt(this->size());
    iterator i = this->begin();
    for (uint32_t count = 0; count < pos; count++, i++);
    std::pair<Key, Data> p = *i;
    this->erase(i);
    return p;
  } // randErase
  
private:
  uint32_t maxSize; ///< the max number of candidates allowed

  // total number of nodes processed so far
  uint32_t n; ///< the population size represented by this map
};

/** @} */

}
#endif // CANDIDATE_MAP_H
