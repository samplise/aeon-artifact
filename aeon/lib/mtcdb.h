/* 
 * mtcdb.h : part of the Mace toolkit for building distributed systems
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
#ifndef _MACE_TC_DB_H
#define _MACE_TC_DB_H

/**
 * \file mtcdb.h
 * \brief defines generic template wrapper for using Tokyo Cabinet DBs in Mace.
 * \todo JWA, can you document this file?
 */


#include <tcutil.h>
#include <tchdb.h>
#include <tcbdb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
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

namespace mace {

/**
 * \addtogroup Collections
 * @{
 */


/// Mace-friendly object oriented interface to Berkeley DB.  
/** Assumes that Key and Data type are Serializable.
 */
template<class Key, class Data>
class TCDB : public virtual Map<Key, Data>, public virtual PrintPrintable {
public:
  typedef Map<Key, Data> baseMap;
  typedef typename baseMap::const_iterator const_iterator;
  typedef typename baseMap::inner_iterator inner_iterator;

public:
  class _inner_iterator : public virtual inner_iterator {
    friend class TCDB<Key, Data>;

  public:
    typedef typename baseMap::ipair ipair;
    typedef typename baseMap::const_key_ipair const_key_ipair;
    typedef boost::shared_ptr<const_key_ipair> kdpairptr;

  public:
    virtual ~_inner_iterator() {
      ASSERT(c);
      tcbdbcurdel(c);
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
      if (!atEnd) {
	if (tcbdbcurnext(c)) {
	  fillCur();
	}
	else {
	  atEnd = true;
	}
      }
    } // next

    void prev() const {
      ASSERT(c);
      if (atEnd) {
	if (tcbdbcurlast(c)) {
	  fillCur();
	  atEnd = false;
	}
      }
      else {
	if (tcbdbcurprev(c)) {
	  fillCur();
	}
      }
    } // prev

    bool equals(const inner_iterator& right) const {
      const _inner_iterator& other = dynamic_cast<const _inner_iterator&>(right);
      if (cur && other.cur) {
	return cur->first == other.cur->first;
      }
      return atEnd == other.atEnd;
    }

//     void assign(const inner_iterator& other) {
//       copy(dynamic_cast<const _inner_iterator&>(other));
//     }

//     inner_iterator* clone() const {
//       return new _inner_iterator(*this);
//     }

    void erase() {
      if (!atEnd && tcbdbcurout(c)) {
	int sz = 0;
	const char* buf = tcbdbcurkey3(c, &sz);
	if (buf) {
	  string ck(buf, sz);
	  fillCur(buf);
	}
	else {
	  atEnd = true;
	}
      }
    } // erase

    void close() const { }


  private:
//     _inner_iterator(const _inner_iterator& other) : c(0), atEnd(true) {
//       copy(other);
//     }

    _inner_iterator(BDBCUR* c, bool init) : c(c), atEnd(true) {
      if (init) {
	if (tcbdbcurfirst(c)) {
	  fillCur();
	}
      }
    }

    _inner_iterator(BDBCUR* c, const Key& k, bool lower) : c(c), atEnd(true) {
      std::string sk = serialize(&k);
      if (tcbdbcurjump(c, sk.data(), sk.size())) {
	if (!lower) {
	  int sz = 0;
	  const char* buf = tcbdbcurkey3(c, &sz);
	  ASSERT(buf);
	  string ck(buf, sz);
	  if (sk != ck) {
	    return;
	  }
	  fillCur(ck);
	}
	else {
	  fillCur();
	}
      }
    }

//     void copy(const _inner_iterator& other) {
//       ASSERT(other.c);
//       c = other.c;
//       atEnd = other.atEnd;
//       cur = other.cur;
//     }

    void fillCur(const string& sk) const {
      atEnd = false;
      deserialize(sk, &ktmp);
      int sz = 0;
      const char* v = tcbdbcurval3(c, &sz);
      ASSERT(v);
      std::string s(v, sz);
      deserialize(s, &dtmp);
      cur = kdpairptr(new const_key_ipair(ktmp, dtmp));
    } // fillCur
    
    void fillCur() const {
      int sz = 0;
      const char* buf = tcbdbcurkey3(c, &sz);
      ASSERT(buf);
      string ck(buf, sz);
      fillCur(ck);
    } // fillCur

  private:
    mutable BDBCUR* c;
    mutable bool atEnd;
    mutable kdpairptr cur;
    mutable Key ktmp;
    mutable Data dtmp;
  }; // class _inner_iterator

public:
  TCDB(const std::string& p, bool autosync = false,
       int32_t lcnum = 0, int32_t ncnum = 0) :
    path(p), alwaysSync(autosync) {

    std::string dir = FileUtil::dirname(path);
    if (!dir.empty() && !FileUtil::fileExists(dir)) {
      FileUtil::mkdir(dir, 0755, true);
    }

    db = tcbdbnew();
    ASSERT(db);

    if (lcnum || ncnum) {
      tcbdbsetcache(db, lcnum, ncnum);
    }
    
    if (!tcbdbopen(db, path.c_str(), BDBOWRITER | BDBOCREAT | BDBONOLCK)) {
      mtcdberror("tcdbopen " + path);
    }
  } // DB

  virtual ~TCDB() {
    close();
  } // ~DB

  virtual void setName(const std::string& n) {
    dbname = n;
    getlogstr = "TCDB::" + dbname + "::get";
    containslogstr = "TCDB::" + dbname + "::contains";
    putlogstr = "TCDB::" + dbname + "::put";
    eraselogstr = "TCDB::" + dbname + "::erase";
    clearlogstr = "TCDB::" + dbname + "::clear";
  } // setName

  virtual void close() {
    if (db) {
      if (!tcbdbclose(db)) {
	mtcdberror("tcdbclose " + path);
      }
      tcbdbdel(db);
      db = 0;
    }
  } // close

  virtual bool get(const Key& kt, Data& d) const {
    static const bool DB_SCOPED_LOG = params::get("DB_SCOPED_LOG", false);
    ScopedLog slog(getlogstr, Log::DEFAULT_LEVEL, Log::NULL_ID, true, true,
		   DB_SCOPED_LOG, false);

    ASSERT(db);
    std::string sk = serialize(&kt);
    int sz = 0;
    const void* buf = tcbdbget3(db, sk.data(), sk.size(), &sz);
    if (!buf) {
      return false;
    }

    std::string s((const char*)buf, sz);
    deserialize(s, &d);
    
    return true;
  } // get

  virtual bool containsKey(const Key& kt) const {
    static const bool DB_SCOPED_LOG = params::get("DB_SCOPED_LOG", false);
    ScopedLog slog(containslogstr, Log::DEFAULT_LEVEL, Log::NULL_ID, true, true,
		   DB_SCOPED_LOG, false);
    ASSERT(db);
    std::string sk = serialize(&kt);
    return tcbdbvnum(db, sk.data(), sk.size());
  } // containsKey

  virtual void put(const Key& kt, const Data& d) {
    static Accumulator* dbWriteBytes = Accumulator::Instance(Accumulator::DB_WRITE_BYTES);
    static Accumulator* dbWriteCount = Accumulator::Instance(Accumulator::DB_WRITE_COUNT);
    static const bool DB_SCOPED_LOG = params::get("DB_SCOPED_LOG", false);
    ScopedLog slog(putlogstr, Log::DEFAULT_LEVEL, Log::NULL_ID, true, true,
		   DB_SCOPED_LOG, false);

//     Log::log("DB::put") << kt << " " << d << " sync=" << alwaysSync << Log::endl;

    ASSERT(db);

    std::string sk = serialize(&kt);
    std::string s = serialize(&d);
    if (!tcbdbput(db, sk.data(), sk.size(), s.data(), s.size())) {
      mtcdberror("tcbdbput");
    }
    
    dbWriteCount->accumulate(1);
    dbWriteBytes->accumulate(s.size());
    
    if (alwaysSync) {
      sync();
    }
  } // put

  virtual void sync() {
    ASSERT(db);
    if (!tcbdbsync(db)) {
      mtcdberror("tcbdbsync");
    }
  } // sync

  virtual baseMap& assign(const baseMap& other) {
    ABORT("XXX assign not supported");
  }

  virtual void clear() {
    static const bool DB_SCOPED_LOG = params::get("DB_SCOPED_LOG", false);
    ScopedLog slog(clearlogstr, Log::DEFAULT_LEVEL, Log::NULL_ID, true, true,
		   DB_SCOPED_LOG, false);
    ASSERT(db);
    if (!tcbdbvanish(db)) {
      mtcdberror("tcbdbvanish");
    }
  } // clear

  virtual void erase(const Key& kt) {
//     static Accumulator* dbEraseCount = Accumulator::Instance(Accumulator::DB_ERASE_COUNT);
//     static const bool USE_DB_TIMERS = params::get("USE_DB_TIMERS", false);
    static const bool DB_SCOPED_LOG = params::get("DB_SCOPED_LOG", false);
    ScopedLog slog(eraselogstr, Log::DEFAULT_LEVEL, Log::NULL_ID, true, true,
		   DB_SCOPED_LOG, false);
//     ScopedTimer stimer(eraseTimer, USE_DB_TIMERS, &eraseTimes);
    ASSERT(db);

    std::string sk = serialize(&kt);
    tcbdbout(db, sk.data(), sk.size());
    if (alwaysSync) {
      sync();
    }
  } // erase

  virtual const_iterator erase(const_iterator i) {
    ASSERT(db);
    i.erase();
    if (alwaysSync) {
      sync();
    }
    return i;
  } // erase

  virtual size_t size() const {
    ASSERT(db);
    return tcbdbrnum(db);
  } // size

  virtual bool empty() const {
    ASSERT(db);
    return begin() == end();
  } // empty

  const_iterator begin() const {
    ASSERT(db);
    BDBCUR* c = tcbdbcurnew(db);
    if (!c) {
      mtcdberror("tcbdbcurnew");
    }
    return const_iterator(
      typename const_iterator::IteratorPtr(new _inner_iterator(c, true)));
  } // begin

  const_iterator end() const {
    ASSERT(db);
    BDBCUR* c = tcbdbcurnew(db);
    if (!c) {
      mtcdberror("tcbdbcurnew");
    }
    return const_iterator(
      typename const_iterator::IteratorPtr(new _inner_iterator(c, false)));
  } // end

  const_iterator find(const Key& kt) const {
    ASSERT(db);
    BDBCUR* c = tcbdbcurnew(db);
    if (!c) {
      mtcdberror("tcbdbcurnew");
    }
    return const_iterator(
      typename const_iterator::IteratorPtr(new _inner_iterator(c, kt, false)));
  } // find

  const_iterator lower_bound(const Key& kt) const {
    ASSERT(db);
    BDBCUR* c = tcbdbcurnew(db);
    if (!c) {
      mtcdberror("tcbdbcurnew");
    }
    return const_iterator(
      typename const_iterator::IteratorPtr(new _inner_iterator(c, kt, true)));
  } // lower_bound

  bool equals(const baseMap& other) const {
    ABORT("equals not implemented");
  } // equals

  void print(std::ostream& printer) const {
    ASSERT(db);
    //     ABORT("print not implemented");
    printer << "tcdb<does not support printing!>";
  }

  void printNode(PrintNode& printer, const std::string& name) const {
    ASSERT(db);
    printMap(printer, name, "TCDB<" + getTypeName() + ">", begin(), end());
  }

  const std::string& getTypeName() const {
    const char* types[] = { "Key", "Data", 0 };
    static const StrUtilNamespace::StdStringList myTypes = StrUtilNamespace::getTypeFromTemplate(__PRETTY_FUNCTION__, types);
    static string r = myTypes[0]+"->"+myTypes[1];
    return r;
  } // getTypeName


protected: 
  void mtcdberror(const char* msg) const {
    mtcdberror(std::string(msg));
  }

  void mtcdberror(const std::string& msg) const {
    int code = tcbdbecode(db);
    Log::err() << "TCDB::" << dbname << ": " << msg << " " << tcbdberrmsg(code)
	       << Log::endl;
    ABORT("TCDB error");
  }
  

private:
  std::string path;
  bool alwaysSync;
  TCBDB* db;

  std::string dbname;

  std::string getlogstr;
  std::string containslogstr;
  std::string putlogstr;
  std::string eraselogstr;
  std::string clearlogstr;
}; // class DB

/** @} */

} // namespace mace

#endif // _MACE_TC_DB_H
