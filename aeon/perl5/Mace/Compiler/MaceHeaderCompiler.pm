# 
# MaceHeaderCompiler.pm : part of the Mace toolkit for building distributed systems
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
package Mace::Compiler::MaceHeaderCompiler;

use strict;

use Mace::Compiler::MaceHeaderParser;
use Mace::Compiler::Param;
use Mace::Compiler::Type;
use Mace::Util qw(:all);
use Mace::Compiler::GeneratedOn;

use Mace::Compiler::ParseTreeObject::BraceBlock;
use Mace::Compiler::ParseTreeObject::Expression;
use Mace::Compiler::ParseTreeObject::MethodTerm;
use Mace::Compiler::ParseTreeObject::ParsedAbort;
use Mace::Compiler::ParseTreeObject::ParsedAssertMsg;
use Mace::Compiler::ParseTreeObject::ParsedAssert;
use Mace::Compiler::ParseTreeObject::ParsedBinaryAssignOp;
use Mace::Compiler::ParseTreeObject::ParsedCatches;
use Mace::Compiler::ParseTreeObject::ParsedCatch;
use Mace::Compiler::ParseTreeObject::ParsedDefaultCase;
use Mace::Compiler::ParseTreeObject::ParsedDoWhile;
use Mace::Compiler::ParseTreeObject::ParsedElseIf;
use Mace::Compiler::ParseTreeObject::ParsedElseIfs;
use Mace::Compiler::ParseTreeObject::ParsedElse;
use Mace::Compiler::ParseTreeObject::ParsedExpectStatement;
use Mace::Compiler::ParseTreeObject::ParsedExpression;
use Mace::Compiler::ParseTreeObject::ParsedFCall;
use Mace::Compiler::ParseTreeObject::ParsedForLoop;
use Mace::Compiler::ParseTreeObject::ParsedForUpdate;
use Mace::Compiler::ParseTreeObject::ParsedForVar;
use Mace::Compiler::ParseTreeObject::ParsedIf;
use Mace::Compiler::ParseTreeObject::ParsedLogging;
use Mace::Compiler::ParseTreeObject::ParsedLValue;
use Mace::Compiler::ParseTreeObject::ParsedOutput;
use Mace::Compiler::ParseTreeObject::ParsedPlusPlus;
use Mace::Compiler::ParseTreeObject::ParsedCaseOrDefault;
use Mace::Compiler::ParseTreeObject::ParsedReturn;
use Mace::Compiler::ParseTreeObject::ParsedSwitchCase;
use Mace::Compiler::ParseTreeObject::ParsedSwitchCases;
use Mace::Compiler::ParseTreeObject::ParsedSwitchConstant;
use Mace::Compiler::ParseTreeObject::ParsedSwitch;
use Mace::Compiler::ParseTreeObject::ParsedMacro;
use Mace::Compiler::ParseTreeObject::ParsedTryCatch;
use Mace::Compiler::ParseTreeObject::ParsedVar;
use Mace::Compiler::ParseTreeObject::ParsedWhile;
use Mace::Compiler::ParseTreeObject::StatementBlock;
use Mace::Compiler::ParseTreeObject::StatementOrBraceBlock;
use Mace::Compiler::ParseTreeObject::SemiStatement;
use Mace::Compiler::ParseTreeObject::ScopedId;
use Mace::Compiler::ParseTreeObject::ArrayIndex;
use Mace::Compiler::ParseTreeObject::ArrayIndOrFunction;
use Mace::Compiler::ParseTreeObject::ArrayIndOrFunctionParts;
use Mace::Compiler::ParseTreeObject::Expression1;
use Mace::Compiler::ParseTreeObject::Expression2;
use Mace::Compiler::ParseTreeObject::ExpressionLValue;
use Mace::Compiler::ParseTreeObject::ExpressionLValue1;
use Mace::Compiler::ParseTreeObject::ExpressionLValue2;
use Mace::Compiler::ParseTreeObject::ExpressionOrAssignLValue;
use Mace::Compiler::ParseTreeObject::ExpressionOrAssignLValue1;
use Mace::Compiler::ParseTreeObject::StateExpression;



use constant SYNC_NAME => "syncname";
use constant SYNC_ID_FIELDS => "id";
use constant SYNC_TYPE => "type";
use constant SYNC_CALLBACK => "callback";
use constant SYNC_RETURNTYPE => "returntype";
use constant BLOCK_TYPE => "block";
use constant BROADCAST_TYPE => "broadcast";

use Class::MakeMethods::Template::Hash 
    (
     'new --and_then_init' => 'new',
     'object' => ['parser' => { class => "Mace::Compiler::MaceHeaderParser" }],
     );

sub init {
    my $this = shift;
    my $p = Mace::Compiler::MaceHeaderParser->new();
    $this->parser($p);
} # init

sub compileHeader {
    my $this = shift;
    my $filename = shift;
    my $text = shift;
    my $linemap = shift or die("no linemap!");
    my $filemap = shift or die("no filemap!");
    my $offsetmap = shift or die("no offsetmap!");

    # for my $l (@$linemap) {
    #     print "line: $l\n";
    # }

    my $sc = $this->parser()->parse($text, $linemap, $filemap, $offsetmap);

    my $service = $sc->className();
    my $target = $sc->className() . ".h";
    my $r = Mace::Compiler::GeneratedOn::generatedOnHeader($target);

    my $hname = "_" . uc($filename) . "_H";
    $r .= ("#ifndef $hname\n" .
	   "#define $hname\n");

    unless ($sc->isServiceClass()) {
	$r .= ("#include \"massert.h\"\n" . '#include' . qq{ "MaceTypes.h"\n});
    }

    my @allh = ();
    for my $el (@{$sc->superclasses()}) {
	push(@allh, $this->parseHandlers($el));
    }

    if ($sc->isServiceClass()) {
	unless (grep/ServiceClass/, @{$sc->superclasses()}) {
	    $sc->push_superclasses("");
	}
    }

    my @c = $sc->superclasses();
    if (scalar(@c)) {
	@c = map { $_ .= "ServiceClass" } @c;
	for my $el (@c) {
	    $r .= '#include' . qq{ "${el}.h"\n};
	}
    }

    for my $el (@{$sc->handlers()}) {
	$r .= '#include' . qq{ "${el}Handler.h"\n};
    }

    if ($sc->count_auto_types()) {
	$r .= '#include' . qq{ "mace-macros.h"\n};
	$r .= '#include' . qq{ "Serializable.h"\n};
    }

    $r .= "\n";

    $r .= $sc->pre();

    if ($sc->isNamespace()) {
	$r .= "\nnamespace " . $sc->className() . " {\n";
    }
    elsif (!$sc->isAutoType()) {
      my $superClasses = "";
      @c = $sc->superclasses();
      if (scalar(@c)) {
          @c = map { $_ .= "ServiceClass" } @c;
          unshift(@c, "");
          $superClasses = join(", public virtual ", @c);
          $superClasses = ": " . substr($superClasses, 2);
      }

#     $r .= "\nclass " . $sc->className() . " $superClasses {\n";
      $r .= "\nclass " . $sc->className() . " $superClasses {\n";

      $r .= qq{public:
  static ${service}& NULL_;
  static const char* name; // = "${\$sc->name()}";
};

      $r .= "\n";
    }

    for my $at ($sc->auto_types()) {
	$r .= "\nclass " . $at->name() . ";\n";
    }

    $r .= join("\n", @{$sc->literals()}) . "\n\n";

    for my $at ($sc->auto_types()) {
	$at->validateAutoTypeOptions();
	$r .= "\n" . $at->toAutoTypeString(body=>1) . "\n";
    }

    my @syncMethods = ();

    for my $meth (@{$sc->methods()}) {
        $r .= "  #line ".$meth->line()." \"".$meth->filename()."\"\n";
	$r .= "  " . $meth->toString("rid" => 1);
	if ($meth->body()) {
	    $r .= $meth->body();
	}
	else {
	    $r .= qq/ { ABORT("Unimplemented method ${\$meth->name()} in base class ${service} called."); }/;
	}
	$r .= "\n";
        $r .= "  // __INSERT_LINE_HERE__\n";

	if ($meth->options(SYNC_NAME)) {
	    push(@syncMethods, $meth);
	}
    }

    if ($sc->isServiceClass()) {
	push(@allh, @{$sc->handlers()});
	@allh = map { $_ . "Handler" } @allh;
	my %allhf = ();
	for my $el (@allh) {
	    $allhf{$el} = 1;
	}
	@allh = keys(%allhf);

	for my $el (@allh) {
	    $r .= qq#  virtual registration_uid_t registerHandler($el& handler, registration_uid_t rid = -1, bool isAppHandler = true) { ABORT("registerHandler method not implemented"); }
		     virtual void unregisterHandler($el& handler, registration_uid_t rid = -1) { ABORT("unregisterHandler method not implemented"); }
		     void unregisterUniqueHandler($el& handler) { unregisterHandler(handler); }
		     virtual void registerUniqueHandler($el& handler, bool isAppHandler = true) { ABORT("registerUniqueHandler method not implemented"); }
       #;
	}
    }


    if ($sc->isNamespace()) {
      $r .= "\n};\n";
    }
    elsif (!$sc->isAutoType()) {
      $r .= "  virtual ~$service() { }\n";

      $r .= $sc->maceLiteral();

      $r .= "};\n";
    }

    $r .= $sc->post();

    if (scalar(@syncMethods)) {
	$r .= $this->generateSyncMethods($sc, @syncMethods);
    }

    $r .= "\n#endif // $hname\n";

    return $r;
} # compileHeader

sub generateResultStructs {
    my $this = shift;
    my ($asyncMethods, $callbacks) = @_;
    my $r = "";
    for my $m (@$asyncMethods) {
	my $cbname = $m->options(SYNC_CALLBACK);
	my $cb = $callbacks->{$cbname};
	if ($cb->count_params() == 0) {
	    $m->options(SYNC_RETURNTYPE, "void");
	    next;
	}
	my $name = ucfirst($m->options(SYNC_NAME)) . "Result";
	$m->options(SYNC_RETURNTYPE, $name);
	my $s = "class $name : public mace::Serializable {\n";
	$s .= "public:\n";
	$s .= "  $name() { }\n";
	$s .= "  $name(";
	my $paramStr = join(", ", map { $_->toString() } $cb->params());
	$s .= $paramStr;
	$s .= ") : ";
	$s .= join(", ", map { $_->name() . "(" . $_->name() . ")" }  $cb->params());
	$s .= " { }\n";
	my $xs = "";
	my $nparams = $cb->count_params();
	my $xd = qq(    for (size_t _i = 0; _i < $nparams; _i++) {
      mace::SerializationUtil::expect(in, "<member>");
      in >> skipws;
      mace::SerializationUtil::expect(in, "<name>");
      std::string _k = mace::SerializationUtil::get(in, '<');
      mace::SerializationUtil::expect(in, "</name>");
      in >> skipws;
      mace::SerializationUtil::expect(in, "<value>");
);
        my $ifv = "if";
        for my $param ($cb->params()) {
	    my $n = $param->name();
	    $xd .= qq(      $ifv (_k == "$n") {
        mace::deserializeXML_RPC(in, &$n, $n);
      }
);
	    if ($ifv eq "if") {
		$ifv = "else if";
	    }
	}

	$xd .= q(      else {
        throw mace::SerializationException("unknown member: " + _k);
      }
      mace::SerializationUtil::expect(in, "</value>");
      in >> skipws;
      mace::SerializationUtil::expect(in, "</member>");
      in >> skipws;
    }
);

	for my $param ($cb->params()) {
	    my $n = $param->name();
	    my $t = $param->type()->type();
	    $s .= "  $t $n;\n";
	    $xs .= qq{    s.append("<member><name>$n</name><value>");
    mace::serializeXML_RPC(s, &$n, $n);
    s.append("</value></member>");
};

	}

	$s .= qq{
  void serialize(std::string& str) const { ABORT("serialize method not implemented"); }
  int deserialize(std::istream& in) throw(mace::SerializationException) { ABORT("deserialize method not implemented"); }
  void serializeXML_RPC(std::string& s) const throw(mace::SerializationException) {
    s.append("<struct>");
$xs
    s.append("</struct>");
  }
  int deserializeXML_RPC(std::istream& in) throw(mace::SerializationException) {
    std::istream::pos_type __offset = in.tellg();
    in >> skipws;
    mace::SerializationUtil::expect(in, "<struct>");
    in >> skipws;
$xd
    mace::SerializationUtil::expect(in, "</struct>");
    return in.tellg() - __offset;
  }

};

	$s .= "};\n\n";

	$r .= $s;
    }

    return $r;
} # generateResultStructs

sub generateSyncMethods {
    my $this = shift;
    my $sc = shift;
    my @asyncMethods = @_;
    my $r = "";

    my @handlers = ();
    for my $el (@{$sc->handlers()}) {
	my $f = $el . "Handler.mh";
        my $path = findPath($f, @Mace::Compiler::Globals::INCLUDE_PATH);
	my @text = readFile("$path/$f");

        my @linemap;
        my @filemap;
        my @offsetmap;

        Mace::Compiler::MInclude::getLines("$path/$f", \@text, \@linemap, \@filemap, \@offsetmap);

        my $text = join("", @text);

	my $h = $this->parser()->parse($text, \@linemap, \@filemap, \@offsetmap);
	push(@handlers, $h);
    }

    my @handlerNames = ();
    my @methodNames = ();
    my %callbacks = ();
    my %handlers = ();
    my %asyncMethods = ();
    for my $m (@asyncMethods) {
	push(@methodNames, $m->name());
	my $cb = $m->options(SYNC_CALLBACK);
	for my $h (@handlers) {
	    my $found = 0;
	    for my $meth (@{$h->methods()}) {
		if ($meth->name() eq $cb) {
		    $asyncMethods{$cb} = $m;
		    $callbacks{$cb} = $meth;
		    $handlers{$cb} = $h;
                    my $handlerClassName = $h->className();
		    if (!grep($_ eq $handlerClassName, @handlerNames)) {
			push(@handlerNames, $h->className());
		    }
		    $found = 1;
		    last;
		}
	    }
	    if (!$found) {
		die ("undefined callback $cb referenced in " . $m->name() . "\n");
	    }
	}
    }
    my @callbackNames = keys(%callbacks);

    $r .= q{
#include "NumberGen.h"
#include "ScopedLock.h"
#include "ScopedLog.h"
#include "mace.h"
#include "Serializable.h"
#include "XmlRpcCollection.h"

};

    my $className = "Synchronous" . $sc->name();
    $r .= "class $className : ";
    $r .= join(", ", map { "public ${_}" } @handlerNames);
    $r .= " {\n";


    $r .= "\npublic:\n";
    $r .= $this->generateResultStructs(\@asyncMethods, \%callbacks);

    $r .= "\nprivate:\n";
    $r .= "  " . $sc->className() . "& _sc;\n";
    for my $h (@handlerNames) {
	$r .= "  $h& " . lcfirst($h) . ";\n";
    }
    $r .= "  registration_uid_t rid;\n";

    my $constructorInit = "";
    
    while (my ($cbname, $m) = each(%asyncMethods)) {
	my $idType = "";
	for my $p ($m->params()) {
	    if ($p->name() eq $m->options(SYNC_ID_FIELDS)) {
		$idType = $p->type()->type();
		last;
	    }
	}
	my $name = $m->name();
	my $returntype = $m->options(SYNC_RETURNTYPE);
	if ($returntype eq "void") {
	    $r .= qq{
  uint64_t ${name}Flag;
};
	    $constructorInit .= ", ${name}Flag(0)";
	}
	else {
	    $r .= qq{
  typedef mace::map<$idType, boost::shared_ptr<$returntype>, mace::SoftState> ${returntype}Map;
  typedef mace::map<$idType, uint64_t, mace::SoftState> ${returntype}AsyncMap;
  ${returntype}Map ${name}Results;
  ${returntype}AsyncMap ${name}Flags;
};
	}
	$r .= "  pthread_cond_t ${name}Signal;\n";
    }


    $r .= "\npublic:\n";

    $r .= "  $className(" . $sc->className() . "& sc, ";
    $r .= join(", ", map { $_ . "& " . lcfirst($_) } @handlerNames);
    $r .= ") : _sc(sc), ";
    $r .= join(", ", map { lcfirst($_) . "(" . lcfirst($_) . ")" } @handlerNames);
    $r .= $constructorInit;
    $r .= " {\n";
    $r .= "    rid = NumberGen::Instance(NumberGen::HANDLER_UID)->GetVal();\n";
    for my $h (@handlerNames) {
	$r .= "    _sc.registerHandler(($h&)*this, rid);\n";
    }
    for my $m (@methodNames) {
# 	$r .= "    pthread_mutex_init(&${cb}Lock, 0);\n";
	$r .= "    pthread_cond_init(&${m}Signal, 0);\n";
    }
    $r .= "  }\n";
    $r .= qq{
  registration_uid_t getRegistrationUid() const {
    return rid;
  }

};

    for my $m ($sc->methods()) {
	my $name = $m->name(); 
	$r .= "  " . $m->toString("novirtual" => 1) . " {\n";
	my $ret = "";
	if (!$m->returnType()->isVoid()) {
	    $ret = "return ";
	}
	my $params = $this->joinParamsWithRid($m->params());
	if (!grep($name eq $_, @methodNames)) {
	    $r .= "    ";
	    $r .= $ret . "_sc.${name}($params);\n";
	}
	else {
	    $r .= "    ScopedLock sl(BaseMaceService::synclock);\n";
	    if ($m->options(SYNC_TYPE) eq BLOCK_TYPE) {
		my $map = "${name}Flags";
		my $id = $m->options(SYNC_ID_FIELDS);
		$r .= qq{
    ASSERT(!$map.containsKey($id));
    ${map}[$id] = 0;
};
	    }
	    elsif ($m->options(SYNC_TYPE) eq BROADCAST_TYPE) {
		$r .= "    ${name}Flag = TimeUtil::timeu();\n";
	    }

	    $r .= qq{    ${ret}_sc.$name($params);
};
	}
	$r .= "  }\n\n";
    }

    for my $h (@handlers) {
	for my $m ($h->methods()) {
	    if (!exists($callbacks{$m->name()})) {
		my $name = $m->name();
		my $ret = "";
		if (!$m->returnType()->isVoid()) {
		    $ret = "return ";
		}
		my $params = $this->joinParamsWithRid($m->params());
		my $hname = lcfirst($h->className());
		$r .= "  " . $m->toString("novirtual" => 1, "rid" => 1) . " {\n";
		$r .= "    ${ret}${hname}.${name}($params);\n";
		$r .= "  }\n\n";
	    }
	}
    }


    for my $m (@asyncMethods) {
	my $name = $m->name();
	if ($m->options(SYNC_RETURNTYPE) eq "void") {
	    $r .= "  void ";
	}
	else {
	    $r .= "  boost::shared_ptr<" . $m->options(SYNC_RETURNTYPE) . "> ";
	}
	my $syncname = $m->options(SYNC_NAME);
	$r .= $m->options(SYNC_NAME) . "(";
	$r .= join(", ", map { $_->toString() } $m->params());
	$r .= ") {\n";
	$r .= qq{    ADD_SELECTORS("${className}::${syncname}");\n};
	$r .= qq{    ScopedLog __scoped_log(selector, 0, selectorId->compiler, true, true, true, PIP);\n};
        $r .= "    ScopedLock sl(BaseMaceService::synclock);\n";
	my $params = $this->joinParamsWithRid($m->params());

	if ($m->options(SYNC_TYPE) eq BLOCK_TYPE) {
	    my $id = $m->options(SYNC_ID_FIELDS);
	    my $returnType = $m->options(SYNC_RETURNTYPE);
	    # XXX allow multiple fields for id?
	    $r .= qq{
    while (${name}Flags.containsKey($id)) {
      pthread_cond_wait(&${name}Signal, &BaseMaceService::synclock);
    }
    ${name}Flags[$id] = TimeUtil::timeu();
    _sc.${name}($params);
    while (!${name}Results.containsKey($id)) {
      pthread_cond_wait(&${name}Signal, &BaseMaceService::synclock);
    }
    boost::shared_ptr<$returnType> p = ${name}Results[$id];
    ${name}Results.erase($id);
    ${name}Flags.erase($id);
    pthread_cond_broadcast(&${name}Signal);
    return p;
};
	} # BLOCK_TYPE
	elsif ($m->options(SYNC_TYPE) eq BROADCAST_TYPE) {
	    $r .= qq{
    _sc.${name}($params);
    pthread_cond_wait(&${name}Signal, &BaseMaceService::synclock);
};
	} # BROADCAST_TYPE
	else {
	    die "unsupported type option: " . $m->options(SYNC_TYPE) . "\n";
	}

	$r .= "  } // $syncname\n";
    }

    while (my ($cbname, $cb) = each(%callbacks)) {
	$r .= "\n  " . $cb->toString("novirtual" => 1, "rid" => 1);
	my $m = $asyncMethods{$cbname};
	my $mname = $m->name();
	my $cbparams = join(", ", map { $_->name() } $cb->params());
	my $cbparamsRid = $this->joinParamsWithRid($cb->params());
	my $handler = lcfirst($handlers{$cbname}->className());

	$r .= "  {\n";
	$r .= qq{    ADD_SELECTORS("${className}::${cbname}");\n};
	$r .= "    ScopedLock sl(BaseMaceService::synclock);\n";

	if ($m->options(SYNC_TYPE) eq BLOCK_TYPE) {
	    my $resultName = $m->options(SYNC_RETURNTYPE);
	    my $flagsMapType = $m->options(SYNC_RETURNTYPE) . "AsyncMap";
	    my $flagsMap = "${mname}Flags";
	    my $resultsMap = "${mname}Results";
	    my $id = $m->options(SYNC_ID_FIELDS);
	    $r .= qq{
    ${flagsMapType}::iterator i = $flagsMap.find($id);
    if (i == $flagsMap.end()) {
      ABORT("Cannot find key in flag map");
    }
    if (i->second == 0) {
      $handler.$cbname($cbparamsRid);
      $flagsMap.erase(i);
    }
    else {
      ${resultsMap}[$id] = boost::shared_ptr<$resultName>(new $resultName($cbparams));
      uint64_t now = TimeUtil::timeu();
      uint64_t diff = now - i->second;
      maceout << diff << " " << $id << Log::endl;
      pthread_cond_broadcast(&${mname}Signal);
    }
};
	} # BLOCK_TYPE
	elsif ($m->options(SYNC_TYPE) eq BROADCAST_TYPE) {
	    my $flag = "${mname}Flag";
	    $r .= qq{
    if ($flag) {
      $handler.$cbname($cbparamsRid);
      uint64_t now = TimeUtil::timeu();
      uint64_t diff = now - $flag;
      $flag = 0;
      maceout << diff << Log::endl;
    }
    pthread_cond_broadcast(&${mname}Signal);
};
	} # BROADCAST_TYPE
	else {
	    die "unsupported type option\n";
	}

	$r .= "  } // $cbname\n";
    }

    $r .= "};\n\n";

    return $r;
#     open(OUT, ">$className.h") or die "cannot open $className.h: $!\n";
#     print OUT $r;
#     close(OUT);
} # generateSyncMethods

sub parseHandlers {
    my $this = shift;
    my $name = shift;

    if ($name =~ m|^ServiceClass$|) {
	return ();
    }

    my $f = $name . "ServiceClass.mh";

    my $path = findPath($f, @Mace::Compiler::Globals::INCLUDE_PATH);
    my @text = readFile("$path/$f");

    my @linemap;
    my @filemap;
    my @offsetmap;

    Mace::Compiler::MInclude::getLines("$path/$f", \@text, \@linemap, \@filemap, \@offsetmap);

    my $text = join("", @text);

    my $sc = $this->parser()->parse($text, \@linemap, \@filemap, \@offsetmap);

    my @r = ();
    for my $el (@{$sc->superclasses()}) {
	push(@r, $this->parseHandlers($el));
    }

    push(@r, @{$sc->handlers()});

    return @r;
} # parseHandlers

sub joinParamsWithRid {
    my $this = shift;
    my @p = @_;

    my $params = "";
    if (scalar(@p)) {
	$params = join(", ", map { $_->name() } @p);
    }
    if ($params) {
	$params .= ", ";
    }
    $params .= "rid";
    return $params;
}

1;
