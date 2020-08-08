/* 
 * pip_includer.h : part of the Mace toolkit for building distributed systems
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
#ifndef _PIP_H
#define _PIP_H
// #define PIP_MESSAGING

#include <string>

/** 
 * \file pip_includer.h
 * \brief file to include pip functions, or make them be empty if not using pip
 */

#ifdef PIP_MESSAGING
#include "annotate.h"
static const bool PIP = true;
inline char* annotate_get_path_id_copy(int* len) { 
  char* pid = (char *)ANNOTATE_GET_PATH_ID(len);
  if (pid != NULL) {
    char* ret = (char *)malloc(*len);
    memcpy(ret, pid, *len);
    return ret;
  }
  return NULL;
}

#else
static const bool PIP = false;
inline void ANNOTATE_INIT(void) {} ///< initialize Pip
inline void ANNOTATE_START_TASK(const char *roles, int level, const char *name) {} ///< begin a Pip task with the given \c name
inline void ANNOTATE_END_TASK(const char *roles, int level, const char *name) {} ///< end a Pip task with the given \c name
inline void ANNOTATE_SET_PATH_ID(const char *roles, int level, const void *path_id, int idsz) {} ///< set the path id
inline const void *ANNOTATE_GET_PATH_ID(int *len) { return 0; } ///< get the path id
inline void ANNOTATE_PUSH_PATH_ID(const char *roles, int level, const void *path_id, int idsz) {} ///< set a new path id, storing the old for pop
inline void ANNOTATE_POP_PATH_ID(const char *roles, int level) {} ///< restore old path id
inline void ANNOTATE_END_PATH_ID(const char *roles, int level, const void *path_id, int idsz) {} ///< mark path as complete (on this node at least)
inline void ANNOTATE_NOTICE(const char *roles, int level, const char *fmt, ...) {} ///< log message to Pip
inline void ANNOTATE_SEND(const char *roles, int level, const void *msgid, int idsz, int size) {} ///< annotate a message sent with given id and size
inline void ANNOTATE_RECEIVE(const char *roles, int level, const void *msgid, int idsz, int size) {} ///< annotate a message received with given id and size

inline void ANNOTATE_SET_PATH_ID_INT(const char* roles, int level, int n)  {} ///< convenience method to set a path from an int
inline void ANNOTATE_END_PATH_ID_INT(const char* roles, int level, int n) {} ///< convenience method to end a path from an int
inline void ANNOTATE_PUSH_PATH_ID_INT(const char* roles, int level, int n) {} ///< convenience method to puth an int path
inline void ANNOTATE_SEND_INT(const char* roles, int level, int n, int size) {} ///< convenience method to mark a message as send with an int id
inline void ANNOTATE_RECEIVE_INT(const char* roles, int level, int n, int size) {} ///< convenience method to mark a message as received with an int id
inline void ANNOTATE_SET_PATH_ID_STR(const char *roles, int level, const char *fmt, ...) {} ///< convenience method to set a path from a format string
inline void ANNOTATE_END_PATH_ID_STR(const char *roles, int level, const char *fmt, ...) {} ///< convenience method to end a path from a format string
inline void ANNOTATE_PUSH_PATH_ID_STR(const char *roles, int level, const char *fmt, ...) {} ///< convenience method to push a path from a format string
inline void ANNOTATE_SEND_STR(const char *roles, int level, int size, const char *fmt, ...) {} ///< convenience method to mark a message as sent from a format string id
inline void ANNOTATE_RECEIVE_STR(const char *roles, int level, int size, const char *fmt, ...) {} ///< convenience method to mark a message as received from a format string id

inline void ANNOTATE_BELIEF_FIRST(int seq, float max_fail_rate, const char *condstr, const char *file, int line) {}  ///< UNKNOWN
inline void REAL_ANNOTATE_BELIEF(const char *roles, int level, int seq, int condition) {} ///< UNKNOWN
inline void ANNOTATE_BELIEF(const char* roles, int level, bool cond, double rate) {} ///< annotate a believe with failure rate (like an assertion which can occasionally be false)
inline char* annotate_get_path_id_copy(int* len) { return NULL; }
#endif // PIP_MESSAGING

inline std::string annotate_get_path() {
  static const std::string empty;
  int length;
  const char* path = (const char*)ANNOTATE_GET_PATH_ID(&length);
  return path == NULL ? empty : std::string(path, length);
}

#endif // _PIP_H
