/* 
 * mlist.h : part of the Mace toolkit for building distributed systems
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
#include <list>
#include <iostream>
#include "CollectionSerializers.h"
#include "StrUtil.h"
#include "mace_constants.h"

#ifndef _MACE_LIST_H
#define _MACE_LIST_H

/**
 * \file mlist.h
 * \brief defines the mace::list extension of std::list
 */

namespace mace {

/**
 * \addtogroup Collections
 * @{
 */

/// mace::list extends std::list, but has support for printing and optional serialization
template<class T, class Serial = SerializeList<T>, class Alloc = std::allocator<T> >
class list : public std::list<T, Alloc>, public virtual Serial, public PrintPrintable {

  BOOST_CLASS_REQUIRE2(Serial, Serializer, boost, ConvertibleConcept);

public:
  typedef typename std::list<T, Alloc> baseType;
  typedef typename baseType::iterator iterator;
  typedef typename baseType::const_iterator const_iterator;
  typedef typename baseType::const_reverse_iterator const_reverse_iterator;
  typedef typename baseType::size_type size_type;

  list() : baseType() {
  }

  list(size_type n) : baseType(n) {
  }

  list(size_type n, const T& t) : baseType(n, t) {
  }

  list(const const_iterator& b, const const_iterator& e) : baseType(b, e) {
  }
  
  list(const const_reverse_iterator& b, const const_reverse_iterator& e) : baseType(b, e) {
  }

  list(const std::list<T, Alloc>& v) : baseType(v) {
  }

  virtual ~list() { }

  /// resizes the list, truncating or adding new default elements
  void resize(size_t sz) {
    baseType::resize(sz);
  }
  
  /// returns the size of the list
  size_t size() const {
    return baseType::size();
  }

  /// equivalent to std::list::push_back
  void add(const T& t) {
    return baseType::push_back(t);
  }

  /// Return the template parameter string Key->Data for actual template parameters. (Mace extension)
  const std::string& getTypeName() const {
    const char* types[] = { "T", "Serial", "Alloc", 0 };
    static const StrUtilNamespace::StdStringList myTypes = StrUtilNamespace::getTypeFromTemplate(__PRETTY_FUNCTION__, types);
    return myTypes[0];
  }

  void printNode(PrintNode& pr, const std::string& name) const {
    mace::printList(pr, name, "list<" + getTypeName() + ">", this->begin(),
		    this->end());
  }

  void print(std::ostream& out) const {
    if(mace::PRINT_TYPE_NAMES) {
      out << "list<"<< this->getTypeName() <<">";
    }
    printList(out, this->begin(), this->end());
  }

  void printState(std::ostream& out) const {
    if(mace::PRINT_TYPE_NAMES) {
      out << "list<"<<this->getTypeName() <<">";
    }
    printListState(out, this->begin(), this->end());
  }

  private:
  class v_iterator : public _SerializeList<T>::iterator {
    private:
      iterator i;
      iterator end;

    public:
      bool isValid() {
        return i != end;
      }
      void next() {
        ASSERT(i != end);
        i++;
      }
      T& getValue() {
        return *i;
      }

      v_iterator(iterator beg, iterator end) : i(beg), end(end) {}
      ~v_iterator() {}
  };

  class v_const_iterator : public _SerializeList<T>::const_iterator {
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
      const T& getValue() {
        return *i;
      }

      v_const_iterator(const_iterator beg, const_iterator end) : i(beg), end(end) {}
      ~v_const_iterator() {}
  };

  protected:
    typename _SerializeList<T>::iterator* getIterator() {
      return new v_iterator(this->begin(), this->end());
    }
    typename _SerializeList<T>::const_iterator* getIterator() const {
      return new v_const_iterator(this->begin(), this->end());
    }

};

/** @} */

}
#endif // _MACE_LIST_H
