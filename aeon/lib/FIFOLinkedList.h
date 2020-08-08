/* 
 * FIFOLinkedList.h : part of the Mace toolkit for building distributed systems
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
#ifndef _FIFO_LINKED_LIST_H
#define _FIFO_LINKED_LIST_H

#include <boost/shared_ptr.hpp>
#include <stdint.h>

/**
 * \file FIFOLinkedList.h
 * \brief defines the FIFOLinkedList class
 */

/**
 * \addtogroup Collections
 * @{
 */

/**
 * \brief Implements a linked list which only supports push/pop for FIFO use.
 *
 * Believed to be thread-safe when used by two threads, one pushing and the other popping.
 * Does not support iteration.
 *
 * \todo JWA, check my docs
 * \todo move to the mace:: namespace
 */
template <class T>
class FIFOLinkedList {
private:
  class Element;
  typedef boost::shared_ptr<Element> ElementPtr;
  
  class Element {
  public:
    Element() { }
    Element(const T& v) : v(v) { }

    T v;
    ElementPtr next;
  };


public:
  FIFOLinkedList(const T& v) : first(0), last(0) {
    head = ElementPtr(new Element(v));
    tail = head;
  }

  virtual ~FIFOLinkedList() { }

  /// returns whether the head element is non-null
  virtual bool canPop() const {
    return (head->next.get() != 0);
  } // canPop

  /// returns the number of elements in the list
  virtual uint64_t size() const {
    return last - first;
  } // size

  /// returns whether the list is empty (is this !canPop?)
  virtual bool empty() const {
    return (first == last);
  }

  /// returns a reference to the first element
  virtual T& front() const {
    return head->v;
  } // front

  /// returns a reference to the last element
  virtual T& back() const {
    return tail->v;
  } // back

  /// adds a new element to the end
  virtual void push(const T& v) {
    last++;
    ElementPtr n = ElementPtr(new Element(v));
    tail->next = n;
    tail = n;
  } // push

  /// removes the first element
  virtual void pop() {
    first++;
    assert(canPop());
    head = head->next;
  } // pop

  /// removes all elements
  virtual void clear() {
    while (!empty()) {
      pop();
    }
  }
  
private:
  uint64_t first;
  uint64_t last;
  ElementPtr head;
  ElementPtr tail;
}; // FIFOLinkedList

/** @} */

#endif // _FIFO_LINKED_LIST_H
