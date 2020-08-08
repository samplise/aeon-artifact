/* 
 * Log.cc : part of the Mace toolkit for building distributed systems
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
#include "Log.h"
#include "FileUtil.h"
#include "StrUtil.h"
#include "SockUtil.h"
#include "Util.h"
#include "TimeUtil.h"
#include "pip_includer.h"
#include "params.h"
#include "Accumulator.h"
#include "LoadMonitor.h"
#include "MatchingReader.h"
#include "ScopedLogReader.h"
#include "CircularBuffer.h"
#include "ScopedLog.h"
#include <limits.h>
#include <fstream>
#include <assert.h>
#include <boost/format.hpp>

#define  DEFAULT_MACE_BIN_LOG_FILE  "binarylog"

using namespace std;
using namespace MatchingReader_namespace;
using namespace ScopedLogReader_namespace;

bool Log::enabled = true;
bool Log::autoAll = false;
bool Log::errorSelected = true;
bool Log::warningSelected = true;
bool Log::flushing = false;
const log_level_t Log::DEFAULT_LEVEL = 0;
const log_level_t Log::MAX_LEVEL = UINT_MAX;
log_level_t Log::logLevel = DEFAULT_LEVEL;
Log::SelectorMap Log::selectors;
Log::IdSelectorList Log::selectorLists(1);
Log::LogFileMap Log::logFiles;
Log::LogBufferMap Log::logBuffers;
const uint64_t Log::DEFAULT_LOG_BUFFER_SIZE = 16*1024*1024;
const uint32_t Log::ThreadSpecific::DEFAULT_TIME_LIST_SIZE = 1*1000*1000;
const uint32_t Log::ThreadSpecific::DEFAULT_BIN_LIST_SIZE = 100*1000;
Log::SelectorList Log::autoSelectors;
Log::SelectorIdMap Log::ids;
Log::IdSelectorMap Log::reverseIds;
LogSelector* Log::autoAllDefault = 0;
pthread_mutex_t Log::llock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Log::bufllock = PTHREAD_MUTEX_INITIALIZER;
uint32_t Log::idCount = 1;
const log_id_t Log::NULL_ID = 0;
const size_t Log::MAX_MESSAGE_SIZE = 4096;
Log::NullOutputType Log::nullOutputVar;
Log::LogFlushManipulator Log::end;
Log::LogFlushManipulator Log::endl;
std::ifstream Log::replayStream;
FILE* Log::sqlCreatesLog;
FILE* Log::sqlEventsLog;

pthread_key_t Log::ThreadSpecific::pkey;
pthread_once_t Log::ThreadSpecific::keyOnce = PTHREAD_ONCE_INIT;
unsigned int Log::ThreadSpecific::count = 0;
Log::ThreadSpecific::ThreadSpecificList Log::ThreadSpecific::threads;
uint64_t Log::ScopedTimeLog::counter = 0;


static FILE* binout = NULL;

// Log::ScopedTimeLog(log_id_t sel, const std::string& msg) :
//   selector(sel), message(msg), start(TimeUtil::timeu()) {
  
// }


Log::ThreadSpecific::ThreadSpecific() {
  count++;
  vtid = count;
} // ThreadSpecific

Log::ThreadSpecific::~ThreadSpecific() {
} // ~ThreadSpecific

Log::ThreadSpecific* Log::ThreadSpecific::init() {
  static const bool PROFILE_SCOPED_TIMES = params::get(params::MACE_PROFILE_SCOPED_TIMES, false);
  static const bool BIN_MEM = params::get(params::MACE_LOG_BIN_BUFFERS, false);
  pthread_once(&keyOnce, Log::ThreadSpecific::initKey);
  ThreadSpecific* t = (ThreadSpecific*)pthread_getspecific(pkey);
  if (t == 0) {
    t = new ThreadSpecific();
    assert(pthread_setspecific(pkey, t) == 0);
    if (PROFILE_SCOPED_TIMES) {
      t->timelogs.reserve(params::get("MACE_LOG_PROFILE_TIME_RESERVE", DEFAULT_TIME_LIST_SIZE));
    }
    if (BIN_MEM) {
      t->binlogs.reserve(params::get("MACE_LOG_MEM_BIN_RESERVE", DEFAULT_BIN_LIST_SIZE));
    }
    lock();
    threads.push_back(t);
    unlock();
  }

  return t;
} // init

void Log::ThreadSpecific::initKey() {
  assert(pthread_key_create(&pkey, NULL) == 0);
} // initKey

unsigned int Log::ThreadSpecific::getVtid() {
  ThreadSpecific* t = (ThreadSpecific*)init();
  return t->vtid;
} // getVtid

Log::StreamMap& Log::ThreadSpecific::getStreams() {
  ThreadSpecific* t = (ThreadSpecific*)init();
  return t->streams;
} // getStreams

Log::TraceStreamMap& Log::ThreadSpecific::getTraceStreams() {
  ThreadSpecific* t = (ThreadSpecific*)init();
  return t->traceStreams;
} // getStreams

// Log::ThreadSpecific::TimeList& Log::ThreadSpecific::getScopedTimes() {
//   return t->timelogs;
// } // getScopedTimes

void Log::ThreadSpecific::logScopedTime(ScopedTimeLog* stl) {
  ThreadSpecific* t = (ThreadSpecific*)init();
  stl->tid = t->vtid;
  stl->end = TimeUtil::timeu();
  stl->setEndCount();
  t->timelogs.push_back(stl);
}

void Log::ThreadSpecific::binaryLog(log_id_t sel, mace::BinaryLogObject* o) {
  ThreadSpecific* t = (ThreadSpecific*)init();
  o->timestamp = TimeUtil::timeu();
  o->selector = sel;
  o->tid = t->vtid;
  t->binlogs.push_back(o);
}

Log::ScopedTimeLog::ScopedTimeLog(log_id_t sel, const std::string& msg) :
  selector(sel), message(msg), start(TimeUtil::timeu()), end(0), tid(0),
  startCount(0), endCount(0) {
  counter++;
  startCount = counter;
}

log_id_t Log::getId(const std::string& selector) {
  log_id_t r = NULL_ID;
  
  lock();

  const LogSelector* sqlLogSel = 0;

  SelectorIdMap::iterator i = ids.find(selector);
  if (i == ids.end()) {
    if (isSelected(selector)) {
      SelectorList* l = selectors[selector];
      r = idCount;
      idCount++;
      selectorLists.push_back(l);
//       streams.push_back(new ThreadStreamMap());

      // If binary logging is enabled, then log the log id and its selector so that we can
      // match them up later on during deserialization
      for (SelectorList::const_iterator i = l->begin(); i != l->end(); i++) {
        const LogSelector* sel = *i;
        if (sel->getLogSelectorOutput() == LOG_BINARY) {
	  ABORT("LOG_BINARY not currently supported");
//           // writes a "Matching" log, which matches the selector id (r) to the selector string (selector)
//           //           std::cout << "M(Matching) " << r << " => " << selector << std::endl;
//           writeBinaryLog(r, sel, "M", selector);
//           break;
        }
	else if (sel->getLogSelectorOutput() == LOG_PGSQL) {
	  sqlLogSel = sel;
	}
      }

      // XXX - this is a hack
      if (selector == "MethodMap" && sqlLogSel == 0) {
	r = NULL_ID;
      }
      else {
	reverseIds[r] = selector;
      }
    }
    ids[selector] = r;
  }
  else {
    r = i->second;
  }

  unlock();

  if (sqlLogSel != 0) {
    binaryLog(r, MatchingReader(r, selector));
  }

  return r;
} // getId

const std::string& Log::getSelector(log_id_t id) {
  static const std::string empty;
  if (id == NULL_ID) {
    return empty;
  }
  Log::IdSelectorMap::const_iterator i = reverseIds.find(id);
  if (i != reverseIds.end()) {
    return i->second;
  }
  ASSERT(0);
} // getSelector

Log::MaceOutputStream& Log::log(const std::string& selector, log_level_t level) {
  return log(getId(selector), level);
} // log

void Log::log(log_id_t id, log_id_t level, const std::string& m) {
  if (!enabled || id == NULL_ID || level > logLevel) {
    return;
  }

  log(id, level, m.c_str());
} // log

void Log::log(log_id_t id, log_id_t level, const char* m,
	      uint64_t timestamp, uint32_t vtid) {
  if (!enabled || id == NULL_ID || level > logLevel) {
    return;
  }
  // WARNING - assumes selector list exists for id
  // does not need to be locked because l is a linked list
  SelectorList* l = selectorLists[id];
  for (SelectorList::const_iterator i = l->begin(); i != l->end(); i++) {
    const LogSelector* sel = *i;
    switch(sel->getLogSelectorOutput()) {
      case LOG_BINARY:
        writeBinaryLog(id, sel, "S", m);
      break;
      case LOG_FPRINTF:
      case LOG_PIP:
        writeTextLog(sel, m, timestamp, vtid); 
      break;
      case LOG_PGSQL:
	// do nothing for string logs
	break;
      default:
      ABORT("Unrecognized log style");
    }
  }
} // log

void Log::logf(log_id_t id, log_level_t level, const char* f ...) {
  if (!enabled || id == NULL_ID || level > logLevel) {
    return;
  }

  va_list ap;
  va_start(ap, f);
  char tmp[MAX_MESSAGE_SIZE];
  vsnprintf(tmp, MAX_MESSAGE_SIZE - 1, f, ap);
  va_end(ap);
  log(id, level, tmp);
} // logf

void Log::logf(log_id_t id, const char* f ...) {
  if (!enabled || id == NULL_ID) {
    return;
  }

  va_list ap;
  va_start(ap, f);
  char tmp[MAX_MESSAGE_SIZE];
  vsnprintf(tmp, MAX_MESSAGE_SIZE - 1, f, ap);
  va_end(ap);
  log(id, tmp);
} // logf

Log::MaceOutputStream& Log::log(log_id_t id, log_level_t level) {
  static MaceOutputStream* nullOutStr = new MaceOutputStream();
  if (id == NULL_ID || level > logLevel) { 
    return *nullOutStr;
  }
  StreamMap& m = ThreadSpecific::getStreams();

  // this does not need to be locked because this is thread-specific data
  StreamMap::iterator i = m.find(id);
  MaceOutputStream* r = 0;
  if (i == m.end()) {
    r = new MaceOutputStream(id);
    m[id] = r;
  }
  else {
    r = i->second;
  }
  
//   pthread_t tid = pthread_self();
//   lock();
//   MaceOutputStream* r = findStream(id, tid);
//   assert(r);
//   unlock();

  return *r;
} // log

void Log::binaryLog(log_id_t id, const std::string& log_type, 
		    const std::string& serializedObj, log_level_t level) {
  
  ABORT("HUH?");
  if (!enabled || id == NULL_ID || level > logLevel) {
    return;
  }
  
  // WARNING - assumes selector list exists for id
  // does not need to be locked because l is a linked list
  SelectorList* l = selectorLists[id];
  for (SelectorList::const_iterator i = l->begin(); i != l->end(); i++) {
    const LogSelector* sel = *i;
    if (sel->getLogSelectorOutput() == LOG_BINARY) {
      writeBinaryLog(id, sel, log_type, serializedObj);
    }
  }
} // binaryLog

void Log::binaryLog(log_id_t id, mace::BinaryLogObject* object, log_level_t level) {
  static const bool BIN_MEM = params::get(params::MACE_LOG_BIN_BUFFERS, false);
  if (!enabled || id == NULL_ID || level > logLevel) {
    delete object;
    return;
  }
  if (BIN_MEM) {
    Log::ThreadSpecific::binaryLog(id, object);
  }
  else {
    binaryLog(id, *object, level);
    delete object;
  }
}

void Log::binaryLog(log_id_t id, const mace::BinaryLogObject& object, log_level_t level, uint64_t timestamp, uint32_t tid) {
//void Log::binaryLog(log_id_t id, const std::string& log_type, 
//		    const std::string& serializedObj, log_level_t level) {
  if (!enabled || id == NULL_ID || level > logLevel) {
    return;
  }
  
  std::string serializedObj;
  std::string text;
  // WARNING - assumes selector list exists for id
  // does not need to be locked because l is a linked list
  SelectorList* l = selectorLists[id];
  for (SelectorList::const_iterator i = l->begin(); i != l->end(); i++) {
    const LogSelector* sel = *i;
    switch(sel->getLogSelectorOutput()) {
      case LOG_BINARY: {
        if (serializedObj == "") {
          mace::serialize(serializedObj, &object);
        }
        writeBinaryLog(id, sel, object.getLogType(), serializedObj);
      }
      break;
      case LOG_FPRINTF: 
      case LOG_PIP: { 
        if (text.empty()) {
          text = /*object.getLogDescription() + ": " +*/ object.toString();
        }
	writeTextLog(sel, text.c_str(), timestamp, tid);
      }
      break;
      case LOG_PGSQL: {
	static bool pgSqlInit = false;
	static bool initializing = false;
	struct timeval clock_;
	struct timezone tz_;
	gettimeofday(&clock_, &tz_);
	
	if (initializing) {
	  return;
	}

	if (!pgSqlInit) {
	  initializing = true;
	  initDbLogs();
	  initializing = false;
	  pgSqlInit = true;
	}
	mace::LogNode*& n = object.getChildRoot();
	if (n == NULL) {
	  n = new mace::LogNode(object.getLogType(), sqlEventsLog);
// 	  n->out.open((object.getLogType() + ".log").c_str());
	}
	
// 	double now = clock_.tv_sec + (double)clock_.tv_usec/1000000.0;
  	uint64_t now = (((uint64_t)clock_.tv_sec * 1000000) + clock_.tv_usec);
	unsigned int tid = Log::ThreadSpecific::getVtid();
	int nextEvent = mace::nextEventId();
	
	if (nextEvent == 0) {
	  ostringstream out;
	  ostringstream out2;
	  
	  out << "CREATE TABLE __events (_id INT PRIMARY KEY, "
	      << "time NUMERIC(20, 0), "
	      << "joinId INT, selId INT, tid INT, tname TEXT, "
	      << "seid INT);" << std::endl;
	  ASSERT(fwrite(out.str().c_str(), out.str().size(), 1, sqlCreatesLog) > 0);
	  // always map the __events table to id 0, other tables start at 1
	  out2 << "map\t__events\t0" << std::endl;
	  ASSERT(fwrite(out2.str().c_str(), out2.str().size(), 1, Log::sqlEventsLog) > 0);
	}
	ostringstream o;
// 	o << "0\t" << nextEvent << "\t" << boost::format("%.6f") % now << "\t";
	o << "0\t" << nextEvent << "\t" << now << "\t";
	if (object.getLogType() == ScopedLogReader::type) {
	  // special case for scoped logs: use the "begin" field as the index
	  // in the events table
	  const ScopedLogReader& r = (const ScopedLogReader&)object;
	  o << (r.begin ? '1' : '0');
	}
	else {
	  o << n->nextId;
	}
	o << "\t" << id << "\t" << tid << "\t" 
	  << n->logName << "\n";
	
	ASSERT(fwrite(o.str().c_str(), o.str().size(), 1, sqlEventsLog) > 0);
	
 	object.sqlize(n);
	
	if (n->nextId == 0) {
	  // I don't think this should happen
	  std::cerr << "cannot sqlize " << object.getLogDescription() << std::endl;
	  ABORT("Object sqlized to nothing");
	}
      }
      break;
      default:
        ABORT("INVALID LogSelectorOutput");
    }
  }
} // binaryLog


/**
 * \pre \f$ id != NULL\_ID \or level > logLevel \f$
 * \pre \f$ sel->getLogSelectorOutput() == LOG\_BINARY \f$
 */
void Log::writeBinaryLog(log_id_t id, const LogSelector* sel, const std::string& log_type, const std::string& serializedObj) {
  static const bool FLUSH = !params::get(params::MACE_LOG_ASYNC_FLUSH, false);
  ASSERT(!log_type.empty());
  std::string str;
  
  ABORT("NOT SUPPORTED CURRENTLY!");
  // serializing log header, which include logid, log selector header (timestamp, tid), log type
  mace::serialize(str, &id);
  str += sel->getSerializedSelectorHeader();
  mace::serialize(str, &log_type);

  // serialize and write out the size of the log content and then write out the log content
  mace::serialize(str, &serializedObj);
  ASSERT(fwrite(str.data(), 1, str.size(), sel->getFile()) > 0);
  //   std::cout << "HEX: " << Log::toHex(str) << std::endl;

  if (FLUSH){
    fflush(sel->getFile());
  }
} // writeBinaryLog

/**
 * \pre \f$ id != NULL\_ID \or level > logLevel \f$
 * \pre \f$ sel->getLogSelectorOutput() == LOG\_FPRINTF \or sel->getLogSelectorOutput() == LOG\_PIP \f$
 */
void Log::writeTextLog(const LogSelector* sel, const char* text,
		       uint64_t timestamp, uint32_t vtid) {
  static const bool FLUSH = !params::get(params::MACE_LOG_ASYNC_FLUSH, false);
  static const bool SCOPED_LOG_TIMES = params::get(params::MACE_LOG_SCOPED_TIMES,
						   false);
  static const bool USE_LOGBUF = params::get(params::MACE_LOG_BUFFERS, false);
  static uint64_t LOGBUF_SIZE = params::get(params::MACE_LOG_BUFFER_SIZE, DEFAULT_LOG_BUFFER_SIZE);
  if(sel->getLogSelectorOutput() == LOG_FPRINTF) {	// normal logging
    if (USE_LOGBUF && !flushing && sel->getFile() != stderr) {
      ScopedLock sl(bufllock);
      CircularBuffer* buf = 0;
      LogBufferMap::iterator i = logBuffers.find(sel->getFileNo());
      if (i == logBuffers.end()) {
	buf = new CircularBuffer(LOGBUF_SIZE);
	logBuffers[sel->getFileNo()] = buf;
      }
      else {
	buf = i->second;
      }
      string h = sel->getSelectorHeader(timestamp, vtid);
      buf->write(h.c_str(), h.size());
      buf->write(text, strlen(text));
      buf->write("\n", 1);
    }
    else {
      fprintf(sel->getFile(), "%s%s\n",
	      sel->getSelectorHeader(timestamp, vtid).c_str(), text);
    }
  } else {
    ASSERT(sel->getLogSelectorOutput() == LOG_PIP);
#ifdef PIP_MESSAGING
    ANNOTATE_NOTICE(sel->getSelectorHeader().c_str(), 4 + level, "%s", text);
#else
    fprintf(sel->getFile(), "%s%s\n", sel->getSelectorHeader().c_str(), text);
#endif
  }
  if (FLUSH && !SCOPED_LOG_TIMES && !flushing) {
    fflush(sel->getFile());
  }
}

void Log::initDbLogs() {
  string prefix = params::get(params::MACE_LOG_PREFIX, string());
  string logpath = params::get(params::MACE_LOG_PATH, string());
  if (!logpath.empty()) {
//     FileUtil::mkdir(logpath, S_IRWXU|S_IRWXG|S_IRWXO, true);
    FileUtil::mkdir(logpath, S_IRWXU, true);
    if (logpath[logpath.size() - 1] != '/') {
      logpath.append("/");
    }
  }

  string clog = logpath + prefix + "sqlCreates.log";
  string elog = logpath + prefix + "sqlEvents.log";
  sqlCreatesLog = fopen(clog.c_str(), "w");
  sqlEventsLog = fopen(elog.c_str(), "w");
  ASSERT(sqlCreatesLog);
  ASSERT(sqlEventsLog);
}

void Log::flush(MaceOutputStream& str) {
  // WARNING - assumes that str is registered
  if (str.stream != NULL) {
    log(str.id, str.stream->str().c_str());
    str.stream->str("");
  }
} // flush

void Log::flush() {
  ScopedLock sl(llock);
  static const bool PROFILE_SCOPED_TIMES = params::get(params::MACE_PROFILE_SCOPED_TIMES, false);
  static const bool USE_LOGBUF = params::get(params::MACE_LOG_BUFFERS, false);
  static const bool BIN_MEM = params::get(params::MACE_LOG_BIN_BUFFERS, false);
  if (USE_LOGBUF) {
    for (LogBufferMap::iterator i = logBuffers.begin();
	 i != logBuffers.end(); i++) {
      int fd = i->first;
      CircularBuffer* buf = i->second;
      buf->copy(fd);
    }
  }
  if (PROFILE_SCOPED_TIMES || BIN_MEM) {
    flushing = true;
    for (Log::ThreadSpecific::ThreadSpecificList::iterator ti =
	   Log::ThreadSpecific::threads.begin();
	 ti != Log::ThreadSpecific::threads.end(); ti++) {
      Log::ThreadSpecific* thr = *ti;
      for (Log::ThreadSpecific::BinaryLogList::const_iterator i =
	     thr->binlogs.begin();
	   i != thr->binlogs.end(); i++) {
	mace::BinaryLogObject* bin = *i;
	binaryLog(bin->selector, *bin, DEFAULT_LEVEL, bin->timestamp, bin->tid);
	delete bin;
      }
      thr->binlogs.clear();
      for (Log::ThreadSpecific::TimeList::const_iterator i =
	     thr->timelogs.begin();
	   i != thr->timelogs.end(); i++) {
	Log::ScopedTimeLog* stl = *i;
	string sm = stl->message + " STARTING " +
	  StrUtil::toString(stl->startCount);
	string em = stl->message + " ENDING " + StrUtil::toString(stl->endCount);
	log(stl->selector, DEFAULT_LEVEL, sm.c_str(), stl->start, stl->tid);
	log(stl->selector, DEFAULT_LEVEL, em.c_str(), stl->end, stl->tid);
      }
      thr->timelogs.clear();
    }
    flushing = false;
  }
  
  for (SelectorMap::const_iterator mi = selectors.begin();
       mi != selectors.end(); mi++) {
    const SelectorList* l = mi->second;
    for (SelectorList::const_iterator i = l->begin(); i != l->end(); i++) {
      const LogSelector* sel = *i;
      fflush(sel->getFile());
    }
  }
} // flush

void Log::autoAddAll(FILE* f,
		     LogSelectorTimestamp lt,
		     LogSelectorName ln,
		     LogSelectorThreadId ltid,
                     LogSelectorOutput lso) {

  if (autoAllDefault != 0) {
    delete autoAllDefault;
  }

  autoAll = true;
  autoAllDefault = new LogSelector("", f, lt, ln, ltid, lso);
} // autoAddAll

void Log::autoAdd(const string& s,
		  FILE* f,
		  LogSelectorTimestamp lt,
		  LogSelectorName ln,
		  LogSelectorThreadId ltid,
                  LogSelectorOutput lso) {

  LogSelector* sel = new LogSelector(s, f, lt, ln, ltid, lso);
  autoSelectors.push_back(sel);
} // autoAdd

void Log::add(const string& s,
	      FILE* f,
	      LogSelectorTimestamp lt,
	      LogSelectorName ln,
	      LogSelectorThreadId ltid,
              LogSelectorOutput lso) {
  lock();

  // this will add any auto selectors
  isSelected(s);

  if (lso == LOG_BINARY) {
    if (binout == NULL) {
      binout = openLogFile(params::get<std::string>(params::MACE_BIN_LOG_FILE, DEFAULT_MACE_BIN_LOG_FILE), "wb");
    }
    f = binout;
  }

  addl(s, f, lt, ln, ltid, lso);
  unlock();
} // add

void Log::addl(const string& s,
	       FILE* f,
	       LogSelectorTimestamp lt,
	       LogSelectorName ln,
	       LogSelectorThreadId ltid,
               LogSelectorOutput lso) {

  // actually updates the data structures; assumes caller holds lock

  SelectorList* l;
  SelectorMap::iterator i = selectors.find(s);
  if (i != selectors.end()) {
    l = i->second;
  }
  else {
    l = new SelectorList();
    selectors[s] = l;
  }

  // delete the selector if it exists
  remove(l, f);

  LogSelector* sel = new LogSelector(s, f, lt, ln, ltid, lso);
  l->push_back(sel);

} // addl

void Log::remove(const string& s, FILE* f) {
  if (selectors.find(s) != selectors.end()) {
    SelectorList* l = selectors[s];
    remove(l, f);
  }
} // remove

void Log::remove(SelectorList* l, FILE* f) {
  SelectorList::iterator i = l->begin();
  while (i != l->end()) {
    if ((*i)->getFile() == f) {
      delete *i;
      *i = 0;
      i = l->erase(i);
    }
    else {
      i++;
    }
  }
} // remove

void Log::removeAll(const string& s) {
  if (selectors.find(s) != selectors.end()) {
    SelectorList* l = selectors[s];
    for (SelectorList::iterator i = l->begin(); i != l->end(); i++) {
      delete *i;
    }
    delete l;
    selectors.erase(s);
  }
} // removeAll

void Log::logToFile(const string& path, const string& sel, const char* mode,
		    LogSelectorTimestamp sts, LogSelectorName sn,
		    LogSelectorThreadId stid) {
  FILE* f = openLogFile(path, mode);
  add(sel, f, sts, sn, stid);
} // logToFile

void Log::logSqlCreate(const std::string& createString, 
		       const mace::LogNode* node) {
  std::ostringstream out;
  
  ASSERT(fwrite(createString.c_str(), createString.size(), 1, Log::sqlCreatesLog) > 0);
  out << "map\t" << node->logName << "\t" << node->logId << std::endl;
  ASSERT(fwrite(out.str().c_str(), out.str().size(), 1, Log::sqlEventsLog) > 0);
  fflush(Log::sqlCreatesLog);
} // logSqlCreate

FILE* Log::openLogFile(const string& path, const char* mode) {
  if (logFiles.find(path) != logFiles.end()) {
    return logFiles[path];
  }
  
  FILE* f = fopen(path.c_str(), mode);
  if (f == 0) {
    fprintf(stderr, "could not open log file %s, aborting\n", path.c_str());
    ::perror("open");
    exit(-1);
  }

  logFiles[path] = f;
  return f;
} // openLogFile

void Log::closeLogFiles() {
  for (LogFileMap::iterator i = logFiles.begin(); i != logFiles.end(); i++) {
    FILE* f = i->second;
    fclose(f);
  }
} // closeLogFiles

void Log::enableLogging() {
  enabled = true;
} // enableLogging

bool Log::disableLogging() {
  bool ret = enabled;
  enabled = false;
  return ret;
} // disableLogging

void Log::log(const std::string& selector, log_level_t level, const string& message) {
  if (!enabled || level > logLevel) {
    return;
  }

  log(selector, message.c_str());
} // log

bool Log::isSelected(const string& s) {
  bool r = true;
  if (selectors.find(s) == selectors.end()) {
    r = false;
    if (errorSelected && s.find("ERROR") != string::npos) {
      addl(s,
	   stderr,
	   LOG_TIMESTAMP_DISABLED,
	   LOG_NAME_ENABLED,
	   LOG_THREADID_DISABLED,
	   LOG_FPRINTF);
      r = true;
    }
    if (warningSelected && s.find("WARNING") != string::npos) {
      addl(s,
	   stdout,
	   LOG_TIMESTAMP_DISABLED,
	   LOG_NAME_ENABLED,
	   LOG_THREADID_DISABLED,
	   LOG_FPRINTF);
      r = true;
    }
    if (autoAll) {
      ASSERT(autoAllDefault != 0);
      FILE* out = autoAllDefault->getFile();
      if (autoAllDefault->getLogSelectorOutput() == LOG_BINARY) {
        if (binout == NULL) {
          binout = openLogFile(params::get<std::string>(params::MACE_BIN_LOG_FILE, DEFAULT_MACE_BIN_LOG_FILE), "wb");
        }
        out = binout;
      }
//       cerr << "adding auto all selector for " << s 
// 	   << " to " << (void*)out << std::endl;
      addl(s,
	   out, 
	   autoAllDefault->getLogTimestamp(),
	   autoAllDefault->getLogName(),
	   autoAllDefault->getLogThreadId(),
           autoAllDefault->getLogSelectorOutput());
      r = true;
    }
    if (!autoSelectors.empty()) {
      for (SelectorList::iterator i = autoSelectors.begin();
          i != autoSelectors.end(); i++) {
        LogSelector* sel = *i;
        if (StrUtil::matches(sel->getName(), s)) {
          FILE* out = sel->getFile();
          if (sel->getLogSelectorOutput() == LOG_BINARY) {
            if (binout == NULL) {
              binout = openLogFile(params::get<std::string>(params::MACE_BIN_LOG_FILE, DEFAULT_MACE_BIN_LOG_FILE), "wb");
            }
            out = binout;
          }
// 	  cerr << "adding auto selector for " << s 
// 	       << " " << sel->getName()
// 	       << " to " << (void*)out << std::endl;
          addl(s,
	       out,
	       sel->getLogTimestamp(),
	       sel->getLogName(),
	       sel->getLogThreadId(),
	       sel->getLogSelectorOutput());
          r = true;
        }
      }
    }
  }
  return r;
} // isSelected

void Log::log(const string& s, log_level_t level, const char* m) {
  if (!enabled || level > logLevel) {
    return;
  }

  log(getId(s), level, m);
} // log

void Log::logf(const string& s, log_level_t level, const char* f ...) {
  if (!enabled || level > logLevel) {
    return;
  }

  va_list ap;
  va_start(ap, f);
  char tmp[MAX_MESSAGE_SIZE];
  vsnprintf(tmp, MAX_MESSAGE_SIZE - 1, f, ap);
  va_end(ap);
  log(s, level, tmp);
} // logf

void Log::logf(const string& s, const char* f ...) {
  if (!enabled) {
    return;
  }

  va_list ap;
  va_start(ap, f);
  char tmp[MAX_MESSAGE_SIZE];
  vsnprintf(tmp, MAX_MESSAGE_SIZE - 1, f, ap);
  va_end(ap);
  log(s, tmp);
} // logf

string Log::toHex(const string& h) {
  string r = "";
  for (size_t i = 0; i < h.size(); i++) {
    char c[3];
    sprintf(c, "%02hhx", (unsigned char)h[i]);
    r += c;
  }
  return r;
} // toHex

void Log::configure() {
  LogSelectorTimestamp lts = LOG_TIMESTAMP_EPOCH;
  if (params::get(params::MACE_LOG_TIMESTAMP_HUMAN, false)) {
    lts = LOG_TIMESTAMP_HUMAN;
  }

  if (params::containsKey(params::MACE_LOG_LEVEL)) {
    int l = params::get<int>(params::MACE_LOG_LEVEL);
    setLevel(l == -1 ? MAX_LEVEL : l);
  }

  if (params::get(params::MACE_LOG_DISABLE_WARNING, false)) {
    disableDefaultWarning();
  }

  string filemode = "w";
  if (params::get(params::MACE_LOG_APPEND, false)) {
    filemode = "a";
  }

  string prefix = params::get(params::MACE_LOG_PREFIX, string());
  string logpath = params::get(params::MACE_LOG_PATH, string());
  if (!logpath.empty()) {
//     FileUtil::mkdir(logpath, S_IRWXU|S_IRWXG|S_IRWXO, true);
    FileUtil::mkdir(logpath, S_IRWXU, true);
    if (logpath[logpath.size() - 1] != '/') {
      logpath.append("/");
    }
  }
  FILE* fp = stdout;
  if (params::containsKey(params::MACE_LOG_FILE)) {
    StringList s = StrUtil::split("\n", params::get<string>(params::MACE_LOG_FILE));
    for (StringList::const_iterator i = s.begin(); i != s.end(); i++) {
//       cerr << "configuring " << *i << std::endl;
      StringList l = StrUtil::split(" ", *i);
      if (l.size() == 1) {
	string fullPath = logpath + prefix + l[0];
	fp = openLogFile(logpath + prefix + l[0], filemode.c_str());
//  	cerr << "logging auto selectors to " << fullPath
// 	     << " " << (void*)fp << std::endl;
      }
      else {
	string path = l.front();
	string fullPath = logpath + prefix + path;
	FILE* tfp = openLogFile(fullPath, filemode.c_str());
	l.pop_front();
	
	for (StringList::const_iterator li = l.begin(); li != l.end(); li++) {
//  	  cerr << "logging " << *li << " to " << fullPath << " "
// 	       << (void*)tfp << std::endl;
// 	  logToFile(logpath + prefix + path, *li, filemode.c_str(), lts);
	  if (l.size() == 1 && !StrUtil::isRegex(*li)) {
	    autoAdd(*li, tfp, lts, LOG_NAME_DISABLED, LOG_THREADID_DISABLED);
	  }
	  else {
	    autoAdd(*li, tfp, lts);
	  }
	}
      }
    }
    
  }

//   cerr << "using default fp " << (void*)fp << std::endl;

  if (params::get(params::MACE_LOG_AUTO_ALL, false)) {
    autoAddAll(fp, lts);
  }
  else {
    if (params::containsKey(params::MACE_LOG_AUTO_SELECTORS)) {
      string tmp = params::get<string>(params::MACE_LOG_AUTO_SELECTORS);

      StringList s = StrUtil::split(" ", tmp);
      for (StringList::const_iterator i = s.begin(); i != s.end(); i++) {
	autoAdd(*i, fp, lts);
      }
    }

    if (params::containsKey(params::MACE_LOG_AUTO_BINARY)) {
      string tmp = params::get<string>(params::MACE_LOG_AUTO_BINARY);

      StringList s = StrUtil::split(" ", tmp);
      for (StringList::const_iterator i = s.begin(); i != s.end(); i++) {
	autoAdd(*i, stdout, lts, LOG_NAME_ENABLED, LOG_THREADID_ENABLED, LOG_BINARY);
      }
    }
  }

  if (params::containsKey(params::MACE_LOG_ACCUMULATOR)) {
    uint64_t interval = params::get(params::MACE_LOG_ACCUMULATOR, 0);
    Accumulator::startLogging(interval*1000);
  }

  if (params::containsKey(params::MACE_LOG_LOADMONITOR)) {
    uint64_t interval = params::get(params::MACE_LOG_LOADMONITOR, 0);
    HiResLoadMonitor::start(interval*1000);
  }
    
} // configure

void Log::perror(const std::string& s) {
  int err = SockUtil::getErrno();
  Log::err()     << s << ": errno " << errno << ": " << Util::getErrorString(errno);
  std::cerr      << s << ": errno " << errno << ": " << Util::getErrorString(errno);
  if (errno != err) {
    Log::err()   << ", socket " << err << ": " << Util::getErrorString(err);
    std::cerr    << ", socket " << err << ": " << Util::getErrorString(err);
  }
  Log::err() << Log::endl;
  std::cerr  << std::endl;
} // Log::perror

void Log::sslerror(const std::string& s) {
  Log::err() << s << ": " << Util::getSslErrorString() << Log::endl;
  std::cerr << s << ": " << Util::getSslErrorString() << std::endl;
} // sslerror

void Log::lock() {
  assert(pthread_mutex_lock(&llock) == 0);
} // lock

void Log::unlock() {
  assert(pthread_mutex_unlock(&llock) == 0);
} // unlock

log_id_t Log::getTraceId(const std::string& selector) {
  static const bool isTrace = params::containsKey(params::MACE_TRACE_FILE);
  if (isTrace) {
    return UINT_MAX;
  }
  else {
    return NULL_ID;
  }
} // getTraceId

Log::MaceTraceStream& Log::trace(log_id_t id) {
  TraceStreamMap& m = ThreadSpecific::getTraceStreams();

  // this does not need to be locked because this is thread-specific data
  TraceStreamMap::iterator i = m.find(id);
  MaceTraceStream* r = 0;
  if (i == m.end()) {
    r = new MaceTraceStream(id);
    m[id] = r;
  }
  else {
    r = i->second;
  }

  return *r;
} // trace

void Log::flush(Log::MaceTraceStream& s) {
  if (s.isNoop()) {
    return;
  }
  static FILE* out = openLogFile(params::get<std::string>(params::MACE_TRACE_FILE), "wb");

  unsigned int vtid = ThreadSpecific::getVtid();
  vtid = htonl(vtid);
  ASSERT(fwrite(&vtid, sizeof(vtid), 1, out) > 0);
  ASSERT(fwrite(s.stream->str().data(), 1, s.stream->str().size(), out) > 0);
  fflush(out);
  s.stream->str("");
} // flush

void Log::traceNewThread(const mace::string& fname) {
  if (Util::REPLAY) {
    scheduleThread(UINT_MAX, fname);
  }
  else {
    if (!params::containsKey(params::MACE_TRACE_FILE)) {
      return;
    }
    static FILE* out = openLogFile(params::get<std::string>(params::MACE_TRACE_FILE), "wb");

    unsigned int vtid = htonl(UINT_MAX);
    ASSERT(fwrite(&vtid, sizeof(vtid), 1, out) > 0);
    std::string s;
    mace::serialize(s, &fname);
    ASSERT(fwrite(s.data(), 1, s.size(), out) > 0);
    fflush(out);
  }
} // traceNewThread

std::istream& Log::replay(log_id_t id) {
  scheduleThread(ThreadSpecific::getVtid(), "");
  return replayStream;
} // replay

void Log::scheduleThread(unsigned int vtid, const mace::string& desc) {
  static pthread_cond_t sig = PTHREAD_COND_INITIALIZER;
  static uint curId = UINT_MAX;
  static mace::string curDesc = desc;

  ASSERT(Util::REPLAY);

  lock();
  if (!replayStream.is_open()) {
    replayStream.open(params::get<std::string>(params::MACE_TRACE_FILE).c_str());
  }
  ASSERT(replayStream);

  if (vtid != UINT_MAX) {
    mace::deserialize(replayStream, &curId);
    if (curId == UINT_MAX) {
      mace::deserialize(replayStream, &curDesc);
    }
    pthread_cond_broadcast(&sig);
  }

  while (vtid != curId || (vtid == UINT_MAX && desc != curDesc)) {
    pthread_cond_wait(&sig, &llock);
  }
  unlock();
} // scheduleThread
