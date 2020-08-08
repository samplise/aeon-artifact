/* 
 * MapConfig.h : part of the Mace toolkit for building distributed systems
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
#ifndef _MAP_CONFIG_H
#define _MAP_CONFIG_H

#include "basemap.h"
#include "TemplateMap.h"
#include "m_map.h"
#include "mhash_map.h"
#include "mdb.h"
#ifdef HAVE_TOKYO_CABINET
#include "mtcdb.h"
#endif
#include "Log.h"
#include "params.h"

/**
 * \file MapConfig.h
 * \brief auto configuration (factory construction) of generic maps based on params
 * \todo JWA, please doc
 */

class MapConfig {
public:
  // types
  static const std::string TREE;
  static const std::string HASH;
  static const std::string DB;
  static const std::string DBHASH;
  static const std::string DBMEM;
  static const std::string TCDB;

  // default db dir
  static const std::string DB_DIR;

  // file for db; if path, implicit env
  static const std::string FILE;

  // bool
  static const std::string AUTO_SYNC;
  static const std::string SYNC; // same as AUTO_SYNC

  // sizes
  static const std::string CACHE;
  static const std::string LOG;
  static const std::string KEY_SIZE;
  static const std::string DATA_SIZE;
  static const std::string NCACHE;
  static const std::string LCACHE;
  
private:
  struct DbEnvInfo {
    DbEnv* env;
    size_t count;

    DbEnvInfo() : env(0), count(0) { }
  };

  typedef mace::map<std::string, DbEnvInfo, mace::SoftState> EnvInfoMap;
  static EnvInfoMap envs;
  static std::string dbsubdir;

public:
  template<typename Key, typename Data>
  static mace::Map<Key, Data>& open(const std::string& name,
				    size_t keySize = 0, size_t dataSize = 0) {
    std::string type = params::get<std::string>(name, "");
    if (type.empty()) {
      if (params::get("MAP_CONFIG_DEFAULT", 0)) {
	type = TREE;
      }
      else {
	Log::err() << "no map config for " << name << Log::endl;
	ABORT("attempt to instantiate unconfigured map");
      }
    }

    if (type == TREE) {
      mace::TemplateMap<mace::map<Key, Data> >* r =
	new mace::TemplateMap<mace::map<Key, Data> >();
      r->setName(name);
      return *r;
    }
//     else if (type == HASH) {
//       return *(new mace::TemplateMap<mace::hash_map<Key, Data> >());
//     }
    else if (type == DBMEM) {
      mace::DB<Key, Data>* db = new mace::DB<Key, Data>(keySize, dataSize);
      db->open();
      db->setName(name);
      return *db;
    }
    else if (type == DB || type == DBHASH) {
      DBTYPE dbtype = DB_BTREE;
      if (type == DBHASH) {
	dbtype = DB_HASH;
      }
      std::string file;
      std::string dir;
      uint32_t cacheSize;
      uint32_t logCache = 0;
      parseDbPaths(name, file, dir, cacheSize, logCache, keySize, dataSize);

      bool autosync = params::get(name + "." + AUTO_SYNC, true);
      if (params::containsKey(name + "." + SYNC)) {
	autosync = params::get<bool>(name + "." + SYNC);
      }

      DbEnvInfo& dbi = envs[dir];
      mace::DB<Key, Data>* db = 0;

      if (dbi.env) {
	Log::log("MapConfig::open") << "opening DB " << file << " in dir "
				    << dir << " count=" << dbi.count
				    << Log::endl;
	db = new mace::DB<Key, Data>(dbi.env, keySize, dataSize);
	db->setName(name);
	db->setAutoSync(autosync);
	db->open(file, dbtype);
      }
      else {
	Log::log("MapConfig::open") << "opening DB " << file
				    << " in dir " << dir
				    << " with cache=" << cacheSize
				    << " with log=" << logCache
				    << " autosync=" << autosync
				    << " with new env" << Log::endl;
	dbi.env = new DbEnv(0);
	db = new mace::DB<Key, Data>(dbi.env, keySize, dataSize);
	db->setName(name);
	db->setAutoSync(autosync);
	db->setCacheSize(cacheSize);
	if (logCache) {
	  db->setMemoryLog(logCache);
	}
	db->open(dir, file, dbtype, true);
      }
      dbi.count++;
      
      return *db;
    }
#ifdef HAS_TOKYO_CABINET
    else if (type == TCDB) {
      std::string path;
      bool sync = true;
      int32_t lcnum = 0;
      int32_t ncnum = 0;
      parseDbPath(name, path, sync, lcnum, ncnum);

      Log::log("MapConfig::open") << "opening TCDB " << path
				  << " with cache=" << lcnum << "/" << ncnum
				  << " sync=" << sync
				  << Log::endl;

      mace::TCDB<Key, Data>* db = new mace::TCDB<Key, Data>(path, sync,
							    lcnum, ncnum);
      db->setName(name);
      return *db;
    }
#endif
    else {
      Log::err() << "unsupported map type: " << type << Log::endl;
      ABORT("unsupported map type");
    }
  } // open

  template<typename T>
  static void close(const std::string& name, T& m) {
    Log::log("MapConfig::close") << "closing " << name << Log::endl;
    m.close();
    delete &m;
    if (params::get<std::string>(name, TREE) == DB) {
      std::string dir = parseDbDir(name);
      DbEnvInfo& dbi = envs[dir];
      if (dbi.count == 0) {
	Log::err() << "cannot find db info for " << name << " on close"
		   << Log::endl;
	ABORT("db close");
      }
      dbi.count--;
      if (dbi.count == 0) {
	dbi.env->close(0);
	delete dbi.env;
	envs.erase(dir);
	Log::log("MapConfig::close") << "closed and erased env for " << dir
				     << Log::endl;
      }
      if (envs.empty()) {
	Log::log("MapConfig::close") << "all DBs closed" << Log::endl;
      }
    }
  } // close

//   static void parseDbTemplateSizes(const std::string& name, size_t& ks,
// 				   size_t& ds) {
//     ks = params::get(name + "." + KEY_SIZE, mace::DB_DEFAULT_MAX_KEY_SIZE);
//     ds = params::get(name + "." + DATA_SIZE, mace::DB_DEFAULT_MAX_DATA_SIZE);
//   } // parseDbTemplateSizes

  static void setDbSubdir(const std::string& s) {
    dbsubdir = s;
  }

  static std::string parseDbDir(const std::string& name) {
    std::string file;
    std::string dir;
    uint32_t cache;
    uint32_t logcache;
    size_t keySize;
    size_t dataSize;
    parseDbPaths(name, file, dir, cache, logcache, keySize, dataSize);
    return dir;
  }

  static void parseDbPaths(const std::string& name, std::string& file,
			   std::string& dir, uint32_t& cacheSize,
			   uint32_t& logMemory,
			   size_t& keySize, size_t& dataSize) {
    file = params::get<std::string>(name + "." + FILE, "");
    if (file.empty()) {
      Log::err() << "no file defined for db " << name << Log::endl;
      ABORT("no file for db");
    }
    size_t i = file.rfind('/');
    if (i == std::string::npos) {
      dir = params::get<std::string>(DB_DIR, "");
      cacheSize = params::get(DB_DIR + "." + CACHE, 0);
      logMemory = params::get(DB_DIR + "." + LOG, 0);
      keySize = params::get(DB_DIR + "." + KEY_SIZE, keySize);
      dataSize = params::get(DB_DIR + "." + DATA_SIZE, dataSize);
    }
    else {
      dir = file.substr(0, i);
      file = file.substr(i + 1);
      cacheSize = params::get(name + "." + CACHE, 0);
      logMemory = params::get(name + "." + LOG, 0);
      keySize = params::get(name + "." + KEY_SIZE, keySize);
      dataSize = params::get(name + "." + DATA_SIZE, dataSize);
    }

    if (!dbsubdir.empty()) {
      dir += "/" + dbsubdir;
    }
    
    if (dir.empty()) {
      Log::err() << "no db dir defined for db " << name << Log::endl;
      ABORT("no dir for db");
    }
  } // parseDbPaths

  static void parseDbPath(const std::string& name, std::string& path,
			  bool& sync, int32_t& lcnum, int32_t& ncnum) {
    std::string file = params::get<std::string>(name + "." + FILE, "");
    if (file.empty()) {
      Log::err() << "no file defined for db " << name << Log::endl;
      ABORT("no file for db");
    }
    size_t i = file.rfind('/');
    if (i == std::string::npos) {
      std::string dir = params::get<std::string>(DB_DIR, "");
      if (!dir.empty()) {
	path = dir + "/" + file;
	lcnum = params::get(dir + "." + LCACHE, lcnum);
	ncnum = params::get(dir + "." + NCACHE, ncnum);
      }
    }
    else {
      path = file;
      lcnum = params::get(name + "." + LCACHE, lcnum);
      ncnum = params::get(name + "." + NCACHE, ncnum);
    }
    sync = params::get(name + "." + SYNC, true);
  }

}; // class MapConfig


#endif // _MAP_CONFIG_H
