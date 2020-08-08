/* 
 * mset.h : part of the Mace toolkit for building distributed systems
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
#ifndef _MACE_SET_H
#define _MACE_SET_H

#include <set>

#include "CollectionSerializers.h"
#include "RandomUtil.h"
#include "mace_constants.h"
#include "StrUtilNamespace.h"

/**
 * \file mset.h
 * \brief defines the mace::set extension of std::set
 */

namespace mace {

/**
 * \addtogroup Collections
 * @{
 */

/// mace::set extends std::set, adds printability and optional serialization, contains, containsKey, set contains, and random element
/**
 * For most documentation, you should refer to the SGI STL documentation
 * http://www.sgi.com/tech/stl
 */
template<class Key, class Serial = SerializeSet<Key>, class LessFcn = std::less<Key>, class Alloc = std::allocator<Key> >
class set : public std::set<Key, LessFcn, Alloc>, public virtual Serial, public PrintPrintable {

  BOOST_CLASS_REQUIRE2(Serial, Serializer, boost, ConvertibleConcept);

public:
  typedef typename std::set<Key, LessFcn, Alloc> baseType;
  typedef typename baseType::iterator iterator;
  typedef typename baseType::const_iterator const_iterator;
  typedef typename baseType::const_reverse_iterator const_reverse_iterator;
  typedef typename baseType::size_type size_type;
  typedef typename baseType::key_compare key_compare;

  set() : baseType() {
  }

  set(const key_compare& comp) : baseType(comp) {
  }

  template<class InputIterator> set(InputIterator f, InputIterator l) : baseType(f,l) { }
  template<class InputIterator> set(InputIterator f, InputIterator l, const key_compare& comp) : baseType(f,l,comp) { }

  set(const std::set<Key, LessFcn, Alloc>& s) : baseType(s) {
  }

  virtual ~set() { }

  /// removes all elements of the set
  void clear() {
    baseType::clear();
  }
  
  /// returns the size of the set
  size_t size() const {
    return baseType::size();
  }

  /// returns find(k) != this->end()
  bool contains(const Key& k) const { 
    return baseType::find(k) != this->end(); 
  }
  /// equivalent to contains(const Key&)
  bool containsKey(const Key& k) const { 
    return contains(k);
  }
  /// set contains - if this set contains all elements of other, using ordered property to make efficient
  bool contains(const baseType& other) const {
    typename baseType::const_iterator myi = this->begin();
    typename baseType::const_iterator i = other.begin();
    while (i != other.end() && myi != this->end()) {
      if (*myi < *i) {
	myi++;
      }
      else if (*myi == *i) {
	myi++;
	i++;
      }
      else {
	return false;
      }
    }
    return i == other.end();
  }

  void push_back(const Key& item) {
    // This is just so we can use XmlRpcArraySerialize
    this->insert(item);
  }

  /// return a random element, or end() if empty
  const_iterator random() const {
    if(this->size()==0) { return this->end(); }
    uint32_t rand = RandomUtil::randInt(this->size());
    return get(rand);
  }

  /// return a random element, or end() if empty
  iterator random() {
    if(this->size()==0) { return this->end(); }
    uint32_t rand = RandomUtil::randInt(this->size());
    return get(rand);
  }

  const_iterator get(uint32_t pos) const {
    const_iterator i = this->begin();
    uint32_t cur = 0;
    while (i != this->end() && cur < pos) {
      i++;
      cur++;
    }
    return i;
  }

  /// Return the template parameter string Key->Data for actual template parameters. (Mace extension)
  const std::string& getTypeName() const {
    const char* types[] = { "Key", "Serial", "LessFcn", "Alloc", 0 };
    static const StrUtilNamespace::StdStringList myTypes = StrUtilNamespace::getTypeFromTemplate(__PRETTY_FUNCTION__, types);
    return myTypes[0];
  }

  void print(std::ostream& out) const {
    if(mace::PRINT_TYPE_NAMES) {
      out << "mace::Set<" << getTypeName() << ">";
    }
    mace::printList(out, this->begin(), this->end());
  }

  void printNode(PrintNode& pr, const std::string& name) const {
    mace::printList(pr, name, "set<" + getTypeName() + ">", this->begin(),
		    this->end());
  }

  void printState(std::ostream& out) const {
    if(mace::PRINT_TYPE_NAMES) {
      out << "mace::Set<" << getTypeName() << ">";
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
