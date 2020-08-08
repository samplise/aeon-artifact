/* 
 * mmultimap.h : part of the Mace toolkit for building distributed systems
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
#include "mdeque.h"
#include "mpair.h"
#include "CollectionSerializers.h"
#include "RandomUtil.h"
#include "mace_constants.h"
#include "StrUtil.h"

/**
 * \file mmultimap.h
 * \brief defines the mace::multimap extension of std::multimap
 */

#ifndef _MACE_MULTIMAP_H
#define _MACE_MULTIMAP_H

namespace mace {

/**
 * \addtogroup Collections
 * @{
 */

  /// mace::multimap extends std::multimap, with support for printing and optional serialization, containsKey, get, and random
  /**
   * For most documentation, you should refer to the SGI STL documentation
   * http://www.sgi.com/tech/stl
   */
  template<class Key, class Data, class Serial = SerializeMap<Key, Data>, class Compare = std::less<Key>, class Alloc = std::allocator<std::pair<const Key, Data> > >
class multimap : public std::multimap<Key, Data, Compare, Alloc>, public virtual Serial, public PrintPrintable {

  BOOST_CLASS_REQUIRE2(Serial, Serializer, boost, ConvertibleConcept);

public:
  typedef typename std::multimap<Key, Data, Compare, Alloc> baseType;
  typedef typename baseType::iterator iterator;
  typedef typename baseType::const_iterator const_iterator;
  typedef typename baseType::const_reverse_iterator const_reverse_iterator;
  typedef typename baseType::size_type size_type;
  typedef typename baseType::key_compare key_compare;

  multimap() : baseType() {
  }

  multimap(const key_compare& comp) : baseType(comp) {
  }

  multimap(const const_iterator& b, const const_iterator& e) : baseType(b, e) {
  }

  multimap(const const_reverse_iterator& b, const const_reverse_iterator& e) : baseType(b, e) {
  }
  
  multimap(const std::multimap<Key, Data, Compare, Alloc>& m) : baseType(m) {
  }

  virtual ~multimap() { }

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
    return baseType::find(k) != this->end();
  } // containsKey

  /// retrieve data from key \c k as a possibly empty deque of items (one for each match)
  deque<Data> get(const Key& k) const {
    deque<Data> l;
    for (const_iterator i = baseType::find(k); i != this->end() && i->first == k; i++) {
      l.push_back(i->second);
    }
    return l;
  } // get
  
  /// return a random element of the multimap
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
    const char* types[] = { "Key", "Data", "Compare", "Alloc", 0 };
    static const StrUtilNamespace::StdStringList myTypes = StrUtilNamespace::getTypeFromTemplate(__PRETTY_FUNCTION__, types);
    static string ret = myTypes[0]+"->"+myTypes[1];
    return ret;
  }

  void print(std::ostream& out) const {
    if(mace::PRINT_TYPE_NAMES) {
      out << "mace::MultiMap<" << getTypeName() << ">";
    }
    mace::printMap(out, this->begin(), this->end());
  }
  void printNode(PrintNode& pr, const std::string& name) const {
    mace::printMap(pr, name, "multimap<" + getTypeName() + ">", this->begin(), this->end());
  }
  void printState(std::ostream& out) const {
    if(mace::PRINT_TYPE_NAMES) {
      out << "mace::MultiMap<" << getTypeName() << ">";
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
      iterator i = this->insert(std::pair<Key, Data>(k, Data()));
      return i->second;
    }

};

/** @} */

}
#endif // _MACE_MULTIMAP_H
