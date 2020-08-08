/* 
 * logreader.h : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, Charles Killian, Darren Dao, James W. Anderson, Ryan Braud, Alex Rasmussen
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
#ifndef H_LOGREADER
#define H_LOGREADER

#include <iostream>
#include <fstream>
#include "m_map.h"
#include "DummyServiceMapper.h"

// Parse modes define the different ways in which the log reader can parse binary 
// log data. They must be declared as numbers > 0, although continuity in the 
// numbering of parse modes isn't necessary.

#define PARSE_MODE_NORMAL 0
#define PARSE_MODE_SQL 1

using mace::Serializable;
using mace::BinaryLogObject;
using mace::DummyServiceMapper;
using mace::SerializationException;

typedef mace::map<log_id_t, std::string> SelectorMap;

typedef struct log_info {
  double timestamp;
  unsigned int tid;
  std::string log_type;
  std::string selector;
} LogInfo;

void printUsage();
std::string sqlSanitize(std::string &s);

int outputSHeader(std::ostream &outs);
int outputDumpHeader(std::ostream &outs);
int outputSLog(std::istream &ifs, std::ostream &outStream, LogInfo &info);
int outputDumpLog(std::istream &ifs, std::ostream &outStream, LogInfo &info, 
  BinaryLogObject *obj);

int outputBinaryLog(std::istream &ifs, 
                    std::ostream &sOutputStream, 
                    std::ostream &dumpOutputStream, 
                    DummyServiceMapper &dummyServiceMapper, 
                    SelectorMap &selectors);


#endif
