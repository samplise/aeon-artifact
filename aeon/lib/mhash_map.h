/* 
 * mhash_map.h : part of the Mace toolkit for building distributed systems
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
#ifndef _MACE_HASH_MAP_H
#define _MACE_HASH_MAP_H

#if __GNUC__ >= 3
#  if __GNUC__ >= 4
#    if __GNUC_MINOR__ >= 3
#      include <tr1/unordered_map>
#      define __MACE_BASE_HMAP__ std::tr1::unordered_map
#      define __MACE_BASE_HASH__ std::tr1::hash
#      define __MACE_USE_UNORDERED__
#    else 
#      include <ext/hash_map>
#      define __MACE_BASE_HMAP__ __gnu_cxx::hash_map
#      define __MACE_BASE_HASH__ __gnu_cxx::hash
#    endif
#  else
#    include <ext/hash_map>
#    define __MACE_BASE_HMAP__ __gnu_cxx::hash_map
#    define __MACE_BASE_HASH__ __gnu_cxx::hash
#  endif
#else
#  include <hash_map>
#  define __MACE_BASE_HMAP__ __gnu_cxx::hash_map
#  define __MACE_BASE_HASH__ __gnu_cxx::hash
#endif

#include "CollectionSerializers.h"
#include "Traits.h"
#include "RandomUtil.h"
#include "mace_constants.h"
#include "StrUtilNamespace.h"

/**
 * \file mhash_map.h
 * \brief defines the mace::hash_map extension of std::hash_map
 */

namespace mace {

/**
 * \addtogroup Collections
 * @{
 */

/// mace::hash_map extends std::hash_map, but provides Serialiable (optional), Printable, containsKey, get, and random
/**
 * For most documentation, you should refer to the SGI STL documentation
 * http://www.sgi.com/tech/stl
 */
template<class Key, class Data,
         class Serial = SerializeMap<Key, Data>,
	 class HashFcn = __MACE_BASE_HASH__<Key>,
	 class EqualKey = std::equal_to<Key>,
	 class Alloc = std::allocator<Data> >
class hash_map : public __MACE_BASE_HMAP__<Key, Data, HashFcn, EqualKey, Alloc>, public virtual Serial, public PrintPrintable {

  BOOST_CLASS_REQUIRE2(Serial, Serializer, boost, ConvertibleConcept);

public:
  typedef typename __MACE_BASE_HMAP__<Key, Data, HashFcn, EqualKey, Alloc> baseType;
  typedef typename baseType::iterator iterator;
  typedef typename baseType::const_iterator const_iterator;
  typedef typename baseType::size_type size_type;

  /// remove all elements from the map.
  void clear() {
    baseType::clear();
  }
  
  /// number of elements in the map.
  size_t size() const {
    return baseType::size();
  }
  
  virtual ~hash_map() { }

  /// whether key \c k is in the map.  Equivalent to find(k) != end() (Mace extension)
  bool containsKey(const Key& k) const {
    return this->find(k) != this->end();
  } // containsKey

  /// retrieve data from key \c k.  Requires \c k already in map
  const Data& get(const Key& k) const {
//     static const Data defaultValue = Data();
    const_iterator i = this->find(k);
    if (i != this->end()) {
      return i->second;
    }
    ASSERT(0);
//     return defaultValue;
  } // get
  
  /// retrieve data from key \c k.  Requires \c k already in map
  Data& get(const Key& k) {
    iterator i = this->find(k);
    if (i != this->end()) {
      return i->second;
    }
    ASSERT(0);
  } // get

  /// hash_map doesn't support lower_bound
  const_iterator lower_bound(const Key& k) const {
    // lower_bound not supported on hash_map
    ASSERT(0);
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

  /// Return the template parameter string Key->Data for actual template parameters. (Mace extension)
  const std::string& getTypeName() const {
    const char* types[] = { "Key", "Data", "Serial", "HashFcn", "EqualKey", "Alloc", 0 };
    static const StrUtilNamespace::StdStringList myTypes = StrUtilNamespace::getTypeFromTemplate(__PRETTY_FUNCTION__, types);
    static string ret = myTypes[0]+"->"+myTypes[1];
    return ret;
  }

  void printNode(PrintNode& pr, const std::string& name) const {
    mace::printMap(pr, name, "hash_map<" + getTypeName() + ">", baseType::begin(), baseType::end());
  }

  void print(std::ostream& out) const {
    if(mace::PRINT_TYPE_NAMES) {
      out << "mace::HashMap<" << getTypeName() << ">";
    }
    mace::printMap(out, this->begin(), this->end());
  }
  void printState(std::ostream& out) const {
    if(mace::PRINT_TYPE_NAMES) {
      out << "mace::HashMap<" << getTypeName() << ">";
    }
    mace::printMapState(out, this->begin(), this->end());
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
};

/** @} */

}
#endif // _MACE_HASH_MAP_H
