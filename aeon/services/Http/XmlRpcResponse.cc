/* 
 * XmlRpcResponse.cc : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, James W. Anderson, Ryan Braud, Charles Killian
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
#include "XmlRpcResponse.h"

const int XmlRpcResponse::BAD_REQUEST;
const int XmlRpcResponse::INVALID_METHOD;
const int XmlRpcResponse::SERVER_ERROR;

std::string XmlRpcResponse::constructFault(int errCode, const std::string& errMsg) {
  char buf[30];

  sprintf(buf, "%d", errCode);
  std::string ret = 
    "<methodResponse>\n"
    "  <fault>\n"
    "    <value>\n"
    "      <struct>\n"
    "        <member>\n"
    "          <name>faultCode</name>\n"
    "          <value><int>";
  ret += buf;
  ret += "</int></value>\n"
    "          </member>\n"
    "        <member>\n"
    "          <name>faultString</name>\n"
    "          <value><string>" + errMsg + "</string></value>\n"
    "          </member>\n"
    "        </struct>\n"
    "      </value>\n"
    "    </fault>\n"
    "  </methodResponse>\n";
  return ret;
} // constructFault

std::string XmlRpcResponse::constructSuccess(const std::string& retVal) {
  std::string r;
  if (retVal.empty()) {
    r =
    "<methodResponse>\n"
    "  </methodResponse>\n";
  }
  else {
    r =
    "<methodResponse>\n"
    "  <params>\n"
    "    <param>\n"
    "      <value>" + retVal + "</value>\n"
    "      </param>\n"
    "    </params>\n"
    "  </methodResponse>\n";
  }
  return r;
} // constructSuccess
  

void XmlRpcResponse::parseFault(std::istream& in, int& errorCode, 
				mace::string& errorString) 
  throw(mace::SerializationException) {
  // Assumes the <fault> tag has already been read, does NOT
  // read the </fault> tag at the end
  mace::SerializationUtil::expectTag(in, "<value>");
  mace::SerializationUtil::expectTag(in, "<struct>");
  mace::SerializationUtil::expectTag(in, "<member>");
  mace::SerializationUtil::expectTag(in, "<name>");
  mace::SerializationUtil::expect(in, "faultCode");
  mace::SerializationUtil::expectTag(in, "</name>");
  mace::SerializationUtil::expectTag(in, "<value>");
  mace::deserializeXML_RPC(in, &errorCode, errorCode);
  mace::SerializationUtil::expectTag(in, "</value>");
  mace::SerializationUtil::expectTag(in, "</member>");
  mace::SerializationUtil::expectTag(in, "<member>");
  mace::SerializationUtil::expectTag(in, "<name>");
  mace::SerializationUtil::expect(in, "faultString");
  mace::SerializationUtil::expectTag(in, "</name>");
  mace::SerializationUtil::expectTag(in, "<value>");
  mace::deserializeXML_RPC(in, &errorString, errorString);
  mace::SerializationUtil::expectTag(in, "</value>");
  mace::SerializationUtil::expectTag(in, "</member>");
  mace::SerializationUtil::expectTag(in, "</struct>");
  mace::SerializationUtil::expectTag(in, "</value>");
} // parseFault
