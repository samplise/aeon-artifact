/* 
 * Iterator.h : part of the Mace toolkit for building distributed systems
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
#include <map>
#include "massert.h"
// #define DEBUG_ITER
#ifdef DEBUG_ITER
#include "Log.h"
#endif

/**
 * \file Iterator.h
 * \brief Defines the mace::MapIterator and mace::ConstMapIterator Java-style iteration classes
 */


#ifndef _MACE_ITERATOR_H
#define _MACE_ITERATOR_H

namespace mace {

/// Java-style iteration for C++ collections (next/hasNext)
/**
 * implemented by always storing an iterator to the \e last item as well as the
 * \e next (unretrieved) item.  Calling next() advances the iterator, returning
 * a reference to the item.  Items can be removed using the remove() method,
 * which calls erase on the map with the \e last iterator.  Whether the
 * iteration reaches all elements will have to do with the properties of the
 * collection if you erase during iteration (for sorted collections, it will
 * generally work), but it shouldn't break in either circumstance.
 *
 * Example:
 * \code
 * mace::map<int, int> m;
 * //insert a bunch of members into m
 * mace::MapIterator<mace::map<int, int> > i(m);
 * while (i.hasNext()) {
 *   int& key;
 *   int& value = i.next(key);
 *   if (key != value) {
 *     i.remove();
 *   }
 * }
 * \endcode
 */
template <typename M>
class MapIterator
{
private:
  //   map<K, V>::iterator iter;
  //   map<K, V>::iterator *last;
  typedef typename M::iterator iterator;
  typedef typename M::const_iterator const_iterator;
  typedef typename M::mapped_type V;
  typedef typename M::key_type K;
  iterator iter;
  iterator last;
  //   map<K, V>& set;
  M& set;
  const iterator begin;
  const iterator end;

public:
  typedef typename M::mapped_type mapped_type;
  typedef typename M::key_type key_type;

  /// Constructs the map iterator from a map reference (iterator over the whole map)
  MapIterator(M& s): iter(s.begin()), set(s), begin(s.begin()), end(s.end()) {
    last = end;
#ifdef DEBUG_ITER
    Log::logf("MAP_ITER_DEBUG", "size: %d", set.size());
#endif
  }
  
  /// Constructs the map iterator from a map reference and a beginning point (iterates to end()) 
  MapIterator(M& s, iterator b): set(s), begin(b), end(s.end()), iter(b) {
    last = end;
#ifdef DEBUG_ITER
    Log::logf("MAP_ITER_DEBUG", "size: %d", set.size());
#endif
  }
  
  /// Constructs the map iterator from a map reference, a beginning point, and an ending point
  MapIterator(M& s, iterator b, iterator e): iter(b), set(s), begin(b), end(e) {
    last = end;
#ifdef DEBUG_ITER
    Log::logf("MAP_ITER_DEBUG", "size: %d", set.size());
#endif
  }

  /// returns whether there are more elements
  bool hasNext() {
    return iter != end && iter != set.end();
  }

  /// returns the present (next) item by reference, advancing the iterators
  V& next() {
    ASSERT(hasNext());
#ifdef DEBUG_ITER
    Log::logf("MAP_ITER_DEBUG", "next(): iter %x", iter->first);
#endif
    last = iter;
    iter++;
    return last->second;
  }

  /// returns the present (next) item by reference, and makes a copy of the key into the \c key reference, advancing the iterators
  V& next(K &key) {
    ASSERT(hasNext());
#ifdef DEBUG_ITER
    Log::logf("MAP_ITER_DEBUG", "next(): iter %x", iter->first);
#endif
    last = iter;
    iter++;
    key = last->first;
    return last->second;
  }

  /// removes the item last retrieved by next()
  void remove() {
#ifdef DEBUG_ITER
    Log::logf("MAP_ITER_DEBUG", "remove(): last %x", last->first);
#endif
    if(last != end) {
      set.erase(last);
      last = end;
    } else {
      ASSERT(false);
    }
  }

};

/// Java-style const iteration for C++ collections (next/hasNext)
/**
 * implemented by always storing an iterator to the \e last item as well as the
 * \e next (unretrieved) item.  Calling next() advances the iterator, returning
 * a reference to the item.   
 *
 * Example:
 * \code
 * mace::map<int, int> m;
 * //insert a bunch of members into m
 * mace::ConstMapIterator<mace::map<int, int> > i(m);
 * while (i.hasNext()) {
 *   int& key;
 *   int& value = i.next(key);
 *   maceout << "Key " << key << " -> Value " << value << Log::endl;
 * }
 * \endcode
 */
  template <typename M>
class ConstMapIterator
{
private:
  //   map<K, V>::iterator iter;
  //   map<K, V>::iterator *last;
  typedef typename M::iterator iterator;
  typedef typename M::const_iterator const_iterator;
  typedef typename M::mapped_type V;
  typedef typename M::key_type K;
  const_iterator iter;
  const_iterator last;
  //   map<K, V>& set;
  const M& set;
  const const_iterator begin;
  const const_iterator end;

public:
  typedef typename M::mapped_type mapped_type;
  typedef typename M::key_type key_type;
  /// Constructs the map iterator from a map reference (iterator over the whole map)
  ConstMapIterator(const M& s): set(s), begin(s.begin()), end(s.end()) {
    iter = begin;
    last = end;
#ifdef DEBUG_ITER
    Log::logf("MAP_ITER_DEBUG::const", "size: %d", set.size());
#endif
  }
  
  /// Constructs the map iterator from a map reference and a beginning point (iterates to end()) 
  ConstMapIterator(const M& s, const_iterator b): set(s), begin(b), end(s.end()) {
    iter = begin;
    last = end;
#ifdef DEBUG_ITER
    Log::logf("MAP_ITER_DEBUG::const", "size: %d", set.size());
#endif
  }
  
  /// Constructs the map iterator from a map reference, a beginning point, and an ending point
  ConstMapIterator(const M& s, const_iterator b, const_iterator e): set(s), begin(b), end(e) {
    iter = begin;
    last = end;
#ifdef DEBUG_ITER
    Log::logf("MAP_ITER_DEBUG::const", "size: %d", set.size());
#endif
  }

  /// returns whether there are more elements
  bool hasNext() {
    return iter != end && iter != set.end();
  }

  /// returns the present (next) item by const reference, advancing the iterators
  const V& next() {
    ASSERT(hasNext());
#ifdef DEBUG_ITER
    Log::logf("MAP_ITER_DEBUG::const", "next(): iter %x", iter->first);
#endif
    last = iter;
    iter++;
    return last->second;
  }

  /// returns the present (next) item by const reference, and makes a copy of the key into the \c key reference, advancing the iterators
  const V& next(K &key) {
    ASSERT(hasNext());
#ifdef DEBUG_ITER
    Log::logf("MAP_ITER_DEBUG::const", "next(): iter %x", iter->first);
#endif
    last = iter;
    iter++;
    key = last->first;
    return last->second;
  }

};

}

#endif //_MACE_ITERATOR_H
