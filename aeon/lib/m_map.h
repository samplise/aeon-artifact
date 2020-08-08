/* 
 * m_map.h : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, Charles Killian, Dejan Kostic, Ryan Braud, James W. Anderson, John Fisher-Ogden, Calvin Hubble, Duy Nguyen, Justin Burke, David Oppenheimer, Amin Vahdat, Adolfo Rodriguez, Sooraj Bhat, Karthik Nagaraj
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
#include "CollectionSerializers.h"
#include "RandomUtil.h"
#include "mace_constants.h"
#include "StrUtilNamespace.h"

#ifndef _MACE_MAP_H
#define _MACE_MAP_H

/**
 * \file m_map.h
 * \brief defines the mace::map extension of std::map
 */

namespace mace {

/**
 * \addtogroup Collections
 * @{
 */

/// mace::map extends std::map, but has support for printing and serialization, random element retrieval, and containsKey
/**
 * For most documentation, you should refer to the SGI STL documentation
 * http://www.sgi.com/tech/stl
 */
template<class Key, 
         class Data, 
         class Serial = SerializeMap<Key, Data>, 
         class Compare = std::less<Key>, 
         class Alloc = std::allocator<std::pair<const Key, Data> > >
class map : public std::map<Key, Data, Compare, Alloc>, public virtual Serial, public PrintPrintable {

  BOOST_CLASS_REQUIRE2(Serial, Serializer, boost, ConvertibleConcept);

public:
  typedef typename std::map<Key, Data, Compare, Alloc> baseType;
  typedef typename baseType::iterator iterator;
  typedef typename baseType::const_iterator const_iterator;
  typedef typename baseType::const_reverse_iterator const_reverse_iterator;
  typedef typename baseType::size_type size_type;
  typedef typename baseType::key_compare key_compare;

public:
  map() : baseType() {
  }

  map(const key_compare& comp) : baseType(comp) {
  }

  map(const const_iterator& b, const const_iterator& e) : baseType(b, e) {
  }

  map(const const_reverse_iterator& b, const const_reverse_iterator& e) : baseType(b, e) {
  }

  map(const std::map<Key, Data, Compare, Alloc>& m) : baseType(m) {
  }

  virtual ~map() { }

  /// remove all elements from the map.
  void clear() {
    baseType::clear();
  }
  
  /// number of elements in the map.
  size_t size() const {
    return baseType::size();
  }
  
  /// whether key \c k is in the map.  Equivalent to find(k) != end() (Mace extension)
  bool containsKey(const Key& k) const {
    return baseType::find(k) != baseType::end();
  } // containsKey

  /// retrieve data from key \c k.  Requires \c k already in map
  const Data& get(const Key& k) const {
//     static const Data defaultValue = Data();
    const_iterator i = baseType::find(k);
    if (i != this->end()) {
      return i->second;
    }
    ABORT("Key not in map");
//     return defaultValue;
  } // get

  /// retrieve data from key \c k.  Requires \c k already in map (Mace extension)
  Data& get(const Key& k) {
    iterator i = baseType::find(k);
    if (i != this->end()) {
      return i->second;
    }
    ABORT("Key not in map");
  } // get

  /// fills in data at key \c k, returns true if data found (Mace extension)
  bool get(const Key& k, Data& d) const {
    const_iterator i = baseType::find(k);
    if (i != this->end()) {
      d = i->second;
      return true;
    }
    return false;
  }

  /// put (k, d) into map. (Mace extension)
  void put(const Key& k, const Data& d) {
    (*this)[k] = d;
  }

  /// Support removal of elements from a const_iterator by re-looking up k.  (Called when the map is non-const but iterator is const). (Mace extension)
  const_iterator erase(const_iterator i) {
    Key k = i->first;
    i++;
    if (i == baseType::end()) {
      baseType::erase(k);
      return baseType::end();
    }
    else {
      Key next = i->first;
      baseType::erase(k);
      return baseType::find(next);
    }
  }

  /// erase iterator i
  void erase(iterator i) {
    baseType::erase(i);
  }

  /// erases any values with key \c k, returns the number of entries erased
  size_t erase(Key k) {
    return baseType::erase(k);
  }

  /// Return the template parameter string Key->Data for actual template parameters. (Mace extension)
  const std::string& getTypeName() const {
    const char* types[] = { "Key", "Data", "Serial", "Compare", "Alloc", 0 };
    static const StrUtilNamespace::StdStringList myTypes = StrUtilNamespace::getTypeFromTemplate(__PRETTY_FUNCTION__, types);
    static string ret = myTypes[0]+"->"+myTypes[1];
    return ret;
  }

  void print(std::ostream& out) const {
    if(mace::PRINT_TYPE_NAMES) {
      out << "mace::Map<" << getTypeName() << ">";
    }
    mace::printMap(out, baseType::begin(), baseType::end());
  }
  void printState(std::ostream& out) const {
    if(mace::PRINT_TYPE_NAMES) {
      out << "mace::Map<" << getTypeName() << ">";
    }
    mace::printMapState(out, baseType::begin(), baseType::end());
  }

  void printNode(PrintNode& pr, const std::string& name) const {
    mace::printMap(pr, name, "map<" + getTypeName() + ">", baseType::begin(), baseType::end());
  }

  /// return a random element, or end() if none. (Mace extension)
  const_iterator random() const {
    if(this->size()==0) { return this->end(); }
    int rand = RandomUtil::randInt(this->size());
    const_iterator i;
    int position;
    for(i = this->begin(), position=0; i != this->end() && position < rand; i++, position++);
    return i;
  }

  /// return a random element, or end() if none. (Mace extension)
  iterator random() {
    if(this->size()==0) { return this->end(); }
    int rand = RandomUtil::randInt(this->size());
    iterator i;
    int position;
    for(i = this->begin(), position=0; i != this->end() && position < rand; i++, position++);
    return i;
  }

  private:
  class v_const_iterator : public _SerializeMap<Key, Data>::const_iterator {
    private:
      const_iterator i;
      const_iterator end;

    public:
      bool isValid() {
        return i != end;
      }
      void next() {
        ASSERT(i != end);
        i++;
      }
      const Key& getKey() {
        return i->first;
      }
      const Data& getValue() {
        return i->second;
      }

      v_const_iterator(const_iterator beg, const_iterator end) : i(beg), end(end) {}
      ~v_const_iterator() {}
  };

  protected:
    typename _SerializeMap<Key, Data>::const_iterator* getIterator() const {
      return new v_const_iterator(this->begin(), this->end());
    }
    Data& insertKey(const Key& k) {
      return (*this)[k];
    }


}; // class map

/** @} */

}
#endif // _MACE_MAP_H
