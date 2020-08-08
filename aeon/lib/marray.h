/* 
 * marray.h : part of the Mace toolkit for building distributed systems
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
#include <vector>
#include "CollectionSerializers.h"
#include "StrUtil.h"
#include "mace_constants.h"

#ifndef _MACE_ARRAY_H
#define _MACE_ARRAY_H

/**
 * \file marray.h
 * \brief defines the mace::array extension of std::vector 
 */

namespace mace {

/**
 * \addtogroup Collections
 * @{
 */

/// mace::array extends std::vector, but is fixed in size, provides printing and serialization
/**
 * For most documentation, you should refer to the SGI STL documentation
 * http://www.sgi.com/tech/stl
 */
template<class T, int mysize, class Serial = SerializeArray<T>, class Alloc = std::allocator<T> >
class array : public std::vector<T, Alloc>, public virtual Serial, public PrintPrintable {

  BOOST_CLASS_REQUIRE2(Serial, Serializer, boost, ConvertibleConcept);

public:
  typedef typename std::vector<T, Alloc> baseType;
  typedef typename baseType::iterator iterator;
  typedef typename baseType::const_iterator const_iterator;
  typedef typename baseType::size_type size_type;

  array() : baseType(mysize) {
  }

  array(const T& t) : baseType(mysize, t) {
  }

  array(const array<T, mysize, Serial, Alloc>& v) : baseType(v) {
  }

  virtual ~array() { }

  /// Return the template parameter string Key->Data for actual template parameters. (Mace extension)
  const std::string& getTypeName() const {
    const char* types[] = { "T", "int mysize", "Alloc", 0 };
    static const StrUtilNamespace::StdStringList myTypes = StrUtilNamespace::getTypeFromTemplate(__PRETTY_FUNCTION__, types);
    return myTypes[0];
  }
  
  void print(std::ostream& out) const {
    if(mace::PRINT_TYPE_NAMES) {
      out << "array<"<< this->getTypeName() << "," << mysize << ">";
    }
    printList(out, this->begin(), this->end());
  }
  void printState(std::ostream& out) const {
    if(mace::PRINT_TYPE_NAMES) {
      out << "array<"<< this->getTypeName() << "," << mysize << ">";
    }
    printListState(out, this->begin(), this->end());
  }

  private:
  class v_iterator : public _SerializeArray<T>::iterator {
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

  class v_const_iterator : public _SerializeArray<T>::const_iterator {
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
    typename _SerializeArray<T>::iterator* getIterator() {
      return new v_iterator(this->begin(), this->end());
    }
    typename _SerializeArray<T>::const_iterator* getIterator() const {
      return new v_const_iterator(this->begin(), this->end());
    }
};

/** @} */

}
#endif // _MACE_ARRAY_H
