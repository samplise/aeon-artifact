/* 
 * logreader.cc : part of the Mace toolkit for building distributed systems
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
#include <iostream>
#include "logreader.h"
#include "MaceTypes.h"
#include "boost/format.hpp"
#include "BinaryLogObject.h"

// Set the parse mode to an unrecognizable default so we know 
// when we forget to set it.
int PARSE_MODE = -1;
using mace::BinaryLogFactory;

/**
 * \brief This application can be used for reading binary log that were produced by Mace binary logging mechanism
 *
 * Usage: logreader path_to_binary_log [sql]
 *
 * Important: In order to use this application, you need to link it with the appropriate Mace
 * services that generated the logs. See Makefile and/or CMakeLists.txt for example
 */
std::ofstream* getLogFile(const std::string& fname, 
			  std::map<std::string, std::ofstream*>& fMap) {
  std::ofstream* out = fMap[fname];
  if (out == NULL) {
    out = new std::ofstream(fname.c_str());
    fMap[fname] = out;
  }
  return out;
}

int main(int argc, char *argv[])
{
  std::ifstream ifs(argv[1] , std::ifstream::in);

  DummyServiceMapper dummyServiceMapper;
  SelectorMap selectors;
  int byteRead = 0;

  if(argc == 2) {
    PARSE_MODE = PARSE_MODE_NORMAL;
    byteRead = outputBinaryLog(ifs, std::cout, std::cout, dummyServiceMapper, selectors);
  }
  else if(argc == 3) {
    std::string parseMode(argv[2]);
    if(parseMode == "sql") {
      PARSE_MODE = PARSE_MODE_SQL;
    } else {
      printUsage();
    }

    std::cout << "Starting dump ... ";

    std::ofstream sOut("log_S");
    std::ofstream dumpOut("log_Dump");

    byteRead = outputBinaryLog(ifs, sOut, dumpOut, dummyServiceMapper, selectors);
    sOut.close();
    dumpOut.close();

    std::cout << "done." << std::endl;
  } else {
    printUsage();
  }

  ifs.close();
  std::cout << "Total bytes read: " << byteRead << std::endl;
  return 0;
}

void printUsage() 
{
    std::cerr << "Usage: logreader path_to_binary_log [sql]" << std::endl;
    exit(1);
}

int outputSHeader(std::ostream &outs)
{
  if(PARSE_MODE == PARSE_MODE_SQL)
  {
    outs << "CREATE TABLE S(";
    outs << "time float8 NOT NULL,";
    outs << "tid INTEGER,";
    outs << "log_type TEXT,";
    outs << "selectors TEXT,";
    outs << "message TEXT,";
    outs << "PRIMARY KEY(tid,time));" << std::endl;
    
    outs << "COPY S FROM stdin;" << std::endl;
  }
  
  return 0;
}

int outputDumpHeader(std::ostream &outs)
{
  if(PARSE_MODE == PARSE_MODE_SQL)
  {
    outs << "CREATE TABLE Dump(";
    outs << "time float8 NOT NULL,";
    outs << "tid INTEGER,";
    outs << "log_type TEXT,";
    outs << "selectors TEXT,";
    outs << "log_description TEXT,";
    outs << "serialized BYTEA,";
    outs << "PRIMARY KEY(tid,time));" << std::endl;
    
    outs << "COPY Dump FROM stdin;" << std::endl;
  }

  return 0;
}

int outputSLog(std::istream &ifs, std::ostream &outStream, LogInfo &info)
{
  int bytesRead = 0;

  std::string message;
  bytesRead += mace::deserialize(ifs, &message);

//   switch(PARSE_MODE) {
//   case PARSE_MODE_NORMAL:
//     outStream << boost::format("%.6f") % info.timestamp;
//     outStream << " " << boost::format("%02d") % info.tid;
//     outStream << " [" << info.selector << "] ";
//     outStream << message << std::endl;
//     break;
//   case PARSE_MODE_SQL:
//     std::string log_type = sqlSanitize(info.log_type);
//     std::string selectors = sqlSanitize(info.selector);
//     message = sqlSanitize(message);

//     outStream << boost::format("%.6f") % info.timestamp << "\t";
//     outStream << boost::format("%02d") % info.tid << "\t";
//     outStream << log_type << "\t";
//     outStream << selectors << "\t";
//     outStream << message << std::endl;
//     break;
//   }
  return bytesRead;
}

int outputDumpLog(std::istream &ifs, std::ostream &outStream, LogInfo &info, 
                  BinaryLogObject *obj)
{
  int bytesRead = 0;
  
  if(PARSE_MODE == PARSE_MODE_NORMAL) {
    outStream << boost::format("%.6f") % info.timestamp;
    outStream << " " << boost::format("%02d") % info.tid;
    outStream << " [" << info.selector << "] ";
    outStream << "(" << obj->getLogDescription() << ") ";
    obj->print(outStream);
    outStream << std::endl;
  } else if(PARSE_MODE == PARSE_MODE_SQL) {
    std::string logDescription = obj->getLogDescription();
    std::string objSerialized = obj->serializeStr();
    
    std::string log_type = sqlSanitize(info.log_type);
    std::string selector = sqlSanitize(info.selector);
    logDescription = sqlSanitize(logDescription);
    
    std::string delimiter = "\t";
        
    outStream << boost::format("%.6f") % info.timestamp << delimiter;
    outStream << boost::format("%02d") % info.tid << delimiter;
    outStream << log_type << delimiter;
    outStream << selector << delimiter;
    outStream << logDescription << delimiter;
    outStream << objSerialized;
    
    outStream << std::endl;
  }
  
  return bytesRead;
}

std::string sqlSanitize(std::string &s)
{
  std::string result(s);

  size_t j;

  while((j = result.find("'"))!= std::string::npos) {
    result.replace(j, 1, "\"");
  }

  result.insert(0, "'");
  result.append("'");

  while((j = result.find("\n")) != std::string::npos) {
    result.replace(j, 1, "\\n");
  }

  while((j = result.find("\t")) != std::string::npos) {
    result.replace(j, 1, "\\t");
  }

  return result;
}

/**
 * \brief Parses the binary output presented by the input stream, possibly
 * outputting that parsed data to the output stream.
 **/

int outputBinaryLog(std::istream &ifs, std::ostream &sOutputStream, 
                    std::ostream &dumpOutputStream, DummyServiceMapper &dummyServiceMapper, 
                    SelectorMap &selectors)
{
  int bytesRead = 0;
  log_id_t id;
  std::string hdr;
  std::map<std::string, std::ofstream*> fileMap;
  
//   outputSHeader(sOutputStream);
//   outputDumpHeader(dumpOutputStream);
  
  std::cout << "CREATE TABLE events (time DOUBLE PRECISION PRIMARY KEY, _id int,"
	    << " tname TEXT);" << std::endl;
  while(ifs.peek() != EOF) {
    bytesRead += mace::deserialize(ifs, &id); 

    LogInfo logInfo;

    mace::deserialize(ifs, &(logInfo.timestamp));
    mace::deserialize(ifs, &(logInfo.tid));
    mace::deserialize(ifs, &(logInfo.log_type));
    
    logInfo.selector = selectors[id];
    if(logInfo.log_type == "S") {
      outputSLog(ifs, sOutputStream, logInfo);
    }
    else if(logInfo.log_type == "M") {
      std::string message;
      mace::deserialize(ifs, &message);
      selectors[id] = message;
    }
    else {
      uint32_t size = 0;
      mace::deserialize(ifs, &size);
//       std::streamoff pos = ifs.tellg();
      BinaryLogFactory fac = dummyServiceMapper.getFactory(logInfo.log_type);
      if (fac == NULL) {
        std::cout << "*** WARNING: Can't deserialize object " << logInfo.log_type << ". Skipping..." << std::endl;
        ifs.seekg(size, std::ios::cur);
      }
      else {
	//XXX: this code does not work anymore
// 	BinaryLogObject* obj = (*fac)();
// 	int b = obj->deserialize(ifs);
// 	std::string s;
//  	obj->sqlize(obj->getChildRoot());
// 	if (s != "") {
// 	  bytesRead += b;
// // 	  std::ofstream* out = getLogFile("events", fileMap);
// // 	  (*out) << boost::format("%.6f") % logInfo.timestamp << "\t"
// // 		 << mace::getIndex(obj->getLogType()) << "\t"
// // 		 << obj->getLogType() << std::endl;
// 	  std::cout << "INSERT INTO events (time, _id, tname) VALUES (" 
// 		    << boost::format("%.6f") % logInfo.timestamp << ", " 
// 		    << (mace::getIndex(obj->getLogType()) - 1)<< ", '" 
// 		    << obj->getLogType() << "');" << std::endl;
// 	}
//         ASSERT((uint32_t)(ifs.tellg() - pos) == size);
//         std::cout << s;
// 	delete obj;
// // 	bytesRead += outputDumpLog(ifs, dumpOutputStream, logInfo, obj);
      }
    }
  }
  
  return bytesRead;
}
