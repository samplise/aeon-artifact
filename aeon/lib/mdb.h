/* 
 * mdb.h : part of the Mace toolkit for building distributed systems
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
#ifndef _MACE_DB_H
#define _MACE_DB_H

/**
 * \file mdb.h
 * \brief defines generic template wrapper for using berkeley DBs in Mace.
 * \todo JWA, can you document this file?
 */

#ifdef NO_DB_CXX
#error "db_cxx.h could not be found.  Reconfigure Mace with the path to db_cxx"
#endif

#include <db_cxx.h>
#include <errno.h>
#include <string>
#include <sstream>
#include <cassert>
#include <boost/shared_ptr.hpp>
#include "Serializable.h"
#include "ScopedLock.h"
#include "basemap.h"
#include "massert.h"
#include "FileUtil.h"
#include "Log.h"
#include "ScopedLog.h"
#include "Accumulator.h"
#include "ScopedTimer.h"
#include "params.h"

#if DB_VERSION_MAJOR < 4 || (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR < 3)
#define DB_BUFFER_SMALL ENOMEM
#endif
namespace mace {

/**
 * \addtogroup Collections
 * @{
 */


/// Mace-friendly object oriented interface to Berkeley DB.  
/** Assumes that Key and Data type are Serializable.
 */
template<class Key, class Data>
class DB : public virtual Map<Key, Data>, public virtual PrintPrintable {
public:
  typedef Map<Key, Data> baseMap;
  typedef typename baseMap::const_iterator const_iterator;
  typedef typename baseMap::inner_iterator inner_iterator;

public:
  class _inner_iterator : public virtual inner_iterator {
    friend class DB<Key, Data>;

  public:
    typedef typename baseMap::ipair ipair;
    typedef typename baseMap::const_key_ipair const_key_ipair;
    typedef boost::shared_ptr<const_key_ipair> kdpairptr;

  public:
    virtual ~_inner_iterator() {
      if (kbuf) {
	delete[] kbuf;
      }
      if (vbuf) {
	delete[] vbuf;
      }
      ASSERT(c->close() == 0);
      c = 0;
    }

  protected:
    const const_key_ipair* arrow() const {
      ASSERT(c);
      return &(*cur);
    }

    const const_key_ipair& star() const {
      ASSERT(c);
      return *cur;
    }

    void next() const {
      ASSERT(c);
      if (iflags == 0) {
	readCur(DB_NEXT);
      }
    } // next

    void prev() const {
      ASSERT(c);
      if (iflags == 0) {
	readCur(DB_PREV);
      }
      else {
	readCur(iflags);
	iflags = 0;
      }
    } // prev

    bool equals(const inner_iterator& right) const {
      const _inner_iterator& other = dynamic_cast<const _inner_iterator&>(right);
      if (cur && other.cur) {
	return cur->first == other.cur->first;
      }
      return iflags == other.iflags;
    }

    void assign(const inner_iterator& other) {
      copy(dynamic_cast<const _inner_iterator&>(other));
    }

    inner_iterator* clone() const {
      return new _inner_iterator(*this);
    }

    void erase() {
      ABORT("erase by key, not by iterator");
//       ASSERT(c);
//       std::cerr << "deleting key " << ktmp << std::endl;
//       ASSERT(c->del(0) == 0);
//       int r = readDbc(DB_NEXT);
//       if (r == DB_NOTFOUND) {
// 	close();
// 	return;
//       }
//       ASSERT(r == 0);
    }

    void close() const {
      iflags = DB_LAST;
    } // close


  private:
    _inner_iterator(const _inner_iterator& other) :
      c(0), iflags(0), kbuf(0), vbuf(0), kused(0), ksize(0), vsize(0),
      KEY_SIZE(other.KEY_SIZE), DATA_SIZE(other.DATA_SIZE) {

      copy(other);
    }

    _inner_iterator(Dbc* c, size_t ksize, size_t dsize, u_int32_t flags) :
      c(c), iflags(flags), kbuf(0), vbuf(0), kused(0), ksize(0), vsize(0),
      KEY_SIZE(ksize), DATA_SIZE(dsize) {

      if (iflags) {
	return;
      }
      next();
    }

    _inner_iterator(Dbc* c, const Key& sk, size_t ksize, size_t dsize,
		    u_int32_t flags = DB_SET) :
      c(c), iflags(0), kbuf(0), vbuf(0), kused(0), ksize(0), vsize(0),
      KEY_SIZE(ksize), DATA_SIZE(dsize) {

      ASSERT(c);
      setCur(sk, flags);
    }

    void copy(const _inner_iterator& other) {
      ASSERT(other.c);
      other.c->dup(&c, DB_POSITION);
      iflags = other.iflags;
      if (iflags == 0) {
	readCur(DB_CURRENT);
      }
    }

    int readDbc(u_int32_t flags) const {
      ASSERT(c);

      allocKbuf();

      if (vbuf == 0) {
	if (DATA_SIZE) {
	  vsize = DATA_SIZE;
	}
	else {
	  vsize = DEFAULT_DATA_SIZE;
	}
	vbuf = new char[vsize];
      }

      DB::initDbt(k, kbuf, ksize);
      if (flags == DB_SET || flags == DB_SET_RANGE) {
	k.set_size(kused);
      }
      
      DB::initDbt(v, vbuf, vsize);

      while (true) {
	try {
	  int r = c->get(&k, &v, flags);
	  return r;
	}
	catch (DbException& e) {
	  if (e.get_errno() == DB_BUFFER_SMALL || e.get_errno() == ENOMEM) {
	    if (k.get_size() > ksize) {
	      ASSERT(KEY_SIZE == 0);
	      ksize = k.get_size();
	      delete[] kbuf;
	      kbuf = new char[ksize];
	      DB::initDbt(k, kbuf, ksize);
	    }
	    if (v.get_size() > vsize) {
	      ASSERT(DATA_SIZE == 0);
	      vsize = v.get_size();
	      delete[] vbuf;
	      vbuf = new char[vsize];
	      DB::initDbt(v, vbuf, vsize);
	    }
	  }
	  else {
	    throw e;
	  }
	}
      }
    } // readDbc

    void allocKbuf(size_t sz = 0) const {
      if (sz > 0) {
	ASSERT(KEY_SIZE == 0 || sz <= KEY_SIZE);
      }
      if (kbuf == 0) {
	if (KEY_SIZE) {
	  ksize = KEY_SIZE;
	}
	else {
	  ksize = DEFAULT_KEY_SIZE;
	  if (sz > ksize) {
	    ksize = sz;
	  }
	}
	kbuf = new char[ksize];
      }
    } // allocKbuf

    void readCur(u_int32_t flags) const {
      int r = readDbc(flags);
      if (r == DB_NOTFOUND) {
	close();
	return;
      }
//       else if (r == DB_KEYEMPTY) {
// 	next();
// 	return;
//       }
      ASSERT(r == 0);
      fillCur();
    }

    void setCur(const Key& kt, u_int32_t flags = DB_SET) {
      std::string sk = serialize(&kt);
      allocKbuf(sk.size());
      kused = sk.size();
      memcpy(kbuf, sk.data(), sk.size());
      readCur(flags);
    } // setCur

    void fillCur() const {
      kused = k.get_size();
      std::string sk((const char*)k.get_data(), kused);
      deserialize(sk, &ktmp);
      std::string s((const char*)v.get_data(), v.get_size());
      deserialize(s, &dtmp);
      cur = kdpairptr(new const_key_ipair(ktmp, dtmp));
    } // fillCur

  private:
    mutable Dbc* c;
    mutable u_int32_t iflags;
    mutable kdpairptr cur;
    mutable Key ktmp;
    mutable Data dtmp;
    mutable char *kbuf;
    mutable char *vbuf;
    mutable size_t kused;
    mutable size_t ksize;
    mutable size_t vsize;
    const size_t KEY_SIZE;
    const size_t DATA_SIZE;
    mutable Dbt k;
    mutable Dbt v;
  }; // class _inner_iterator

public:
  DB(size_t keySize = 0, size_t dataSize = 0) :
    isOpen(false), privateEnv(true), alwaysSync(true), dbenv(0), db(0),
    KEY_SIZE(keySize), DATA_SIZE(dataSize), databuf(0) {

    dbenv = new DbEnv(0);

    init();
  } // DB

  DB(DbEnv* env, size_t keySize = 0, size_t dataSize = 0) :
    isOpen(false), privateEnv(false), alwaysSync(true), dbenv(env), db(0),
    KEY_SIZE(keySize), DATA_SIZE(dataSize), databuf(0) {

    ASSERT(dbenv);
    init();
  } // DB

//   DB(size_t keySize, size_t dataSize) :
//     isOpen(false), privateEnv(false), alwaysSync(false), dbenv(0), db(0), 
//     KEY_SIZE(keySize), DATA_SIZE(dataSize), databuf(0) {
//     init();
//   } // DB

  virtual ~DB() {
    close();
    if (DATA_SIZE) {
      delete[] databuf;
      databuf = 0;
    }
    if (privateEnv) {
      delete dbenv;
    }
    dbenv = 0;
    delete db;
    db = 0;
  } // ~DB

  virtual void open(const std::string& dir, const std::string& file,
		    DBTYPE dbtype = DB_BTREE, bool create = false) {
    ScopedLock sl(lock);

    if (create && !FileUtil::fileExists(dir)) {
      FileUtil::mkdir(dir, 0755, true);
    }
    
    ASSERT(!isOpen);
    // want DB_INIT_TXN, DB_RECOVER flags?
    // need a way to enable DB_INIT_LOCK if necessary

    // XXX - seems we want DB_REGISTER, but flag is not defined?
    ASSERT(dbenv->open(dir.c_str(),
		       DB_CREATE | DB_INIT_LOG | DB_INIT_MPOOL |
		       DB_INIT_TXN | DB_RECOVER | 
// 		       DB_INIT_LOCK | DB_THREAD |
		       DB_PRIVATE, 0) == 0);

    dbfile = file;
    _open(dbtype);
  } // open

  virtual void open(const std::string& file, DBTYPE dbtype = DB_BTREE) {
    ScopedLock sl(lock);

    ASSERT(!isOpen);
    ASSERT(!privateEnv);

    dbfile = file;
    _open(dbtype);
  } // open

  virtual void open(DBTYPE dbtype = DB_BTREE) {
    ScopedLock sl(lock);
    ASSERT(!isOpen);
    ASSERT(!privateEnv);
    ASSERT(dbenv == 0);

    _open(dbtype);
  } // open

  virtual void setAutoSync(bool s) {
    ASSERT(!isOpen);
    alwaysSync = s;
  } // setAutoSync

  virtual void setCacheSize(uint32_t c) {
    ASSERT(!isOpen);
    ASSERT(dbenv);
    ASSERT(dbenv->set_cachesize(0, c, 1) == 0);
  } // setCacheSize

  virtual void setMemoryLog(uint32_t c = 1048576) {
    ASSERT(!isOpen);
    ASSERT(dbenv);

#if (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR > 6) || DB_VERSION_MAJOR > 4
    ASSERT(dbenv->log_set_config(DB_LOG_IN_MEMORY, 1) == 0);
#else
    ASSERT(dbenv->set_flags(DB_LOG_INMEMORY, 1) == 0);
#endif
    ASSERT(dbenv->set_lg_bsize(c) == 0);
  } // setMemoryLog
  
  virtual void setName(const std::string& n) {
    ASSERT(!isOpen);
    dbenv->set_error_stream(&std::cerr);
    std::string errorPrefix = "DB::" + dbname + "::error: ";
    dbenv->set_errpfx(errorPrefix.c_str());
    dbname = n;
    getlogstr = "DB::" + dbname + "::get";
    putlogstr = "DB::" + dbname + "::put";
    eraselogstr = "DB::" + dbname + "::erase";
    clearlogstr = "DB::" + dbname + "::clear";
  } // setName

  virtual void close() {
    ScopedLock sl(lock);
    if (isOpen) {
      isOpen = false;
      db->close(0);
      if (privateEnv) {
	dbenv->close(0);
      }
    }

//     static const bool USE_DB_TIMERS = params::get("USE_DB_TIMERS", false);
//     if (USE_DB_TIMERS) {
//       Log::log("DB::timers::" + dbname + "::get")
// 	<< mace::listToString(getTimes.begin(), getTimes.end())
// 	<< Log::endl;
//       Log::log("DB::timers::" + dbname + "::put")
// 	<< mace::listToString(putTimes.begin(), putTimes.end())
// 	<< Log::endl;
//       Log::log("DB::timers::" + dbname + "::erase")
// 	<< mace::listToString(eraseTimes.begin(), eraseTimes.end())
// 	<< Log::endl;
	
      //<< getTimes << Log::endl;
//       Log::log("DB::timers::" + dbname + "::put") << putTimes << Log::endl;
//       Log::log("DB::timers::" + dbname + "::erase") << eraseTimes << Log::endl;
//     }
  } // close

  virtual bool get(const Key& kt, Data& d) const {
//     static const bool USE_DB_TIMERS = params::get("USE_DB_TIMERS", false);
    static const bool DB_SCOPED_LOG = params::get("DB_SCOPED_LOG", false);
    ScopedLog slog(getlogstr, Log::DEFAULT_LEVEL, Log::NULL_ID, true, true,
		   DB_SCOPED_LOG, false);
//     ScopedTimer stimer(getTimer, USE_DB_TIMERS, &getTimes);
    static char buf[DEFAULT_DATA_SIZE];
    ASSERT(isOpen);

    std::string sk = serialize(&kt);
    Dbt k((void*)sk.data(), sk.size());

    int r = 0;
    char* dbuf = 0;
    bool del = false;
    Dbt v;
    if (DATA_SIZE) {
      dbuf = databuf;
      initDbt(v, dbuf, DATA_SIZE);

      r = db->get(NULL, &k, &v, 0);
    }
    else {
      dbuf = buf;
      initDbt(v, dbuf, DEFAULT_DATA_SIZE);

      try {
	r = db->get(NULL, &k, &v, 0);
      }
      catch (DbMemoryException& e) {
	if (e.get_errno() == DB_BUFFER_SMALL || e.get_errno() == ENOMEM) {
	  size_t sz = v.get_size();
	  dbuf = new char[sz];
	  del = true;
	  initDbt(v, dbuf, sz);
	  r = db->get(NULL, &k, &v, 0);
	}
	else {
	  throw e;
	}
      }
      catch (DbException& e) {
	throw e;
      }
    }

    if (r == DB_NOTFOUND) {
      if (del) {
	delete[] dbuf;
      }
      return false;
    }
    ASSERT(r == 0);

    std::string s(dbuf, v.get_size());
    deserialize(s, &d);

    if (del) {
      delete[] dbuf;
    }
    
    return true;
  } // get

  virtual bool containsKey(const Key& kt) const {
    Data d;
    return get(kt, d);
  } // containsKey

  virtual void put(const Key& kt, const Data& d) {
    static Accumulator* dbWriteBytes = Accumulator::Instance(Accumulator::DB_WRITE_BYTES);
    static Accumulator* dbWriteCount = Accumulator::Instance(Accumulator::DB_WRITE_COUNT);
//     static const bool USE_DB_TIMERS = params::get("USE_DB_TIMERS", false);
    static const bool DB_SCOPED_LOG = params::get("DB_SCOPED_LOG", false);
    ScopedLog slog(putlogstr, Log::DEFAULT_LEVEL, Log::NULL_ID, true, true,
		   DB_SCOPED_LOG, false);
//     ScopedTimer stimer(putTimer, USE_DB_TIMERS, &putTimes);

    ASSERT(isOpen);

//     Log::log("DB::put") << kt << " " << d << " sync=" << alwaysSync << Log::endl;

    std::string sk = serialize(&kt);
    if (KEY_SIZE) {
      ASSERT(sk.size() <= KEY_SIZE);
    }

    Dbt k((void*)sk.data(), sk.size());

    std::string s = serialize(&d);
    if (DATA_SIZE) {
      ASSERT(s.size() <= DATA_SIZE);
    }

    
    Dbt v((void*)s.data(), s.size());
	
    ASSERT(db->put(NULL, &k, &v, 0) == 0);
    dbWriteCount->accumulate(1);
    dbWriteBytes->accumulate(s.size());
    
    if (alwaysSync) {
      sync();
    }
  } // put

  virtual void sync() {
    ASSERT(isOpen);
    ASSERT(db->sync(0) == 0);
  }

  virtual baseMap& assign(const baseMap& other) {
    ABORT("XXX assign not supported");
  }

  virtual void clear() {
    static const bool DB_SCOPED_LOG = params::get("DB_SCOPED_LOG", false);
    ScopedLog slog(clearlogstr, Log::DEFAULT_LEVEL, Log::NULL_ID, true, true,
		   DB_SCOPED_LOG, false);
    ASSERT(isOpen);
    DBTYPE dbtype;
    ASSERT(db->get_type(&dbtype) == 0);
    db->close(0);
    if (dbenv) {
      ASSERT(dbenv->dbremove(0, dbfile.empty() ? NULL : dbfile.c_str(), NULL, 0) == 0);
    }
    else {
      ABORT("clear not supported yet");
      ASSERT(db->remove(dbfile.empty() ? NULL : dbfile.c_str(), NULL, 0) == 0);
    }
    delete db;
    db = 0;
    _open(dbtype);
  } // clear

  virtual void erase(const Key& kt) {
    static Accumulator* dbEraseCount = Accumulator::Instance(Accumulator::DB_ERASE_COUNT);
//     static const bool USE_DB_TIMERS = params::get("USE_DB_TIMERS", false);
    static const bool DB_SCOPED_LOG = params::get("DB_SCOPED_LOG", false);
    ScopedLog slog(eraselogstr, Log::DEFAULT_LEVEL, Log::NULL_ID, true, true,
		   DB_SCOPED_LOG, false);
//     ScopedTimer stimer(eraseTimer, USE_DB_TIMERS, &eraseTimes);
    ASSERT(isOpen);

    std::string sk = serialize(&kt);

    Dbt k((void*)sk.data(), sk.size());
    int r = db->del(NULL, &k, 0);
    dbEraseCount->accumulate(1);
    ASSERT(r == 0 || r == DB_NOTFOUND);
    if (alwaysSync) {
      sync();
    }
  } // erase

  virtual const_iterator erase(const_iterator i) {
    Key k = i->first;
    i.close();
    erase(k);
    return lower_bound(k);
  } // erase

  virtual size_t size() const {
    size_t s = 0;
    for (const_iterator i = begin(); i != end(); i++) {
      s++;
    }
    return s;
  } // size

  virtual bool empty() const {
    return begin() == end();
  } // empty

  const_iterator begin() const {
    Dbc* c;
    ASSERT(db->cursor(NULL, &c, 0) == 0);
    return const_iterator(
      typename const_iterator::IteratorPtr(
	new _inner_iterator(c, KEY_SIZE, DATA_SIZE, 0)));
  } // begin

  const_iterator end() const {
    Dbc* c;
    ASSERT(db->cursor(NULL, &c, 0) == 0);
    return const_iterator(
      typename const_iterator::IteratorPtr(
	new _inner_iterator(c, KEY_SIZE, DATA_SIZE, DB_LAST)));
  } // end

  const_iterator find(const Key& kt) const {
    Dbc* c;
    ASSERT(db->cursor(NULL, &c, 0) == 0);
    return const_iterator(
      typename const_iterator::IteratorPtr(
	new _inner_iterator(c, kt, KEY_SIZE, DATA_SIZE)));
  } // find

  const_iterator lower_bound(const Key& kt) const {
    Dbc* c;
    ASSERT(db->cursor(NULL, &c, 0) == 0);
    return const_iterator(
      typename const_iterator::IteratorPtr(
	new _inner_iterator(c, kt, KEY_SIZE, DATA_SIZE, DB_SET_RANGE)));
  } // lower_bound

  bool equals(const baseMap& other) const {
    ABORT("equals not implemented");
  } // equals

  void print(std::ostream& printer) const {
    //     ABORT("print not implemented");
    printer << "mdb<does not support printing!>";
  }

  void printNode(PrintNode& printer, const std::string& name) const {
    printMap(printer, name, "DB<" + getTypeName() + ">", begin(), end());
  }

  const std::string& getTypeName() const {
    const char* types[] = { "Key", "Data", 0 };
    static const StrUtilNamespace::StdStringList myTypes = StrUtilNamespace::getTypeFromTemplate(__PRETTY_FUNCTION__, types);
    static string r = myTypes[0]+"->"+myTypes[1];
    return r;
  } // getTypeName


protected: 
  void init() {
    pthread_mutex_init(&lock, 0);
    if (DATA_SIZE) {
      databuf = new char[DATA_SIZE];
    }
  }

//   void init(uint32_t cacheSize, const std::string& errorPrefix) {
//     init();

// //     dbenv->set_flags(DB_LOG_INMEMORY, 1);
// //     dbenv->set_lg_bsize(cacheSize);
    
//     if (cacheSize) {
//       ASSERT(dbenv->set_cachesize(0, cacheSize, 1) == 0);
// //       dbenv->set_cache_max(0, cacheSize);
//     }
//   } // init

  static void initDbt(Dbt& dbt, char* buf, size_t sz) {
    dbt.set_data(buf);
    dbt.set_size(sz);
    dbt.set_ulen(sz);
    dbt.set_flags(DB_DBT_USERMEM);
  }

  virtual void _open(DBTYPE dbtype) {
    if (db == 0) {
      db = new Db(dbenv, 0);
      db->set_error_stream(&std::cerr);
    }

    // want DB_AUTO_COMMIT flag?
    ASSERT(db->open(NULL, dbfile.empty() ? NULL : dbfile.c_str(), NULL, dbtype,
// 		    DB_THREAD |
		    DB_CREATE, 0) == 0);
//     ASSERT(db->open(NULL, file.c_str(), NULL, dbtype,
// 		    DB_CREATE | DB_THREAD | DB_AUTO_COMMIT, 0) == 0);

    isOpen = true;
  } // _open


private:
  bool isOpen;
  bool privateEnv;
  bool alwaysSync;
  std::string dbfile;
  DbEnv *dbenv;
  Db *db;

  const size_t KEY_SIZE;
  const size_t DATA_SIZE;
  char* databuf;

  pthread_mutex_t lock;

  static const size_t DEFAULT_KEY_SIZE = 1024;
  static const size_t DEFAULT_DATA_SIZE = 4096;

  std::string dbname;
  mutable uint64_t getTimer;
  mutable ScopedTimer::TimeList getTimes;
  uint64_t putTimer;
  ScopedTimer::TimeList putTimes;
  uint64_t eraseTimer;
  ScopedTimer::TimeList eraseTimes;

  std::string getlogstr;
  std::string putlogstr;
  std::string eraselogstr;
  std::string clearlogstr;
}; // class DB

/** @} */

} // namespace mace

#endif // _MACE_DB_H
