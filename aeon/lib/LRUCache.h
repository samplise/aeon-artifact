/* 
 * LRUCache.h : part of the Mace toolkit for building distributed systems
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
#include <pthread.h>

#include "TimeUtil.h"
#include "ScopedLock.h"
#include "mstring.h"
#include "m_map.h"

#ifndef LRUCACHE_H
#define LRUCACHE_H

/**
 * \file LRUCache.h
 * \brief defines the mace::LRUCache class
 */


namespace mace {

/**
 * \brief Provides a cache with a least recently used replacement policy and a map like interface.
 * 
 * The add methods will evict the least recently used non-dirty key.
 * If all the indices are dirty, then the least recently used dirty
 * key will be evicted.
 * 
 * Use timeouts with caution---if a dirty entry expires, it will be
 * deleted the next time containsKey is called for its entry.  Because
 * dirty entries may be deleted before they have a chance to be
 * cleared, it is advised to not use a timeout when storing dirty data
 * in the cache.
 * 
 * \warning you MUST call containsKey and check that the result is true
 *       before calling get, operator[], or obtain the key by getLastDirtyKey
 */
template <typename K, typename D>
class LRUCache {

  class CacheEntry {
  public:
    CacheEntry(const K& index, const D& d, bool dirty, time_t t) :
      index(index), data(d), dirty(dirty), init(t), prev(0), next(0) {
    } // CacheEntry

    virtual ~CacheEntry() {
    }

  public:
    K index;
    D data;
    bool dirty;
    time_t init;
    CacheEntry* prev;
    CacheEntry* next;
  }; // CacheEntry

//   typedef hash_map<K, CacheEntry*, HashFcn, EqualKey> LRUCacheIndexMap;
  typedef map<K, CacheEntry*, SoftState> LRUCacheIndexMap;

public:

  /**
   * \brief create a new cache with the specified capacity.  
   *
   * If a timeout is given, then entries will be expired after the specified
   * timeout.
   */
  LRUCache(unsigned capacity = 32, time_t timeout = 0, bool useLocking = true) :
    capacity(capacity), timeout(timeout), size(0), dirtyCount(0), cache(0),
    lockptr(0) {

    pthread_mutexattr_t ma;
    ASSERT(pthread_mutexattr_init(&ma) == 0);
    ASSERT(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_RECURSIVE) == 0);
    pthread_mutex_init(&lrulock, &ma);
    ASSERT(pthread_mutexattr_destroy(&ma) == 0);
    if (useLocking) {
      lockptr = &lrulock;
    }
  } // LRUCache

  virtual ~LRUCache() {
    ASSERT(pthread_mutex_lock(&lrulock) == 0);
    while (cache != 0) {
      CacheEntry* tmp = cache;
      cache = cache->next;
      indexMap.erase(tmp->index);
      delete tmp;
      tmp = 0;
    }
    ASSERT(pthread_mutex_unlock(&lrulock) == 0);
    pthread_mutex_destroy(&lrulock);
  } // ~LRUCache

  /**
   * \brief add a new entry to the cache, mark as dirty
   *
   * The add methods will evict the least recently used non-dirty key.
   * If all the indices are dirty, then the least recently used dirty
   * key will be evicted.
   */
  void addDirty(const K& index, const D& buf) {
    add(index, buf, true);
  } // addDirty

  /**
   * \brief add a new entry to the cache, set dirty as specified
   *
   * The add methods will evict the least recently used non-dirty key.
   * If all the indices are dirty, then the least recently used dirty
   * key will be evicted.
   */
  void add(const K& index, const D& buf, bool dirty = false) {
    ScopedLock sl(lockptr);
    typename LRUCacheIndexMap::iterator i = indexMap.find(index);
    if (i != indexMap.end()) {
      // If you add a block that is already in the cache, you overwrite
      // it unconditionally.  If the old block was dirty and the new one
      // is not, then you lose the dirty data
      CacheEntry* e = i->second;
      e->data = buf;
      if (e->dirty && !dirty) {
	dirtyCount--;
      }
      else if (!e->dirty && dirty) {
	dirtyCount++;
      }
      e->dirty = dirty;
      moveToHead(e);
      return;
    }

    // XXX - jwa - why was this here?  It should be an error to attempt
    // to insert into a full dirty cache.  If this code was left in
    // place, the cache would silently lose data---better to assert.
//     if (isFullDirty()) {
//       clearDirty(getLastDirtyKey());
//     }
    
    ASSERT(!isFullDirty());

    if (size == capacity) {
      // evict the lru non-dirty block
      CacheEntry* last = cache->prev;
      while (last->dirty) {
	last = last->prev;
      }

      remove(last->index);
    }

    ASSERT(size < capacity);

    size++;
    if (dirty) {
      dirtyCount++;
    }

    time_t now = TimeUtil::time();
    CacheEntry* e = new CacheEntry(index, buf, dirty, now);
    if (cache == 0) {
      e->prev = e;
    }
    else {
      e->next = cache;
      e->prev = cache->prev;
      cache->prev = e;
    }
    cache = e;
    indexMap[index] = e;
  } // add

  inline void remove(const K& index) {
    erase(index);
  } // remove

  /// removes the entry from the cache
  void erase(const K& index) {
    ScopedLock sl(lockptr);
    typename LRUCacheIndexMap::iterator i = indexMap.find(index);
    if (i != indexMap.end()) {
      CacheEntry* e = i->second;

      clearDirty(e);

      if (e->next) {
	e->next->prev = e->prev;
      }
      else {
	cache->prev = e->prev;
      }

      if (cache == e) {
	cache = e->next;
      }
      else {
        //Before, this would complete the loop if the first element were removed.
        e->prev->next = e->next;
      }

      delete e;
      e = 0;
      indexMap.erase(i);
      size--;

      ASSERT(indexMap.size() == size);
      if (size == 0) {
	ASSERT(cache == 0);
      }
    }
  } // erase

  /**
   * \brief lookup the value for the given key
   * \warning you MUST call containsKey and check that the result is true
   *  before calling get, operator[], or obtain the key by getLastDirtyKey
   */
  bool get(const K& index, D& data) {
    ScopedLock sl(lockptr);
    typename LRUCacheIndexMap::iterator i = indexMap.find(index);
    if (i == indexMap.end()) {
      return false;
    }
    
    CacheEntry* e = i->second;
    moveToHead(e);
    data = e->data;
    return true;
  }
    
  /**
   * \brief lookup the value for the given key
   * \warning you MUST call containsKey and check that the result is true
   *  before calling get, operator[], or obtain the key by getLastDirtyKey
   */
  D& get(const K& index) {
    ScopedLock sl(lockptr);
    typename LRUCacheIndexMap::iterator i = indexMap.find(index);
    ASSERT(i != indexMap.end());

    CacheEntry* e = i->second;
    moveToHead(e);
    return e->data;
  } // get

  /**
   * \brief lookup the value for the given key
   * \warning you MUST call containsKey and check that the result is true
   *  before calling get, operator[], or obtain the key by getLastDirtyKey
   */
  D& operator[](const K& index) {
    return get(index);
  } // operator[]

  bool containsKey(const K& index) {
    ScopedLock sl(lockptr);
    typename LRUCacheIndexMap::iterator i = indexMap.find(index);
    bool c = (i != indexMap.end());
    if (c && (timeout != 0)) {
      time_t now = time(0);
      CacheEntry* e = i->second;
      if ((now - e->init) > timeout) {
	remove(index);
	return false;
      }
    }

    return c;
  } // containsKey

  /// true if the cache is full, and all entries are dirty
  bool isFullDirty() const {
    return (dirtyCount == capacity);
  } // isFullDirty

  /// true if any entries are dirty
  bool hasDirty() const {
    return (dirtyCount > 0);
  } // hasDirty

  /// return the least recently accessed dirty key
  const K& getLastDirtyKey() const {
    ScopedLock sl(lockptr);
    ASSERT(hasDirty());

    CacheEntry* last = cache->prev;
    while (!last->dirty) {
      last = last->prev;
    }

    return last->index;
  } // getLastDirtyKey

  /// return the least recently accessed dirty value
  D& getLastDirty() const {
    ScopedLock sl(lockptr);
    CacheEntry* last = getLastDirtyEntry();
    return last->data;
  } // getLastDirty

  /// return the least recently accessed dirty key and value
  bool getLastDirty(K& k, D& d) const {
    ScopedLock sl(lockptr);
    if (!hasDirty()) {
      return false;
    }

    CacheEntry* last = getLastDirtyEntry();
    k = last->index;
    d = last->data;
    return true;
  } // getLastDirty

  /// clear the dirty bit for the key returned by getLastDirtyEntry()
  void clearLastDirty() {
    ScopedLock sl(lockptr);
    clearDirty(getLastDirtyEntry());
  } // clearLastDirty

  /// clear the dirty bit for the given key
  void clearDirty(const K& index) {
    ScopedLock sl(lockptr);
    typename LRUCacheIndexMap::iterator i = indexMap.find(index);
    ASSERT(i != indexMap.end());
    CacheEntry* e = i->second;
    clearDirty(e);
  } // clearDirty

  /** \brief get the value of the given key
   *
   *  does not adjust access status: the key remains in its current position
   *  with respect to how recently it was used
   */
  D& getDirty(const K& index) {
    // this does not move the index to the head of the cache
    ScopedLock sl(lockptr);
    typename LRUCacheIndexMap::iterator i = indexMap.find(index);
    ASSERT(i != indexMap.end());
    CacheEntry* e = i->second;
    return e->data;
  } // getDirty

private:

  CacheEntry* getLastDirtyEntry() const {
    ASSERT(hasDirty());
    CacheEntry* last = cache->prev;
    while (!last->dirty) {
      last = last->prev;
    }
    return last;
  }

  void clearDirty(CacheEntry* e) {
    if (e->dirty) {
      dirtyCount--;
      e->dirty = false;
    }
  }
    
  void moveToHead(CacheEntry* e) {
    if (e->index == cache->index) {
      ASSERT(cache == e);
      return;
    }
    e->prev->next = e->next;
    if (e->next) {
      e->next->prev = e->prev;
      e->prev = cache->prev;
    }
    e->next = cache;
    cache->prev = e;
    cache = e;
  } // moveToHead

private:
  unsigned capacity;
  time_t timeout;
  unsigned size;
  unsigned dirtyCount;
  CacheEntry* cache;
  LRUCacheIndexMap indexMap;
  mutable pthread_mutex_t lrulock;
  mutable pthread_mutex_t* lockptr;
};


} // namespace mace

#endif // LRUCACHE_H
