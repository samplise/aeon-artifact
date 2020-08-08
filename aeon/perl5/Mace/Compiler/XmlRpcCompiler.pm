# 
# XmlRpcCompiler.pm : part of the Mace toolkit for building distributed systems
# 
# Copyright (c) 2011, Charles Killian, James W. Anderson, Adolfo Rodriguez, Dejan Kostic
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in the
#      documentation and/or other materials provided with the distribution.
#    * Neither the names of the contributors, nor their associated universities 
#      or organizations may be used to endorse or promote products derived from
#      this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
# USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# 
# ----END-OF-LEGAL-STUFF----
package Mace::Compiler::XmlRpcCompiler;

use strict;

use Mace::Compiler::ClassParser;
use Mace::Compiler::MInclude;

use constant DEBUG_XML => 0;

my @IGNORE = qw(registerHandler registerUniqueHandler
		unregisterHandler unregisterUniqueHandler);

use Class::MakeMethods::Template::Hash 
    (
     'new --and_then_init' => 'new',
     'object' => ['parser' => { class => "Mace::Compiler::ClassParser" }],
     'object' => ['class' => { class => "Mace::Compiler::ServiceClass" }],
     'hash' => 'returnSet',
     );

sub init {
    my $this = shift;
    my $p = Mace::Compiler::ClassParser->new();
    $this->parser($p);
} # init

sub compileHeader {
    my $this = shift;
    my $in = shift;
    my $filename = shift;
    my $classname = shift || "";
    my @serial = @_;

    my @in = split(/\n/, $in);
    my @linemap;
    my @filemap;
    my @offsetmap;
    Mace::Compiler::MInclude::getLines($filename, \@in, \@linemap, \@filemap, \@offsetmap);
    
    my $sc = $this->parser()->parse($in, $classname, \@linemap, \@filemap, \@offsetmap) || die "error parsing $classname";
    $this->class($sc);
    for my $m ($sc->methods()) {
	$this->returnSet_push($m->returnType()->type(), 1);
    }
#     for my $s (@serial) {
# 	my $c = $this->parser()->parse($in, $s) || die "error parsing $s";
#     }
    
    $this->writeServerHFile($filename);
    $this->writeClientHFile($filename);
    $this->writeClientCFile();
} # compileHeader

sub writeServerHFile {
    my $this = shift;
    my $filename = shift;
    my $cname = $this->class()->name();
    my $capname = uc($cname);
    my $xname = $cname . "XmlRpcHandler";
    my $constructor = "";
    my $protected = "";
    my %seen = ();
    for my $m ($this->class()->methods()) {
	my $n = $m->name();
	if (grep(/^$n$/, @IGNORE)) {
	    next;
	}
	if ($seen{$n}) {
	    die "duplicate method found: $n\n";
	}
	$seen{$n} = 1;
	$constructor .= qq[    methods.insert("$n");
    handlerFuncs["$n"] = &${xname}::${n}Handler;
];
	my $params = "";
	my $dparam = "";
	my $paramstr = "";
	my $i = 0;
	for my $p ($m->params()) {
	    $params .= "    " . $p->type()->type() . " param${i};\n";
	    $dparam .= "      mace::deserializeXML_RPCParam(xmlData, &param${i}, param${i});\n";
	    $paramstr .= "param${i}, ";
	    $i++;
	}
	$paramstr = substr($paramstr, 0, -2);

	my $ret = $m->returnType()->type();
	if ($ret eq "void") {
	    $protected .= qq[
  std::string ${n}Handler(istream& xmlData) {
$params

    try {
      mace::SerializationUtil::expectTag(xmlData, "<params>");
$dparam
      mace::SerializationUtil::expectTag(xmlData, "</params>");
      service->${n}($paramstr);
    } catch (const mace::SerializationException& se) {
      return XmlRpcResponse::constructFault(XmlRpcResponse::BAD_REQUEST, "Syntax error in request:" + se.toString());
    }
    try {
      return XmlRpcResponse::constructSuccess("");
    } catch(const mace::SerializationException& se) {
      return XmlRpcResponse::constructFault(XmlRpcResponse::SERVER_ERROR, "Internal server error serializing response");
    }
  } // ${n}Handler
];
	}
	else {
	    $protected .= qq[
  std::string ${n}Handler(istream& xmlData) {
$params
    std::string retString = "";
    $ret ret;

    try {
      mace::SerializationUtil::expectTag(xmlData, "<params>");
$dparam
      mace::SerializationUtil::expectTag(xmlData, "</params>");
      ret = service->${n}($paramstr);
    } catch (const mace::SerializationException& se) {
      return XmlRpcResponse::constructFault(XmlRpcResponse::BAD_REQUEST, "Syntax error in request:" + se.toString());
    }
    try {
      mace::serializeXML_RPC(retString, &ret, ret);
      return XmlRpcResponse::constructSuccess(retString);
    } catch(const mace::SerializationException& se) {
      return XmlRpcResponse::constructFault(XmlRpcResponse::SERVER_ERROR, "Internal server error serializing response");
    }
  } // ${n}Handler
];
	}
    }


    my $r = qq[#ifndef _${capname}_XML_RPC_HANDLER_H
#define _${capname}_XML_RPC_HANDLER_H

#include <istream>
#include "Serializable.h"
#include "XmlRpcHandler.h"
#include "m_map.h"
#include "$filename"

template<class C>
class $xname : public XmlRpcHandler {
  typedef std::string(${xname}::*ExecFunc)(std::istream& xmlData);

public:
  $xname(C* o) : service(o) {
$constructor
  }

  virtual ~${xname}() { }

  virtual std::string execute(const std::string& methodName, std::istream& xmlData) {
    if(handlerFuncs.containsKey(methodName)) {
      return (this->*handlerFuncs[methodName])(xmlData);
    }
    else {
      return XmlRpcResponse::constructFault(XmlRpcResponse::INVALID_METHOD, "INVALID METHOD");
    }
  }

protected:
$protected

protected:
  mace::map<std::string, ExecFunc, mace::SoftState> handlerFuncs;

private:
  C* service;

};

#endif // _${capname}_XML_RPC_HANDLER_H
];

    $this->writeFile($cname . "XmlRpcHandler.h", $r);
} # writeServerHFile

sub writeFile {
    my $this = shift;
    my ($fname, $text) = @_;
    open(OUT, ">$fname") or die "could not open $fname: $!\n";
    print OUT $text;
    close(OUT);
} # writeFile

sub getTypeMethod {
    my $this = shift;
    my $t = shift;
    my $ft = uc($t);
    $ft =~ s|[^[:alnum:]]|_|g;
    return "get${ft}Type";
} # getTypeMethod

sub writeClientHFile {
    my $this = shift;
    my $filename = shift;
    my $cname = $this->class()->name();
    my $capname = uc($cname);
    my $xname = $cname . "XmlRpcClient";
    my $hname = $cname . "XmlRpcCallbackHandler";
    my $iname = $cname . "CallbackInfo";
    my $cbt = $cname . "Callbacks_t";

    my $htypes = "";
    my $hmethods = "";
    my $hunion = "";
    my $pubmeths = "";
    my $privmeths = "";
    my $cbtypedefs = "";
    my $i = 0;
    for my $m ($this->class()->methods()) {
	my $n = $m->name();
	if (grep(/^$n$/, @IGNORE)) {
	    next;
	}
	my $rt = $m->returnType()->type();
	my $params = "const XmlRpcResponseState<$rt >& ret, void* cbParam";
	if ($rt eq "void") {
	    $params = "const XmlRpcVoidResponseState& ret, void* cbParam";
	}
	$htypes .= "  static const type " . uc($n) . " = $i;\n";
	$hmethods .= "  virtual void " . $cname . ucfirst($n) . "Result($params) { ASSERT(0) };\n";
# 	$cbtypedefs = "  typedef ";
	$hunion .= "  void (${hname}::*${n}Callback)($params);\n";
	my $mp = "";
	if ($m->count_params()) {
	    $mp = join(", ", map { $_->type()->toString() . " " . $_->name() } $m->params());
	}
	my $mpComma = $mp;
	if ($mp) {
	    $mpComma .= ",";
	}
	my $cbptr = "void (${hname}::*rpcCallback)($params)";
# 	$pubmeths .= "  void $n($mpComma ${hname}* obj, $cbptr, void* cbParam = 0) throw(mace::SerializationException, XmlRpcClientException, HttpClientException);\n";
	$pubmeths .= "  void $n($mpComma ${hname}* obj, void* cbParam = 0) throw(mace::SerializationException, XmlRpcClientException, HttpClientException);\n";
	$pubmeths .= "  $rt $n($mp) throw(mace::SerializationException, XmlRpcClientException, HttpClientException);\n";

	$privmeths .= "  std::string makeXmlRpcStr_$n($mp);\n";

	$i++;
    }
    chomp($hunion);
    chomp($pubmeths);

    for my $t ($this->returnSet_keys()) {
	my $ft = $this->getTypeMethod($t);
	if ($t eq "void") {
	    $privmeths .= "  void $ft(std::istream& in, bool& fault, int& errCode, mace::string& errString) throw(mace::SerializationException);\n";
	}
	else {
	    $privmeths .= "  void $ft(std::istream& in, ${t}& val, bool& fault, int& errCode, mace::string& errString) throw(mace::SerializationException);\n";
	}
    }
    chomp($privmeths);

    my $r = qq[#ifndef _${capname}_XML_RPC_CLIENT_H
#define _${capname}_XML_RPC_CLIENT_H

#include <queue>
#include "Serializable.h"
#include "XmlRpcHandler.h"
#include "XmlRpcClient.h"
#include "XmlRpcClientException.h"
#include "mstring.h"
#include "massert.h"
#include "XmlRpcResponse.h"
#include "$filename"

class ${hname} {
public:
  typedef int type;
$htypes

public:
$hmethods
  virtual ~${hname}() { }
}; // class $hname

union ${cbt} {
$hunion
};

class ${iname} {
public:
  $iname(${hname}* h, const ${cbt}& cb, ${hname}::type which, void* param) : handler(h), callback(cb), selector(which), cbParam(param) { }

public:
  ${hname}* handler;
  $cbt callback;
  ${hname}::type selector;
  void* cbParam;
}; // class $iname

class $xname : public XmlRpcClient {
private:
  std::queue<$iname> requests;

public:
  $xname(const std::string& host, int port, const std::string& url, bool async = true) : XmlRpcClient(host, port, url, async) { }
  $xname(const std::string& url, bool async = true) : XmlRpcClient(url, async) { }
  virtual ~$xname() { }
$pubmeths

private:
  void dispatch(const std::string& content, const ${iname}& cb, const HttpClientException* e);
$privmeths
  virtual void postRequestResult(HttpClientResponseState s);
  virtual void getRequestResult(HttpClientResponseState s) { }
}; // class $xname

#endif // _${capname}_XML_RPC_CLIENT_H
];

    $this->writeFile($cname . "XmlRpcClient.h", $r);
} # writeClientHFile

sub writeClientCFile {
    my $this = shift;
    my $cname = $this->class()->name();
    my $capname = uc($cname);
    my $xname = $cname . "XmlRpcClient";
    my $hname = $cname . "XmlRpcCallbackHandler";
    my $iname = $cname . "CallbackInfo";
    my $cbt = $cname . "Callbacks_t";

    my $case = "";
    my $methods = "";
    my $make = "";
    for my $m ($this->class()->methods()) {
	my $n = $m->name();
	if (grep(/^$n$/, @IGNORE)) {
	    next;
	}
	my $cb = $n . "Callback";
	my $rt = $m->returnType()->type();
	my $capn = uc($n);
	my $gtm = $this->getTypeMethod($rt);
	if ($rt eq "void") {
	  $case .= qq[  case ${hname}::$capn: {
    if (e == 0) {
      $gtm(in, fault, errCode, errString);
      if (fault) {
        XmlRpcClientException ce(errString, errCode);
        XmlRpcVoidResponseState rv(&ce);
        (cb.handler->*cb.callback.$cb)(rv, cb.cbParam);
      }
      else {
        XmlRpcVoidResponseState rv(0);
        (cb.handler->*cb.callback.$cb)(rv, cb.cbParam);
      }
    }
    else {
      XmlRpcVoidResponseState rv(e);
      (cb.handler->*cb.callback.$cb)(rv, cb.cbParam);
    }
    return;
  }
];
	}
	else {
	  $case .= qq[  case ${hname}::$capn: {
    $rt ret;
    if (e == 0) {
      $gtm(in, ret, fault, errCode, errString);
      if (fault) {
        XmlRpcClientException ce(errString, errCode);
        XmlRpcResponseState<$rt > rv(0, &ce);
        (cb.handler->*cb.callback.$cb)(rv, cb.cbParam);
      }
      else {
        XmlRpcResponseState<$rt > rv(&ret, 0);
        (cb.handler->*cb.callback.$cb)(rv, cb.cbParam);
      }
    }
    else {
      XmlRpcResponseState<$rt > rv(0, e);
      (cb.handler->*cb.callback.$cb)(rv, cb.cbParam);
    }
    return;
  }
];
	}
	my $params = "";
	my $paramNames = "";
	if ($m->count_params()) {
	    $params = join(", ", map {
		$_->type()->toString() . " " . $_->name()
		} $m->params());
	    $paramNames = join(", ", map { $_->name() } $m->params());
	}
	my $paramsComma = $params;
	if ($params) {
	    $paramsComma .= ",";
	}
	my $rpcCallback = "void (${hname}::*rpcCallback)(const XmlRpcResponseState<$rt >& ret, void* cbParam)";
	if ($rt eq "void") {
	    $rpcCallback = "void (${hname}::*rpcCallback)(const XmlRpcVoidResponseState& ret, void* cbParam)";
	}

	my $rtline = "$rt ret;";
	my $gtmline = "$gtm(in, ret, fault, errCode, errString);";
	my $returnline = "return ret;";
	if ($rt eq "void") {
	    $rtline = "";
	    $gtmline = "$gtm(in, fault, errCode, errString);";
	    $returnline = "";
	}

	my $cbname = '&' . "$hname" . "::" . $cname . ucfirst($n) . "Result";

        my $add_selectors = "";
        my $xmlrequest = "";
        my $rcontent = "";
        if (DEBUG_XML) {
          $add_selectors = "ADD_FUNC_SELECTORS";
          $xmlrequest = qq{macedbg(1) << "xmlRequest[ " << xmlRequest << " ]" << Log::endl;};
          $rcontent = qq{macedbg(1) << "r.content[ " << r.content << " ]" << Log::endl;};
        }

	$methods .= qq[
//void ${xname}::$n($paramsComma ${hname}* obj, $rpcCallback, void* cbParam) throw(mace::SerializationException, XmlRpcClientException, HttpClientException) {
void ${xname}::$n($paramsComma ${hname}* obj, void* cbParam) throw(mace::SerializationException, XmlRpcClientException, HttpClientException) {
  $add_selectors
  $cbt cb;
  std::string xmlRequest = makeXmlRpcStr_$n($paramNames);
  $xmlrequest

  cb.$cb = $cbname;
  requests.push($iname(obj, cb, ${hname}::$capn, cbParam));
  postUrlAsync(url, "text/xml", xmlRequest, "1.1", true, StringHMap(), this, &XmlRpcClient::postRequestResult);
} // $n

$rt ${xname}::$n($params) throw(mace::SerializationException, XmlRpcClientException, HttpClientException) {
  $add_selectors
  $rtline
  HttpResponse r;
  bool fault = false;
  int errCode = -1;
  mace::string errString = "";
  std::istringstream in;
  std::string xmlRequest = makeXmlRpcStr_$n($paramNames);
  $xmlrequest

  r = postUrl(url, "text/xml", xmlRequest);
  $rcontent
  in.str(r.content);
  $gtmline
  if(fault) {
    throw(XmlRpcClientException(errString, errCode));
  }
  $returnline
} // $n
];

	my $serparams = "";
	for my $p ($m->params()) {
	    my $pn = $p->name();
	    $serparams .= qq[  xmlRequest += "    <param>\\n      <value>";
  mace::serializeXML_RPC(xmlRequest, &${pn}, $pn);
  xmlRequest += "</value>\\n      </param>\\n";
];
	}
	chomp($serparams);

	$make .= qq[std::string ${xname}::makeXmlRpcStr_$n($params) {
  std::string xmlRequest = "<?xml version=\\"1.0\\"?>\\n<methodCall>\\n  <methodName>";
  xmlRequest += "$cname.$n</methodName>\\n";
  xmlRequest += "  <params>\\n";
$serparams
  xmlRequest += "    </params>\\n  </methodCall>";
  return xmlRequest;
} // makeXmlRpcStr_$n

];

    }

    my $gettypes = "";
    for my $t ($this->returnSet_keys()) {
	my $fn = $this->getTypeMethod($t);
	if ($t eq "void") {
	    $gettypes .= qq[void ${xname}::$fn(std::istream& in, bool& fault, int& errCode, mace::string& errString) throw(mace::SerializationException) {
  // ignore the <?xml version=*?> tag (fix later?)
  mace::SerializationUtil::getTag(in, false);
  mace::SerializationUtil::expectTag(in, "<methodResponse>");
  std::string tag = mace::SerializationUtil::getTag(in);
  if(tag == "<fault>") {
    fault = true;
    XmlRpcResponse::parseFault(in, errCode, errString);
    mace::SerializationUtil::expectTag(in, "</fault>");
  }
  else if (tag != "</methodResponse>") {
    throw(mace::SerializationException("Invalid tag in XML response:" + tag));
  }
}

];
	}
	else {
	    $gettypes .= qq[void ${xname}::$fn(std::istream& in, $t& val, bool& fault, int& errCode, mace::string& errString) throw(mace::SerializationException) {
  // ignore the <?xml version=*?> tag (fix later?)
  mace::SerializationUtil::getTag(in, false);
  mace::SerializationUtil::expectTag(in, "<methodResponse>");
  std::string tag = mace::SerializationUtil::getTag(in);
  if(tag == "<params>") {
    fault = false;
    mace::SerializationUtil::expectTag(in, "<param>");
    mace::SerializationUtil::expectTag(in, "<value>");
    mace::deserializeXML_RPC(in, &val, val);
    mace::SerializationUtil::expectTag(in, "</value>");
    mace::SerializationUtil::expectTag(in, "</param>");
    mace::SerializationUtil::expectTag(in, "</params>");
  }
  else if(tag == "<fault>") {
    fault = true;
    XmlRpcResponse::parseFault(in, errCode, errString);
    mace::SerializationUtil::expectTag(in, "</fault>");
  }
  else {
    throw(mace::SerializationException("Invalid tag in XML response" + tag));
  }
  mace::SerializationUtil::expectTag(in, "</methodResponse>");
}

];
	}
    }

    my $logging = "";
    if (DEBUG_XML) {
      $logging = qq{#include "Log.h"\n#include "mace-macros.h"};
    }
    my $r = qq[#include <string>
#include <sstream>
#include "${xname}.h"
$logging

void ${xname}::dispatch(const std::string& content, const $iname& cb, const HttpClientException* e) {
  istringstream in(content);
  bool fault = false;
  int errCode = -1;
  mace::string errString = "";

  switch (cb.selector) {
$case
  } // switch (cb.selector)
} // dispatch

void ${xname}::postRequestResult(HttpClientResponseState s) {
  if (requests.empty()) {
    // We did not make an XmlRpc call?
    return;
  }

  ${iname}& front = requests.front();
  try {
    HttpResponse& response = s.getResponse();
    dispatch(response.content, front, 0);
  } catch(const HttpClientException& e) {
    dispatch("", front, &e);
  }
  requests.pop();
} // postRequestResult

$methods
$make
$gettypes
];

    $this->writeFile($cname . "XmlRpcClient.cc", $r);
} # writeClientCFile



1;
