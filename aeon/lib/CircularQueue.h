/* 
 * CircularQueue.h : part of the Mace toolkit for building distributed systems
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
#ifndef CIRCULAR_QUEUE_H
#define CIRCULAR_QUEUE_H

/**
 * \file CircularQueue.h
 * \brief defines the CircularQueue class
 */

/**
 * \addtogroup Collections
 * @{
 */

/**
 * \brief an implementation of a circuluar queue
 *
 * Does not provide iteration.  Only supports pushing at the end and pulling
 * from the front.  However, it does support a lock-free mechanism for queueing
 * data between two threads, one pushing and the other popping.  The circular
 * queue is implemented as a fixed size array of pointers.  First and last
 * pointers keep the position in the queue, and elements can be added as long
 * as the queue is not full.
 *
 * \todo move to mace:: namespace
 */
template<class T>
class CircularQueue {
public:
  /*static const size_t DEFAULT_MAX_SIZE = 4096; ///< the default maximum size of the queue*/
  static const size_t DEFAULT_MAX_SIZE = 65536; ///< the default maximum size of the queue

public:
  CircularQueue(size_t len = DEFAULT_MAX_SIZE) :
    MAX_SIZE(len), data(0), first(0), last(0) {
    data = new T*[MAX_SIZE];
  } // CircularQueue

  virtual ~CircularQueue() {
    while (!empty()) {
      pop();
    }
    delete [] data;
  } // ~CircularQueue

  /// Returns the number of elements in the queue
  virtual uint64_t size() const {
    if (last >= first) {
      return last - first;
    }
    else {
      return MAX_SIZE - first + last;
    }
  } // size

  /// Returns whether the queue is full
  virtual bool isFull() const {
    return (((last + 1) % MAX_SIZE) == first);
  } // isFull

  /// Returns whether the queue is empty
  virtual bool empty() const {
    return (first == last);
  } // empty

  /// Returns a reference to the first item without removing it
  virtual T& front() const {
    assert(!empty());
    return *(data[first]);
  } // front

  /// Removes the first item from the queue
  virtual void pop() {
    assert(!empty());
    T* v = data[first];
    first = (first + 1) % MAX_SIZE;
    delete v;
  } // pop

  /// Places an item at the end of the queue
  virtual void push(const T& v) {
    assert(!isFull());
    data[last] = new T(v);
    last = (last + 1) % MAX_SIZE;
  } // push

  /// Removes all elements from the queue
  virtual void clear() {
    while (!empty()) {
      pop();
    }
  } // clear

public:
  const size_t MAX_SIZE; ///< the max size of the queue (and the number of slots reserved for the queue

protected:
  T** data;
  size_t first;
  size_t last;

}; // CircularQueue

/** @} */

#endif // CIRCULAR_QUEUE_H
