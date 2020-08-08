/* 
 * mhash_set.h : part of the Mace toolkit for building distributed systems
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
#ifndef _MACE_HASH_SET_H
#define _MACE_HASH_SET_H

#if __GNUC__ >= 3
#  if __GNUC__ >= 4
#    if __GNUC_MINOR__ >= 3
#      include <tr1/unordered_set>
#      define __MACE_BASE_HSET__ std::tr1::unordered_set
#      define __MACE_BASE_HASH__ std::tr1::hash
#      define __MACE_USE_UNORDERED__
#    else 
#      include <ext/hash_set>
#      define __MACE_BASE_HSET__ __gnu_cxx::hash_set
#      define __MACE_BASE_HASH__ __gnu_cxx::hash
#    endif
#  else
#    include <ext/hash_set>
#    define __MACE_BASE_HSET__ __gnu_cxx::hash_set
#    define __MACE_BASE_HASH__ __gnu_cxx::hash
#  endif
#else
#  include <hash_set>
#  define __MACE_BASE_HSET__ __gnu_cxx::hash_set
#  define __MACE_BASE_HASH__ __gnu_cxx::hash
#endif

#include "CollectionSerializers.h"
#include "RandomUtil.h"
#include "mace_constants.h"
#include "StrUtilNamespace.h"

/**
 * \file mhash_set.h
 * \brief defines the mace::hash_set extension of std::hash_set
 */

namespace mace {

/**
 * \addtogroup Collections
 * @{
 */

/// mace::hash_set extends std::hash_set, adds printability and optional serialization, contains, and random
/**
 * For most documentation, you should refer to the SGI STL documentation
 * http://www.sgi.com/tech/stl
 */
template<class Key, 
         class Serial = SerializeSet<Key>,
         class HashFcn = __MACE_BASE_HASH__<Key>, 
         class EqualKey = std::equal_to<Key>, 
         class Alloc = std::allocator<Key> >
class hash_set : public __MACE_BASE_HSET__<Key, HashFcn, EqualKey, Alloc>, public virtual Serial, public PrintPrintable {

  BOOST_CLASS_REQUIRE2(Serial, Serializer, boost, ConvertibleConcept);

public:
  typedef typename __MACE_BASE_HSET__<Key, HashFcn, EqualKey, Alloc> baseType;
  typedef typename baseType::iterator iterator;
  typedef typename baseType::const_iterator const_iterator;
  typedef typename baseType::size_type size_type;

  virtual ~hash_set() { }

  /// removes all elements of the set
  void clear() {
    baseType::clear();
  }
  
  /// returns true if the set contains element \c k
  bool contains(const Key& k) const { 
    return this->find(k) != this->end(); 
  }

  void push_back(const Key& item) {
    // This is just so we can use XmlRpcArraySerialize
    insert(item);
  }

  size_t size() const {
    return baseType::size();
  }

  /// returns a random element of the set, or end() if empty
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
    const char* types[] = { "Key", "HashFcn", "EqualKey", "Alloc", 0 };
    static const StrUtilNamespace::StdStringList myTypes = StrUtilNamespace::getTypeFromTemplate(__PRETTY_FUNCTION__, types);
    return myTypes[0];
  }

  void print(std::ostream& out) const {
    if(mace::PRINT_TYPE_NAMES) {
      out << "mace::HashSet<" << getTypeName() << ">";
    }
    mace::printList(out, this->begin(), this->end());
  }
  void printState(std::ostream& out) const {
    if(mace::PRINT_TYPE_NAMES) {
      out << "mace::HashSet<" << getTypeName() << ">";
    }
    mace::printListState(out, this->begin(), this->end());
  }

  private:
  class v_const_iterator : public _SerializeSet<Key>::const_iterator {
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
      const Key& getValue() {
        return *i;
      }

      v_const_iterator(const_iterator beg, const_iterator end) : i(beg), end(end) {}
      ~v_const_iterator() {}
  };

  protected:
    typename _SerializeSet<Key>::const_iterator* getIterator() const {
      return new v_const_iterator(this->begin(), this->end());
    }
    void add(const Key& k) {
      this->insert(k);
    }

};

/** @} */

}
#endif // _MACE_HASH_SET_H
