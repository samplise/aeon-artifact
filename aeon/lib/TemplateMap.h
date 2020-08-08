/* 
 * TemplateMap.h : part of the Mace toolkit for building distributed systems
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
#ifndef _TEMPLATE_MAP_H
#define _TEMPLATE_MAP_H

#include <cassert>
#include "basemap.h"
#include "massert.h"
#include "ScopedLog.h"

/**
 * \file TemplateMap.h
 * \todo JWA, will you doc this?
 */

namespace mace {

/**
 * \addtogroup Collections
 * @{
 */

/// TemplateMap class -- undocumented
template<typename T>
class TemplateMap : public T,
		    public virtual Map<typename T::key_type,
				       typename T::mapped_type> {
public:
  typedef typename T::key_type Key;
  typedef typename T::mapped_type Data;
  typedef Map<Key, Data> baseMap;
  typedef typename baseMap::const_iterator const_iterator;
  typedef typename baseMap::inner_iterator inner_iterator;
  typedef typename T::const_iterator base_const_iterator;

public:
  class _inner_iterator : public virtual inner_iterator {
  friend class TemplateMap<T>;

  public:
    typedef typename baseMap::ipair ipair;
    typedef typename baseMap::const_key_ipair const_key_ipair;

  private:
    mutable base_const_iterator i;

  private:
    _inner_iterator(base_const_iterator _i) : i(_i) { }

  protected:
    const const_key_ipair* arrow() const {
      return &(*i);
    }

    const const_key_ipair& star() const {
      return *i;
    }

    void next() const {
      i++;
    }

    void prev() const {
      i--;
    }

    bool equals(const inner_iterator& right) const {
      return i == dynamic_cast<const _inner_iterator&>(right).i;
    }

    void assign(const inner_iterator& other) {
      i = dynamic_cast<const _inner_iterator&>(other).i;
    }

    inner_iterator* clone() const {
      return new _inner_iterator(i);
    }

    void erase() {
      ABORT("erase not supported");
    }

    void close() const { }

  public:
    virtual ~_inner_iterator() { }
  }; // class _inner_iterator

  bool containsKey(const Key& k) const {
    static const bool SCOPED_LOG = params::get("MAP_SCOPED_LOG", false);
    ScopedLog slog(containslogstr, Log::DEFAULT_LEVEL, Log::NULL_ID, true, true,
		   SCOPED_LOG, false);
    return T::find(k) != T::end();
  } // containsKey

  bool get(const Key& k, Data& d) const {
    static const bool SCOPED_LOG = params::get("MAP_SCOPED_LOG", false);
    ScopedLog slog(getlogstr, Log::DEFAULT_LEVEL, Log::NULL_ID, true, true,
		   SCOPED_LOG, false);
    base_const_iterator i = T::find(k);
    if (i == T::end()) {
      return false;
    }
    d = i->second;
    return true;
  }

  void put(const Key& k, const Data& d) {
    static const bool SCOPED_LOG = params::get("MAP_SCOPED_LOG", false);
    ScopedLog slog(putlogstr, Log::DEFAULT_LEVEL, Log::NULL_ID, true, true,
		   SCOPED_LOG, false);
    (*this)[k] = d;
  }

  void erase(const Key& k) {
    static const bool SCOPED_LOG = params::get("MAP_SCOPED_LOG", false);
    ScopedLog slog(eraselogstr, Log::DEFAULT_LEVEL, Log::NULL_ID, true, true,
		   SCOPED_LOG, false);
    T::erase(k);
  }

  const_iterator erase(const_iterator i) {
    static const bool SCOPED_LOG = params::get("MAP_SCOPED_LOG", false);
    ScopedLog slog(eraselogstr, Log::DEFAULT_LEVEL, Log::NULL_ID, true, true,
		   SCOPED_LOG, false);
    Key k = i->first;
    i++;
    if (i == end()) {
      T::erase(k);
      return end();
    }
    else {
      Key next = i->first;
      T::erase(k);
      return find(next);
    }
  }

  void sync() { }

  bool empty() const {
    return T::empty();
  }

  size_t size() const {
    return T::size();
  }

  void close() { }

  void clear() {
    static const bool SCOPED_LOG = params::get("MAP_SCOPED_LOG", false);
    ScopedLog slog(clearlogstr, Log::DEFAULT_LEVEL, Log::NULL_ID, true, true,
		   SCOPED_LOG, false);
    T::clear();
  }

  Map<Key, Data>& assign(const Map<Key, Data>& other) {
    T::operator=(dynamic_cast<const T&>(other));
    return *this;
  }

  const_iterator begin() const {
    return const_iterator(typename const_iterator::IteratorPtr(new _inner_iterator(T::begin())));
  }

  const_iterator end() const {
    return const_iterator(typename const_iterator::IteratorPtr(new _inner_iterator(T::end())));
  }

  const_iterator find(const Key& k) const {
    return const_iterator(typename const_iterator::IteratorPtr(new _inner_iterator(T::find(k))));
  }

  const_iterator lower_bound(const Key& k) const {
    return const_iterator(typename const_iterator::IteratorPtr(new _inner_iterator(T::lower_bound(k))));
  }

  bool equals(const baseMap& other) const {
    return *this == dynamic_cast<const T&>(other);
  }

  void setName(const std::string& n) {
    name = n;
    containslogstr = "MAP::" + name + "::contains";
    getlogstr = "MAP::" + name + "::get";
    putlogstr = "MAP::" + name + "::put";
    eraselogstr = "MAP::" + name + "::erase";
    clearlogstr = "MAP::" + name + "::clear";
  }
  
  protected:
  std::string name;
  std::string containslogstr;
  std::string getlogstr;
  std::string putlogstr;
  std::string eraselogstr;
  std::string clearlogstr;
}; // class TemplateMap

/** @} */

} // namespace mace

#endif // _TEMPLATE_MAP
