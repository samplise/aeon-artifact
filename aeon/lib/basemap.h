/* 
 * basemap.h : part of the Mace toolkit for building distributed systems
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
#ifndef _BASE_MAP_H
#define _BASE_MAP_H

#include <boost/shared_ptr.hpp>
#include "Printable.h"

/**
 * \file basemap.h
 * \brief Implements generic Map related functionality.
 *
 * Includes both mace::Map, the generic Map handling both BDB and memory map,
 * and the ScopedStorage class, which handles map lookup and storage in
 * constructor/destructor.
 *
 * \todo JWA, will you document this file?
 */

namespace mace {

/**
 * \addtogroup Collections
 * @{
 */

/// Map class -- undocumented
template<class Key, class Data>
class Map : public virtual Printable {

public:
/**
 * \addtogroup Scoped
 * \brief The scoped tools all take advantage of begin and end behaviors in the constructor and destructor of objects.
 * @{
 */

  /**
   * \brief Scoped storage for map lookup and storage.
   *
   * \todo incomplete docs.
   */
  class ScopedStorage {
    friend class Map<Key, Data>;
  public:
    ScopedStorage(Map<Key, Data>* m, const Key& k) :
      map(m), key(k), data(new Data()), hasData(false) {

      hasData = map->get(key, *data);
    }
    virtual ~ScopedStorage() {
      map->put(key, *data);
      delete data;
    }
    Data& operator*() {
      return *data;
    }
    Data* operator->() {
      return data;
    }
    bool found() const {
      return hasData;
    }

  protected:
    Map<Key, Data>* map;
    Key key;
    Data* data;
    bool hasData;
  }; // class ScopedStorage

/** @} */

public:
  typedef std::pair<Key, Data> ipair;
  typedef std::pair<const Key, Data> const_key_ipair;

public:
  class const_iterator;
  class inner_iterator {
    friend class Map<Key, Data>::const_iterator;
  protected:
    virtual const const_key_ipair* arrow() const = 0;
    virtual const const_key_ipair& star() const = 0;
    virtual void next() const = 0;
    virtual void prev() const = 0;
    virtual bool equals(const inner_iterator& right) const = 0;
//     virtual void assign(const inner_iterator& other) = 0;
//     virtual inner_iterator* clone() const = 0;
    virtual void erase() = 0;
    virtual void close() const = 0;
  public:
    virtual ~inner_iterator() { }
  }; // class inner_iterator

  class const_iterator {
  public:
    typedef boost::shared_ptr<inner_iterator> IteratorPtr;

  private:
    IteratorPtr inner;

  public:
    const_iterator() { }
    const_iterator(IteratorPtr i) : inner(i) { }
//     const_iterator(const const_iterator& other) : inner(other.inner) { }
    const const_key_ipair* operator->() const {
      return inner->arrow();
    }
    const const_key_ipair& operator*() const {
      return inner->star();
    }
    const const_iterator& operator++() const {
      inner->next();
      return *this;
    }
    const const_iterator& operator--() const {
      inner->prev();
      return *this;
    }
    const const_iterator& operator++(int _i) const {
      inner->next();
      return *this;
    }
    const const_iterator& operator--(int _i) const {
      inner->prev();
      return *this;
    }
    bool operator==(const const_iterator& right) const {
      return inner->equals(*(right.inner));
    }
    bool operator!=(const const_iterator& right) const {
      return !inner->equals(*(right.inner));
    }
    const_iterator& operator=(const const_iterator& other) {
      inner = other.inner;
//       if (inner == NULL) {
//         inner = IteratorPtr(other.inner->clone());
//       } 
//       else {
//         inner->assign(*(other.inner));
//       }
      return *this;
    }
    const_iterator& erase() {
      inner->erase();
      return *this;
    }
    void close() const {
      inner->close();
    }
  }; // class iterator

public:
  virtual ~Map() { }
  virtual bool get(const Key& kt, Data& d) const = 0;
  virtual bool containsKey(const Key& kt) const = 0;
  virtual void put(const Key& kt, const Data& d) = 0;
  virtual void erase(const Key& kt) = 0;
  virtual bool empty() const = 0;
  virtual size_t size() const = 0;
  virtual void sync() = 0;
  virtual void close() = 0;
  virtual const_iterator erase(const_iterator i) = 0;
  virtual const_iterator begin() const = 0;
  virtual const_iterator end() const = 0;
  virtual const_iterator find(const Key& kt) const = 0;
  virtual const_iterator lower_bound(const Key& kt) const = 0;
  virtual void clear() = 0;
  virtual bool equals(const Map& other) const = 0;
  virtual Map<Key, Data>& assign(const Map<Key, Data>& other) = 0;
  bool operator==(const Map<Key, Data>& other) const {
    return this->equals(other);
  }
  bool operator!=(const Map<Key, Data>& other) const {
    return !this->equals(other);
  }
  Map<Key, Data>& operator=(const Map<Key, Data>& other) {
    return this->assign(other);
  }
  virtual void copy(const Map<Key, Data>& other) {
    clear();
    add(other);
  }
  virtual void add(const Map<Key, Data>& other) {
    for (const_iterator i = other.begin(); i != other.end(); i++) {
      put(i->first, i->second);
    }
  }

}; // class Map

/** @} */

} // namespace mace

#endif // _BASE_MAP_H
