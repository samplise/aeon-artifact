#
# ServiceImpl.pm : part of the Mace toolkit for building distributed systems
#
# Copyright (c) 2012, We-Chiu Chuang, Bo Sang, Charles Killian, James W. Anderson, Adolfo Rodriguez, Dejan Kostic
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
package Mace::Compiler::ServiceImpl;

use Data::Dumper;

#TODO: hashState default implementation in ServiceClass!

use strict;
use Class::MakeMethods::Utility::Ref qw( ref_clone );
use Mace::Compiler::ClassCache;
use Mace::Compiler::SQLize;
use v5.10.1;
use feature 'switch';

use Class::MakeMethods::Template::Hash
    (
     'new' => 'new',

     #[[[The following structures are all storing state as its being parsed

     'string' => 'name',
     'string' => 'header',
     'array'  => 'provides',
     'string'  => 'attributes', #Stored as comma separated list
     'string' => 'registration',
     'string' => 'trace',
     'boolean' => 'macetime',
     'number' => 'locking',   # service-wide locking
     'array' => 'logObjects',
     'hash --get_set_items' => 'wheres',
     'number' => 'queryLine',
     'boolean' => 'logService',
     'string' => 'queryArg',
     'string' => 'queryFile',
     'string' => 'localAddress',
     'hash --get_set_items'   => 'selectors',
     'array_of_objects' => ["constants" => { class => "Mace::Compiler::Param" }],
     'array' => "constants_includes",
     'array_of_objects' => ["constructor_parameters" => { class => "Mace::Compiler::Param" }],
     'array_of_objects' => ["service_variables" => { class => "Mace::Compiler::ServiceVar" }],
     'array'  => 'states',
     'array_of_objects' => ['typedefs' => { class => "Mace::Compiler::TypeDef" }],
     'array_of_objects' => ["state_variables" => { class => "Mace::Compiler::Param" }],
     'array_of_objects' => ["structuredLogs" => { class => "Mace::Compiler::Method" }],
#     'array_of_objects' => ["queries" => { class => "Mace::Compiler::Query" }],
     'array_of_objects' => ["timers" => { class => "Mace::Compiler::Timer" }],
     'array_of_objects' => ["auto_types" => { class => "Mace::Compiler::AutoType" }],
     'array_of_objects' => ["messages" => { class => "Mace::Compiler::AutoType" }],
     'array_of_objects' => ["detects" => { class => "Mace::Compiler::Detect" }],
     'array_of_objects' => ["contexts" => { class => "Mace::Compiler::Context" }],

     #These are the parsed Methods for remapping
     'array_of_objects' => ["usesDowncalls" => { class => "Mace::Compiler::Method" }],
     'array_of_objects' => ["usesUpcalls" => { class => "Mace::Compiler::Method" }],
     'array_of_objects' => ["implementsUpcalls" => { class => "Mace::Compiler::Method" }],
     'array_of_objects' => ["implementsDowncalls" => { class => "Mace::Compiler::Method" }],

     #These are methods added for asynchrony
     'array_of_objects' => ["asyncMethods" => { class => "Mace::Compiler::Method" }],
     'array_of_objects' => ["asyncHelperMethods" => { class => "Mace::Compiler::Method" }],
     'array_of_objects' => ["asyncDispatchMethods" => { class => "Mace::Compiler::Method" }],
     'array_of_objects' => ["asyncLocalWrapperMethods" => { class => "Mace::Compiler::Method" }],

     #These are methods added for broadcast transition
     'array_of_objects' => ["broadcastMethods" => { class => "Mace::Compiler::Method" }],
     'array_of_objects' => ["broadcastHelperMethods" => { class => "Mace::Compiler::Method" }],
     'array_of_objects' => ["broadcastDispatchMethods" => { class => "Mace::Compiler::Method" }],
     'array_of_objects' => ["broadcastLocalWrapperMethods" => { class => "Mace::Compiler::Method" }],

     #These are context helper methods 
     'array_of_objects' => ["contextHelperMethods" => { class => "Mace::Compiler::Method"}],

     #These are methods added for contexted routine calls
     'array_of_objects' => ["routineHelperMethods" => { class => "Mace::Compiler::Method"}],

     #These are methods added for contexted down/up calls
     'array_of_objects' => ["downcallHelperMethods" => { class => "Mace::Compiler::Method"}],
     'array_of_objects' => ["upcallHelperMethods" => { class => "Mace::Compiler::Method"}],

     'array_of_objects' => ["routines" => { class => "Mace::Compiler::Method" }],
     'array_of_objects' => ["routineObjects" => { class => "Mace::Compiler::RoutineObject" }],
     'array_of_objects' => ["transitions" => { class => "Mace::Compiler::Transition" }],
     'number' => "transitionEnd",
     'string' => "transitionEndFile",
     'hash --get_set_items' => 'rawTransitions',
     'array_of_objects' => ["safetyProperties" => { class => "Mace::Compiler::Properties::Property" }],
     'array_of_objects' => ["livenessProperties" => { class => "Mace::Compiler::Properties::Property" }],

     #End state from parse]]]

     'array' => 'ignores',
     'array' => 'deferNames',

     'hash --get_set_items' => 'selectorVars',
     'array_of_objects' => ['upcallDeferMethods' => { class => "Mace::Compiler::Method" }],
     'array_of_objects' => ['downcallDeferMethods' => { class => "Mace::Compiler::Method" }],
     'array_of_objects' => ['routineDeferMethods' => { class => "Mace::Compiler::Method" }],


     #These are the API methods this service is providing - upcalls and downcalls for transitions
     'array_of_objects' => ['providedMethods' => { class => "Mace::Compiler::Method" }],
     'array_of_objects' => ['usesHandlerMethods' => { class => "Mace::Compiler::Method" }],
     'array_of_objects' => ['timerMethods' => { class => "Mace::Compiler::Method" }],
     'array_of_objects' => ['timerHelperMethods' => { class => "Mace::Compiler::Method" }],
     #These are the API methods this service is providing - public interface calls
     'array_of_objects' => ['providedMethodsAPI' => { class => "Mace::Compiler::Method" }],
     'array_of_objects' => ['usesHandlerMethodsAPI' => { class => "Mace::Compiler::Method" }],
     #These are the API methods this service is providing - private serial forms only
     'array_of_objects' => ['providedMethodsSerials' => { class => "Mace::Compiler::Method" }],
     'array_of_objects' => ['usesHandlerMethodsSerials' => { class => "Mace::Compiler::Method" }],

     #These are the aspect transition methods.
     'array_of_objects' => ["aspectMethods" => { class => "Mace::Compiler::Method" }],
     'array_of_objects' => ["onChangeVars" => { class => "Mace::Compiler::Param" }],

     'array' => 'usesHandlers',
     'hash_of_arrays' => ['usesHandlerNames' => 'string'],

     #These are the upcalls this service may make -- a unified list and a per-handler list
     'array_of_objects' => ['providedHandlerMethods' => { class => "Mace::Compiler::Method" }],
     'array_of_objects' => ['providedHandlers' => { class => "Mace::Compiler::ServiceClass" }],

     #These are the downcalls this service may make -- a unified list and a per-serviceclass list
     'array_of_objects' => ['usesClassMethods' => { class => "Mace::Compiler::Method" }],
     'hash_of_arrays' => ['usesClasses' => { class => "Mace::Compiler::Method" }],

     #This stack of guard methods is used during parsing to parse guard blocks
     # chuangw: deprecated??
     'array_of_objects' => ['guards' => { class => 'Mace::Compiler::Guard' }],

     'object' => ['parser' => { class => "Parse::RecDescent" }],

     'string' => "origMacFile",
     'string' => "annotatedMacFile",

     'array' => "downcall_registrations",

     'object' => ['asyncExtraField' => { class => "Mace::Compiler::AutoType" }],

     'boolean' => 'useTransport',
    );
my %transitionNameMap;

sub toString {
    my $this = shift;
    my $s =
	"Service: ".$this->name()."\n"
	    . "Provides: ".join(', ', $this->provides())."\n"
		. "Trace Level: ".$this->trace()."\n"
		    ;

    $s .= "Selectors { \n";
    while ( my ($selector, $selstring) = each(%{$this->selectors()}) ) {
	$s .= "  $selector = $selstring\n";
    }
    $s .= "}\n";

    $s .= "Constants { \n";
    $s .= join("", map { "  ".$_->toString().";\n" } $this->constants());
    $s .= "}\n";

    $s .= "Constructor Parameters { \n";
    $s .= join("", map { "  ".$_->toString().";\n" } $this->constructor_parameters());
    $s .= "}\n";

    $s .= "States { " . join(', ', $this->states()) . " }\n";

    $s .= "Service Variables { \n";
    $s .= join("", map { "  ".$_->toString().";\n" } $this->service_variables());
    $s .= "}\n";

    $s .= "Typedefs { \n";
    while ( my ($id, $type) = each(%{$this->typedefs()}) ) {
	$s .= "  $id => ".$type->toString()."\n";
    }
    $s .= "}\n";

    $s .= "State Variables { \n";
    $s .= join("", map { "  ".$_->toString(nodefaults => 1).";\n" } $this->state_variables());
    $s .= "}\n";

    $s .= "Auto Types { \n";
    $s .= join("", map { "  ".$_->toString().";\n" } $this->auto_types());
    $s .= "}\n";

    $s .= "Messages { \n";
    $s .= join("", map { "  ".$_->toString().";\n" } $this->messages());
    $s .= "}\n";

    $s .= "Uses Upcalls { \n";
    $s .= join("", map { "  ".$_->toString().";\n" } $this->usesUpcalls());
    $s .= "}\n";

    $s .= "Uses Downcalls { \n";
    $s .= join("", map { "  ".$_->toString().";\n" } $this->usesDowncalls());
    $s .= "}\n";

    $s .= "Implements Upcalls { \n";
    $s .= join("", map { "  ".$_->toString().";\n" } $this->implementsUpcalls());
    $s .= "}\n";

    $s .= "Implements Downcalls { \n";
    $s .= join("", map { "  ".$_->toString().";\n" } $this->implementsDowncalls());
    $s .= "}\n";

    $s .= "Routines { \n";
    $s .= join("", map { "  ".$_->toString().";\n" } $this->routines());
    $s .= "}\n";

    $s .= "Routine Objects { \n";
    $s .= join("", map { "  ".$_->toString().";\n" } $this->routineObjects());
    $s .= "}\n";

    $s .= "Transitions { \n";
    $s .= join("", map { "  ".$_->toString().";\n" } $this->transitions());
    $s .= "}\n";

    return $s;
}

my $context_count;
sub hasContexts {
    my $this = shift;

    if( not defined $context_count  ){
        $context_count = ${ $this->contexts() }[0]->count_subcontexts();
    }

    return $context_count;
}

sub printHFile {
    my $this = shift;
    my $outfile = shift;

    my $servicename = $this->name();

    my $r = Mace::Compiler::GeneratedOn::generatedOnHeader("$servicename header file");
    print $outfile $r;

    print $outfile <<END;

#ifndef ${servicename}_header
#define ${servicename}_header

END

    $this->printIncludesH($outfile);
    $this->printUsingH($outfile);
    $this->printIncludeBufH($outfile);

    print $outfile <<END;
    namespace ${servicename}_namespace {

        void load_protocol();

END
    #  $this->printConstantsH($outfile);
    $this->printSelectorConstantsH($outfile);

    $this->printAutoTypeForwardDeclares($outfile);
    $this->printTypeDefs($outfile);

    my $messageDeclares = join("", map {$_->toForwardDeclare()} $this->messages());

    my $eventreqDeclares = join("", map {
      my $at = $_->options("serializer");
      if( defined $at ){
        $at->toForwardDeclare()
      }
    } 
      ($this->asyncMethods(), $this->broadcastMethods(), $this->timerMethods(),  $this->usesHandlerMethods()  , $this->providedHandlerMethods(), $this->providedMethods() ), $this->routines() );

    my @contexts = $this->contexts();
    my $contextForwardDeclares = $this->printContextForwardDeclaration( \@contexts );
    print $outfile qq{
	//Message Declarations
	$messageDeclares
  // Event Request Declarations
  $eventreqDeclares
	//Context Forward Declarations
	$contextForwardDeclares
    };

    print $outfile "\nclass ${servicename}Service;\n";
    print $outfile "typedef ${servicename}Service ServiceType;\n";
    print $outfile "typedef std::deque<std::pair<uint64_t, const ServiceType*> > VersionServiceMap;\n";
    print $outfile "typedef mace::map<int, ${servicename}Service const *, mace::SoftState> _NodeMap_;\n";
    print $outfile "typedef mace::map<MaceKey, int, mace::SoftState> _KeyMap_;\n";

    print $outfile qq/static const char* __SERVICE__ __attribute((unused)) = "${servicename}";\n/;
    $this->printAutoTypes($outfile);
    $this->printContextClasses($outfile, \@contexts );
    $this->printDeferTypes($outfile);
    $this->printService($outfile);
    print $outfile $this->generateMergeClasses();
    $this->printRoutineObjects($outfile);
    $this->printMessages($outfile);
    $this->printEventRequests($outfile);
    $this->printTimerClasses($outfile);

#    print $outfile "\nclass ${servicename}Dummy;\n";
#    $this->printDummyClass($outfile);
#    $this->printTimerDummyClasses($outfile);
    $this->printStructuredLogDummies($outfile);
    print $outfile <<END;
    }
#endif
END

} # printHFile

sub printContextForwardDeclaration {
    my $this = shift;
    my $contexts = shift;
    my $r = "";

    my @subcontexts;
    foreach my $context( @{$contexts} ) {
        $r .= "class $context->{className};\n";
        push @subcontexts,$context->subcontexts();
    }
    if( @subcontexts != 0 ){
        $r .= $this->printContextForwardDeclaration(\@subcontexts);
    }
    return $r;
}

sub printStructuredLogDummies {
    my $this = shift;
    my $outfile = shift;

    for my $log ($this->structuredLogs()) {
	if ($log->doStructuredLog()) {
	    $this->printStructuredLogMemoryDummy($log, $outfile, 0);
	}
    }

    for my $at ($this->auto_types()) {
	for my $m ($at->methods()) {
	    if ($m->doStructuredLog()) {
		$this->printStructuredLogDummy($m, $outfile, 0);
	    }
	}
    }

    for my $slog (grep(!($_->name =~ /^(un)?register.*Handler$/), $this->providedMethods()), @{$this->aspectMethods()}, @{$this->usesHandlerMethods()}, @{$this->timerMethods()}) {
#	if (not $slog->messageField() and (defined $slog->options('transitions') or scalar(grep {$_ eq $slog->name} $this->ignores()))) {
	if ($slog->doStructuredLog()) {
	    #&& $slog->shouldLog()) {
	    $this->printStructuredLogDummy($slog, $outfile, 0);
	}
    }
    for my $slog (@{$this->usesClassMethods()}) {
	if ($slog->doStructuredLog()) {
	    #&& $slog->shouldLog()) {
	    $this->printStructuredLogDummy($slog, $outfile, 0);
	}
    }
    for my $slog (@{$this->providedHandlerMethods()}) {
	if ($slog->doStructuredLog()) {
	    #&& $slog->shouldLog()) {
	    $this->printStructuredLogDummy($slog, $outfile, 0);
	}
    }
    for my $slog ($this->routines()) {
	if ($slog->doStructuredLog()) {
	    #&& $slog->shouldLog()) {
	    $this->printStructuredLogDummy($slog, $outfile, 1);
	}
    }
}

sub printStructuredLogMemoryDummy {
    my $this = shift;
    my $log = shift;
    my $outfile = shift;
    my $ll = shift;

    if ($log->messageField() or $log->getLogLevel($this->traceLevel()) <= $ll) {
        return;
    }

    my $name = $this->name();
    my $logName = (defined $log->options('binlogname')) ? $log->options('binlogname') : $log->name();
    my $shortName = (defined $log->options('binlogshortname')) ? $log->options('binlogshortname') : $log->name();
    my $longName = (defined $log->options('binloglongname')) ? $log->options('binloglongname') : $log->name();
    #print "DEBUG: Printing $name -- $logName Dummy -- ".$log->getLogLevel($this->traceLevel())."\n";
    my $className = "class ${logName}Dummy : public mace::BinaryLogObject, public mace::PrintPrintable";
    my $param;
    my $pname;
    my $printBody = qq{out << "$shortName(";\n};
#    my $serialBody = "";
    my $constructor1 = "${logName}Dummy() ";
    my @constructor1initlist;
    my $constructor2 = "${logName}Dummy(";
    my @constructor2varlist;
    my @constructor2initlist;
    my $destructorBody = "";

    print $outfile "$className {\n";
    print $outfile "protected:\n";
    print $outfile "static mace::LogNode* rootLogNode;\n";
    print $outfile "public:\n";

    for $param ($log->params()) {
	$pname = $param->name();
	my $ptype = $param->type()->type();
	print $outfile "$ptype $pname;\n";
#	print $outfile "$newType $pname;\n";
	$printBody .= qq{out << "[$pname=";
mace::printItem(out, &$pname);
out << "]";
};
	if (!@{$this->logObjects()}) {
	    # if there are no objects to log (no queries block), then
	    # log everything
	    $param->shouldLog(1);
	}
#        $serialBody .= "mace::serialize(__str, &$pname);\n";
        push @constructor1initlist, "_$pname(new $ptype()), $pname(*_$pname)";
        push @constructor2varlist, "$ptype ___$pname";
        push @constructor2initlist, "$pname(___$pname)";
#        $destructorBody .= "if (_$pname != NULL) { delete _$pname; _$pname = NULL; }\n";
    }

    if ($log->count_params()) {
        $constructor1 .= ": ". join(", ", @constructor1initlist) . " { }\n";
        $constructor2 .= join(", ", @constructor2varlist). ") : ". join(", ", @constructor2initlist) . " { }\n";;
    } else {
        $constructor1 .= " { }\n";
        $constructor2 = "";
    }

    print $outfile <<END;

    const std::string& getLogType() const {
        static std::string type = "${name}_${logName}";
	return type;
    }

    const std::string& getLogDescription() const {
        static const std::string desc = "${name}::${longName}";
        return desc;
    }

    LogClass getLogClass() const {
        return SERVICE_BINLOG;
    }

    mace::LogNode*& getChildRoot() const {
      return rootLogNode;
    }

    $constructor2

    ~${logName}Dummy() {
    }

    void serialize(std::string& __str) const {
    }

    int deserialize(std::istream& is) throw(mace::SerializationException) {
	int bytes = 0;
	return bytes;
    }
END
    my $body;
#    for $param ($log->params()) {
#	$pname = $param->name();
#	$body .= "bytes += mace::deserialize(is, _$pname);\n";
#    }
#    $body .= "return bytes;\n}\n";
    $body .= "void print(std::ostream& out) const {\n";
    $body .= qq~$printBody out << ")";\n}\n~;
    print $outfile $body;
    my $sqlizeBody = Mace::Compiler::SQLize::generateBody(\@{$log->params()}, 0, 0);

    print $outfile <<END;
    void sqlize(mace::LogNode* __node) const {
	$sqlizeBody
    }
    };
END
}

sub printStructuredLogDummy {
    my $this = shift;
    my $log = shift;
    my $outfile = shift;
    my $ll = shift;

#    if ($log->messageField() or $log->getLogLevel($this->traceLevel()) <= $ll) {
#        return;
#    }

    if ($log->getLogLevel($this->traceLevel()) <= $ll) {
        return;
    }

    my $name = $this->name();
    my $logName = (defined $log->options('binlogname')) ? $log->options('binlogname') : $log->name();
    my $shortName = (defined $log->options('binlogshortname')) ? $log->options('binlogshortname') : $log->name();
    my $longName = (defined $log->options('binloglongname')) ? $log->options('binloglongname') : $log->name();
    #print "DEBUG: Printing $name -- $logName Dummy -- ".$log->getLogLevel($this->traceLevel())."\n";
    my $className = "class ${logName}Dummy : public mace::BinaryLogObject, public mace::PrintPrintable";
    my $param;
    my $pname;
    my $printBody = qq{out << "$shortName(";\n};
#    my $serialBody = "";
    my $constructor1 = "${logName}Dummy() ";
    my @constructor1initlist;
    my $constructor2 = "${logName}Dummy(";
    my @constructor2varlist;
    my @constructor2initlist;
    my $destructorBody = "";

    print $outfile "$className {\n";
    print $outfile "protected:\n";
    print $outfile "static mace::LogNode* rootLogNode;\n";
    print $outfile "public:\n";

    if (!$log->messageField()) {
	for $param ($log->params()) {
	    $pname = $param->name();
	    my $ptype = $param->type()->type();
	    print $outfile "$ptype* _$pname;\n";
	    my $const = "";
	    my $numStars = ($ptype =~ tr/\*//);
	    my $newType = "const $ptype&";

	    if ($numStars > 0) {
		$ptype =~ /([^*]+)[*]*/;
		$newType = $1;
		for (my $i = 0; $i < $numStars; $i++) {
		    $newType .= " const*";
		}
		$newType .= " const&";
	    }
	    print $outfile "$newType $pname;\n";
	    $printBody .= qq{out << "[$pname=";
			     mace::printItem(out, &$pname);
			     out << "]";
			 };
	    if (!@{$this->logObjects()}) {
		# if there are no objects to log (no queries block), then
		# log everything
		$param->shouldLog(1);
	    }
#        $serialBody .= "mace::serialize(__str, &$pname);\n";
	    push @constructor1initlist, "_$pname(new $ptype()), $pname(*_$pname)";
	    push @constructor2varlist, "$newType ___$pname";
	    push @constructor2initlist, "_$pname(NULL), $pname(___$pname)";
	    $destructorBody .= "if (_$pname != NULL) { delete _$pname; _$pname = NULL; }\n";
	}
    }
    if ($log->count_params() and !$log->messageField()) {
        $constructor1 .= ": ". join(", ", @constructor1initlist) . " { }\n";
        $constructor2 .= join(", ", @constructor2varlist). ") : ". join(", ", @constructor2initlist) . " { }\n";;
    } else {
        $constructor1 .= " { }\n";
        $constructor2 = "";
    }

    print $outfile <<END;

    const std::string& getLogType() const {
        static std::string type = "${name}_${logName}";
	return type;
    }

    const std::string& getLogDescription() const {
        static const std::string desc = "${name}::${longName}";
        return desc;
    }

    LogClass getLogClass() const {
        return SERVICE_BINLOG;
    }

    mace::LogNode*& getChildRoot() const {
      return rootLogNode;
    }

    $constructor1

    $constructor2

    ~${logName}Dummy() {
        $destructorBody
    }

    void serialize(std::string& __str) const {
    }

    int deserialize(std::istream& is) throw(mace::SerializationException) {
	int bytes = 0;
	return bytes;
    }
END
    my $body;
#    for $param ($log->params()) {
#	$pname = $param->name();
#	$body .= "bytes += mace::deserialize(is, _$pname);\n";
#    }
#    $body .= "return bytes;\n}\n";
    $body .= "void print(std::ostream& out) const {\n";
    $body .= qq~$printBody out << ")";\n}\n~;
    print $outfile $body;
    my $sqlizeBody;

    if (!$log->messageField()) {
	$sqlizeBody = Mace::Compiler::SQLize::generateBody(\@{$log->params()}, 0, 0);
    }
    else {
	my @emptyArr;
	$sqlizeBody = Mace::Compiler::SQLize::generateBody(\@emptyArr, 0, 0);
    }

    print $outfile <<END;
    void sqlize(mace::LogNode* __node) const {
	$sqlizeBody
    }
    };
END
}

sub printDeferTypes {
    my $this = shift;
    my $outfile = shift;
    print $outfile <<EOF;
    //BEGIN Mace::Compiler::ServiceImpl::printDeferTypes
EOF

    for my $m ($this->upcallDeferMethods(), $this->downcallDeferMethods(), $this->routineDeferMethods()) {
	print $outfile generateDeferType($m);
    }
    print $outfile <<EOF;
    //END Mace::Compiler::ServiceImpl::printDeferTypes
EOF

}

sub generateDeferType {
    my $m = shift;
    my $n = getVarFromMethod($m);
    my $type = "__DeferralArgsFor${n}";
    my $r = "struct $type {\n";
    $r .= "uint64_t __calltime;\n";
    $r .= "$type() : __calltime(TimeUtil::timeu()) { };\n";
    if ($m->count_params()) {
	$r .= ("$type(\n" .
	       join(", ", map{$_->toString(paramconst => 1, paramref=> 1)}
		    $m->params()) .") : __calltime(TimeUtil::timeu()), " .
	       join(", ", map{$_->name()."(".$_->name().")"} $m->params()) .
	       "{ }\n");
	$r .= join("\n", map{$_->type()->type()." ".$_->name().";"} $m->params());
    }
    $r .= "};\n";
    return $r;
}

sub generateProcessDefer {
    my $type = shift;
    my @marr = @_;
    if (scalar(@marr) == 0) {
        return "";
    }
    if ($type) {
        $type .= "_";
    }
    my $r = "";
    $r .= "while (" . join(" || ", map { "!__deferralArgList_" . getVarFromMethod($_) . ".empty()" } @marr) . ") {
uint64_t _firstcall = std::numeric_limits<uint64_t>::max();
";
    for my $m (@marr) {
        my $varm = getVarFromMethod($m);
        my $vl = "__deferralArgList_${varm}";
        $r .= "if (!$vl.empty()) { _firstcall = std::min(_firstcall, $vl.front().__calltime); }\n";
    }

    for my $m (@marr) {
        my $varm = getVarFromMethod($m);
        my $vl = "__deferralArgList_${varm}";
        $r .= "if (!$vl.empty() && _firstcall == $vl.front().__calltime) {\n";
        if ($m->count_params()) {
            $r .= "__DeferralArgsFor${varm}& a = __deferralArgList_${varm}.front();\n";
        }
        $r .= $type . $m->name() . "(" . join(", ", map { "a." . $_->name() } $m->params()) . ");
    __deferralArgList_${varm}.pop_front();
    }\n";
    }
    $r .= "}\n";
    return $r;
}

sub generateAddDefer {
    my $this = shift;
    my $type = shift;
    my $methods = shift;
    my $name = $this->name();
    if ($type) {
	$type .= "_";
    }

    # Note: add defer to the parameters included in per-method. it has two types - up / downcall.
    return map{"void ".$_->toString(noreturn=>1,nodefaults=>1,methodprefix=>"${name}Service::defer_${type}",
						 noid=> 0, novirtual => 1).
						     " { mace::ScopedStackExecution::addDefer(this); __deferralArgList_".
							 getVarFromMethod($_).".push_back(__DeferralArgsFor".getVarFromMethod($_)."(".
						     join(",", map{ $_->toString(notype=>1, nodefaults=>1, noline=>1) } $_->params()).")); }"
			} @$methods;
}

sub printCCFile {
    my $this = shift;
    my $outfile = shift;

    my $servicename = $this->name();

    my $serviceVariableString = '';
    #  my $serviceVariableString = join("\n", map{my $sv=$_->service; qq{#include "$sv.h"
    #                                                                 using ${sv}_namespace::${sv}Service;}} $this->service_variables());
    my $timerDelete = join("\n", map{my $t = $_->name(); "delete &$t;"} $this->timers());
    my $unregisterInstance = join("\n", map{ qq/mace::ServiceFactory<${_}ServiceClass>::unregisterInstance("${\$this->name}", this);/ } $this->provides());
    my $modelCheckSafety = join("\n", map{"// ${\$_->toString()}
      ${\$_->toMethod(nostatic=>1, methodprefix=>$servicename.qq/Service::/)}"    } $this->safetyProperties);
    my $modelCheckLiveness = join("\n", map{"// ${\$_->toString()}
      ${\$_->toMethod(nostatic=>1, methodprefix=>$servicename.qq/Service::/)}"    } $this->livenessProperties);

    my $r = Mace::Compiler::GeneratedOn::generatedOnHeader("$servicename main source file");
    my $sqlizeBody = Mace::Compiler::SQLize::generateBody(\@{$this->state_variables()}, 1, 1);

    my $incSimBasics = "";
    if ($this->count_safetyProperties() or $this->count_livenessProperties()) {
        $incSimBasics = qq/#include "SimulatorBasics.h"/;
    }
    my $incContextService = "";

    if( $Mace::Compiler::Globals::useFullContext ){
      $incContextService = qq/#include "ContextService.h"/;
    }

    print $outfile $r;
    print $outfile <<END;
    //BEGIN Mace::Compiler::ServiceImpl::printCCFile
#include "mace.h"
$incContextService
#include "EventExtraField.h"
#include "NumberGen.h"
$incSimBasics
#include "$servicename.h"
#include "$servicename-macros.h"
#include "Enum.h"
#include "Log.h"
#include "ScopedLock.h"
#include "MaceTime.h"
#include "ScopedLog.h"
#include "ScopedSerialize.h"
#include "pip_includer.h"
#include "AsyncDispatch.h"
#include "HeadEventDispatch.h"
#include "lib/MaceTime.h"
#include "lib/ServiceFactory.h"
#include "lib/ServiceConfig.h"
#include <boost/algorithm/string.hpp>
#include "lib/SysUtil.h"
#include "lib/ScopedContextRPC.h"

    bool operator==(const mace::map<int, mace::map<int, ${servicename}_namespace::${servicename}Service*, mace::SoftState>::const_iterator, mace::SoftState>::const_iterator& lhs, const mace::map<int, ${servicename}_namespace::${servicename}Service*, mace::SoftState>::const_iterator& rhs) {
        return lhs->second == rhs;
    }
    bool operator==(const mace::map<int, ${servicename}_namespace::${servicename}Service*, mace::SoftState>::const_iterator& lhs, const mace::map<int, mace::map<int, ${servicename}_namespace::${servicename}Service*, mace::SoftState>::const_iterator, mace::SoftState>::const_iterator& rhs) {
        return lhs == rhs->second;
    }
END
    print $outfile "namespace mace {\ntemplate<typename T> mace::LogNode* mace::SimpleLogObject<T>::rootLogNode = NULL;\n}\n";
    print $outfile <<END;
    namespace ${servicename}_namespace {
    mace::LogNode* ${servicename}Service::rootLogNode = NULL;
//    mace::LogNode* ${servicename}Dummy::rootLogNode = NULL;
        //Selector Constants
END

    $this->printSelectorConstantsCC($outfile);

    print $outfile <<END;
	//Change Tracker (optional)
END
    $this->printChangeTracker($outfile);

    print $outfile <<END;
	// When entering the service stack, create event
END

    print $outfile <<END;

	//service variable includes and uses
	    $serviceVariableString

	    //Transition and Guard Implementations
END

    $this->printTransitions($outfile);

    print $outfile <<END;

	//Structured Logging Functions
END
    $this->printStructuredLogs($outfile);

    print $outfile <<END;
    // logging funcs
END
    $this->shouldLogFuncsBody($outfile);

    print $outfile <<END;

	//Auto Type Methods
END
    $this->defineAutoTypeMethods($outfile);

    print $outfile <<END;

	//Routines
END

    $this->printRoutines($outfile);

    print $outfile <<END;

	//Timer Demux (and utility timer)
END

    $this->printTimerDemux($outfile);

    my $getMessageNameCases = join("\n", map{my $n = $_->name; qq{case ${n}::messageType: return "${servicename}::$n";}} $this->messages());
    my $getStateNameCases = join("\n", map{qq/case $_: return "${servicename}::$_";/} $this->states());
    my $traceStateChange = ($this->traceLevel() > 0)? q{Log::logf(selectorId, "FSM: state changed to %s", getStateName(state));}:"";
#    my $printStateVars = join("\n", map { $_ . " __out << std::endl;" } (grep(/./, map { $_->toPrint("__out") } $this->state_variables())));
    my $printNodeStateVars = join("\n", map { $_->toPrintNode("__printer") } $this->state_variables());
    my $printStateVars = join("\n", map { $_ . " __out << std::endl;" } (grep(/./, map { $_->toPrint("__out",0) } $this->state_variables())));
    my $printState_StateVars = join("\n", grep(/./, map { $_->toPrintState("__out") } $this->state_variables()));
    my $serializeStateVars = "mace::serialize(__str, &state);\n" . join("\n", (grep(/./, map { $_->toSerialize("__str") } $this->state_variables())));
    my $serializeContexts = join("\n", (grep(/./, map { $_->toSerialize("__str") } $this->contexts() )));
    my $deserializeContexts = join("\n", (grep(/./, map { $_->toDeserialize("__in", prefix => "serializedByteSize += ") }  $this->contexts()  )));
    my $deserializeStateVars = "serializedByteSize += mace::deserialize(__in, &_actual_state);\n" . join("\n", (grep(/./, map { $_->toDeserialize("__in", prefix => "serializedByteSize += ") } $this->state_variables())));
#    my $printScheduledTimers = join("\n", map { $_->toPrint("__out")." __out << std::endl;" } $this->timers());
    my $printNodeScheduledTimers = join("\n", map { $_->toPrintNode("__printer") } $this->timers());
    my $printScheduledTimers = join("\n", map { $_->toPrint("__out", 0)." __out << std::endl;" } $this->timers());
    my $printState_ScheduledTimers = join("\n", map { $_->toPrintState("__out") } $this->timers());
    my $serializeScheduledTimers = join("\n", map { $_->toSerialize("__str") } $this->timers());
    my $deserializeScheduledTimers = join("\n", map { $_->toDeserialize("__in", prefix => "serializedByteSize += ") } $this->timers());
    my $printLowerServices = join("\n", map { unless($_->intermediate()) {" << _".$_->name()} else {""} } $this->service_variables());
    my $printState_LowerServices = join("\n", map { $_->toPrintState("__out") } $this->service_variables());

    # Note : deferred calls are handled in here...
    my $deferralCalls = "";
    $deferralCalls .= generateProcessDefer("upcall", $this->upcallDeferMethods());
    $deferralCalls .= generateProcessDefer("downcall", $this->downcallDeferMethods());
    $deferralCalls .= generateProcessDefer("", $this->routineDeferMethods());

    my $processDeferred = "void ${servicename}Service::processDeferred() { $deferralCalls }";
    if ($this->traceLevel() > 2) {
        $processDeferred = qq~void ${servicename}Service::processDeferred() {
            $deferralCalls
	    ~;
	    if ($this->logService()) {
		my $clause = $this->wheres->{$servicename};
		my $logStr = "Log::binaryLog(logid, *this);\n";

		if (defined($clause)) {
		    $logStr = "if $clause {\n" . $logStr . "}\n";
		}
		$logStr = "if (mace::LogicalClock::instance().shouldLogPath()) {\n" . $logStr . "}\n";
		$processDeferred .= qq~static const log_id_t logid = Log::getId("SNAPSHOT::${servicename}");
		// log to both text and binary logs for now until we come up with a good solution
		~;
		$processDeferred .= $logStr;
	    }
	    $processDeferred .= "}\n";
#        } ~;
    }
    my $accessorMethods = "";
 

    print $outfile <<END;

    //Load Protocol

END

    $this->printLoadProtocol($outfile);

    print $outfile <<END;

    //Constructors
END

    $this->printConstructor($outfile);


    print $outfile <<END;


	//Destructor
    ${servicename}Service::~${servicename}Service() {
      $timerDelete
      $unregisterInstance
    }

    // Methods for snapshotting...
    void ${servicename}Service::snapshot(const uint64_t& ver) const {
        ADD_SELECTORS("${servicename}Service::snapshot");
        //Assumes state is locked.
        ${servicename}Service* _sv = new ${servicename}Service(*this);
        macedbg(1) << "Snapshotting version " << ver << " for this " << this << " value " << _sv << Log::endl;
        ASSERT(versionMap.empty() || versionMap.back().first < ver);
        versionMap.push_back(std::make_pair(ver,_sv));
    }
    void ${servicename}Service::snapshotRelease(const uint64_t& ver) const {
        ADD_SELECTORS("${servicename}Service::snapshot");
        //Assumes state is locked.
        while (!versionMap.empty() && versionMap.front().first < ver) {
            macedbg(1) << "Deleting snapshot version " << versionMap.front().first << " for service " << this << " value " << versionMap.front().second << Log::endl;
            delete versionMap.front().second;
            versionMap.pop_front();
        }
    }

    $accessorMethods

    //Auxiliary Methods (dumpState, print, serialize, deserialize, processDeferred, getMessageName, changeState, getStateName)
    void ${servicename}Service::dumpState(LOGLOGTYPE& logger) const {
        logger << "state_dump: " << *this << std::endl;
        return;
    }

    void ${servicename}Service::printNode(mace::PrintNode& __pr, const std::string& name) const {
        mace::PrintNode __printer(name, "${servicename}Service");
        const char* __pr_stateName = getStateName(state);
        mace::printItem(__printer, "state", &__pr_stateName);
        $printNodeStateVars
        $printNodeScheduledTimers
        __pr.addChild(__printer);
        return;
    }

    void ${servicename}Service::print(std::ostream& __out) const {
        __out << "state=" << getStateName(state) << std::endl;
        $printStateVars
        $printScheduledTimers
        __out << std::endl;
        if(_printLower) {
            __out $printLowerServices << std::endl;
        }
        return;
    }
    void ${servicename}Service::printState(std::ostream& __out) const {
        __out << "state=" << getStateName(state) << std::endl;
        $printState_StateVars
        $printState_ScheduledTimers
        __out << std::endl;
        if(_printLower) {
            $printState_LowerServices
            }
        return;
    }

    void ${servicename}Service::sqlize(mace::LogNode* __node) const {
        $sqlizeBody
    }

    void ${servicename}Service::serialize(std::string& __str) const {
        $serializeStateVars
        $serializeContexts
        $serializeScheduledTimers
        mace::serialize( __str, &__local_address );
        return;
    }

    int ${servicename}Service::deserialize(std::istream& __in) throw(SerializationException){
        int serializedByteSize = 0;
        $deserializeStateVars
        $deserializeContexts
        $deserializeScheduledTimers

        MaceKey oldLocalAddress;
        serializedByteSize += mace::deserialize(__in, &oldLocalAddress);

        return serializedByteSize;
    }
END

    print $outfile <<END;
    $processDeferred
    const char* ${servicename}Service::getMessageName(uint8_t messageType) const {
        switch(messageType) {
            $getMessageNameCases
            default: ASSERT(false); return "INVALID MESSAGE NUMBER";
        }
    }
    void ${servicename}Service::changeState(state_type stateNum, int selectorId) {
        _actual_state = stateNum;
        $traceStateChange
    }
    const char* ${servicename}Service::getStateName(state_type state) const {
        switch(static_cast<uint64_t>(state)) {
            $getStateNameCases
            default: ASSERT(false); return "INVALID STATE NUMBER";
        }
    }

    //API demux (provides -- registration methods, maceInit/maceExit special handling)
END

    $this->printAPIDemux($outfile);
    $this->printAsyncDemux($outfile);
    $this->printBroadcastDemux($outfile);
    $this->printAspectDemux($outfile);
    $this->printHandlerRegistrations($outfile);

    print $outfile <<END;

    //Handler demux (uses handlers)
END

    $this->printHandlerDemux($outfile);

    print $outfile <<END;

    //Downcall helpers (uses)
END

    $this->printDowncallHelpers($outfile);

    print $outfile <<END;

    //Upcall helpers (provides handlers)
END

    $this->printUpcallHelpers($outfile);

    print $outfile <<END;

    //Async helpers
END

    my $name = $this->name();
    # FIXME: some of them do not need PREPARE_FUNCTION
    map {
        print $outfile $_->toString(methodprefix=>"${name}Service::", body => 1,selectorVar => 1, prepare => 1, nodefaults=>1);
    } $this->asyncHelperMethods(), $this->broadcastHelperMethods(), $this->routineHelperMethods(), $this->timerHelperMethods(), $this->downcallHelperMethods(), $this->upcallHelperMethods(); #, $this->appUpcallDispatchMethods();
    map {
        print $outfile $_->toString(methodprefix=>"${name}Service::", body => 1,selectorVar => 1, prepare => 0, nodefaults=>1);
    } $this->asyncLocalWrapperMethods(), $this->broadcastLocalWrapperMethods(), $this->contextHelperMethods();
    map {
        print $outfile $_->toString(methodprefix=>"${name}Service::", body => 1,selectorVar => 1, prepare => 0, nodefaults=>1, traceLevel=>1);
    } $this->asyncDispatchMethods(), $this->broadcastDispatchMethods() ;

    print $outfile <<END;

    //Serial Helper Demux
END

    $this->printSerialHelperDemux($outfile);

    my $applicationUpcallDeferralQueue="";
    my $hnumber = 1;
    for my $m ( $this->providedHandlerMethods() ){
        if( !$m->returnType->isVoid ){
            $hnumber++;
            next;
        }
        my $serialize = "";
        my @serializedParamName;
        if ($m->options('original')) {
            #TODO: try/catch Serialization
            my $sorigmethod = $m->options('original');
            my @oparams = $sorigmethod->params();
            for my $p ($m->params()) {
                my $op = shift(@oparams);
                if (! $op->type->eq($p->type)) {
                    my $optype = $op->type->type();
                    my $opname = $op->name;
                    my $ptype = $p->type->type();
                    my $pname = $p->name;
                    $serialize .= qq{ $optype $opname;
                                      ScopedSerialize<$optype, $ptype > __ss_$pname($opname, it->second.$pname);
                                  };
                    push @serializedParamName, $opname;
                }else{
                    push @serializedParamName, "it->second." . $op->name;
                }
            }
        }else{
            map{ push @serializedParamName, "it->second." . $_->name() } $m->params() ;
        }
        my @handlerArr = @{$m->options('class')};
        my $handler = $handlerArr[0];
        my $hname = $handler->name;
        my $iterator = "iterator";
        my $mname = $m->name;
        my $rid = $m->params()->[-1]->name();
        if ($m->isConst()) {
            $iterator = "const_iterator"
        }
        my $body = $m->body;
        my $callm = $m->name."(".join(",", @serializedParamName ).")";
        $applicationUpcallDeferralQueue .= qq#{
            std::pair< Deferred_${hnumber}_${mname}::iterator, Deferred_${hnumber}_${mname}::iterator >  range = deferred_queue_${hnumber}_${mname}.equal_range( myTicket );
            Deferred_${hnumber}_$m->{name}::iterator it;
            for( it = range.first; it != range.second; it++ ){
              $serialize
                maptype_${hname}::$iterator iter = map_${hname}.find( it->second.$rid );
                if(iter == map_${hname}.end()) {
                    //maceWarn("No $hname registered with uid %"PRIi32" for upcall $mname!", it->second.$rid );
                    $body
                    }
                else {
                    iter->second->$callm;
                }
            }
            deferred_queue_${hnumber}_${mname}.erase( range.first , range.second );
        }
        #;
        $hnumber ++;
    }

    print $outfile <<END;

    //Model checking safety methods
    $modelCheckSafety

    //Model checking liveness methods
    $modelCheckLiveness

    } // end namespace

    //END Mace::Compiler::ServiceImpl::printCCFile
END
} # printCCFile

sub printConstantsFile {
    my $this = shift;
    my $outfile = shift;

    my $servicename = $this->name();

    my $r = Mace::Compiler::GeneratedOn::generatedOnHeader("$servicename constants header file");
    print $outfile $r;

    my $constantsIncludes = join("", $this->constants_includes());

    print $outfile <<END;
#ifndef ${servicename}_constants_h
#define ${servicename}_constants_h

#include "MaceBasics.h"
$constantsIncludes

    namespace ${servicename}_namespace {
END

    $this->printConstantsH($outfile);

    print $outfile <<END;

    } //end namespace ${servicename}_namespace

#endif // ${servicename}_constants_h
END
}

sub printConstantsH {
    my $this = shift;
    my $outfile = shift;

    my $servicename = $this->name();

    print $outfile <<END;
    //BEGIN: Mace::Compiler::ServiceImpl::printConstantsH
END

    foreach my $constant ($this->constants()) {
	my $conststr = $constant->toString();
	print $outfile <<END;
	    static const $conststr;
END
    }

    print $outfile <<END;
    //END: Mace::Compiler::ServiceImpl::printConstantsH
END
}

sub printSelectorConstantsH {
    my $this = shift;
    my $outfile = shift;

    my $servicename = $this->name();

    print $outfile <<END;
    //BEGIN: Mace::Compiler::ServiceImpl::printSelectorConstantsH
END

    while (my ($sv, $s) = each(%{$this->selectorVars()})) {
        #        const char selector_${sv}[] = $s;
        print $outfile <<END;
            const std::string selector_${sv} = $s;
END
    }

    print $outfile <<END;
    //END: Mace::Compiler::ServiceImpl::printSelectorConstantsH
END
}

sub printSelectorConstantsCC {
    my $this = shift;
    my $outfile = shift;

    my $servicename = $this->name();

    print $outfile <<END;
    //BEGIN: Mace::Compiler::ServiceImpl::printSelectorConstantsCC
END

    while (my ($sv, $s) = each(%{$this->selectorVars()})) {
	#        const char selector_${sv}[] = $s;
	print $outfile <<END;
	    //const std::string selector_${sv};
	    const LogIdSet* ${servicename}Service::selectorId_${sv};
END
    }

    print $outfile <<END;
    //END: Mace::Compiler::ServiceImpl::printSelectorConstantsCC
END
}

sub printIncludesH {
    my $this = shift;
    my $outfile = shift;

    my $servicename = $this->name();

    print $outfile <<END;
    //BEGIN: Mace::Compiler::ServiceImpl::printIncludesH
END

    my $undefCurtime = "";
    #  if ($this->macetime()) {
    #      $undefCurtime = '#undef curtime';
    #  }

    print $outfile <<END;
#include "lib/mace.h"
#include "lib/mace_constants.h"
#include "Enum.h"
//#include "lib/Util.h"
#include "lib/TimeUtil.h"
#include <map> //only include if can make upcalls
#include "lib/mace-macros.h"
    $undefCurtime
#include "lib/Log.h"
#include "lib/MethodMap.h" // only need this for sql logging of method calls
#include "lib/ScopedLog.h"
#include "lib/SimpleLogObject.h"
#include "lib/ScopedStackExecution.h"
#include "lib/BinaryLogObject.h"
#include "lib/Serializable.h"
#include "lib/ScopedFingerprint.h"
#include "${servicename}-constants.h"
#include "lib/ContextBaseClass.h"
#include "lib/ContextMapping.h"
#include "Event.h"
#include "lib/InternalMessage.h"
END

    if( $this->hasContexts() ){
      print $outfile <<END;
#include "lib/InternalMessageProcessor.h"

END
    }else{
      print $outfile <<END;
#include "lib/NullInternalMessageProcessor.h"

END
    }

    if ($this->macetime()) {
	print $outfile <<END;
#include "lib/MaceTime.h"
END
    }

    foreach my $scProvided ($this->provides()) {
	print $outfile <<END;
#include "${scProvided}ServiceClass.h"
END
    }
    foreach my $scUsed ($this->service_variables()) {
	print $outfile $scUsed->returnSCInclude();
    }
    if ($this->isDynamicRegistration() || grep($_->registration(), $this->service_variables())) {
        print $outfile qq{#include "DynamicRegistration.h"\n};
    }

    print $outfile <<END;
    //END: Mace::Compiler::ServiceImpl::printIncludesH
END
}

sub printUsingH {
    my $this = shift;
    my $outfile = shift;

    print $outfile <<END;
    //BEGIN: Mace::Compiler::ServiceImpl::printUsingH
           using mace::BinaryLogObject;
           using mace::Serializable;
           using mace::SerializationException;
           using mace::ContextMapping;
           using mace::__asyncExtraField;
           using mace::__ServiceStackEvent__;
           using mace::__ScopedRoutine__;
           using mace::__ScopedTransition__;
           using mace::__CheckRoutine__;
           using mace::__CheckTransition__;
           using mace::Message;
END

    if ($this->macetime()) {
	print $outfile <<END;
	using mace::MaceTime;
END
    }

    print $outfile <<END;
    //END: Mace::Compiler::ServiceImpl::printUsingH
END

}

sub printIncludeBufH {
    my $this = shift;
    my $outfile = shift;

    print $outfile <<END;
    //BEGIN: Mace::Compiler::ServiceImpl::printIncludeBufH
END

    print $outfile $this->header()."\n";

    print $outfile <<END;
    //END: Mace::Compiler::ServiceImpl::printIncludeBufH
END

}

sub printAutoTypeForwardDeclares {
    my $this = shift;
    my $outfile = shift;

    print $outfile <<END;
    //BEGIN: Mace::Compiler::ServiceImpl::printAutoTypeForwardDeclares
END

    foreach my $at ($this->auto_types()) {
	print $outfile "  ".$at->toForwardDeclare();
    }

    print $outfile <<END;
    //END: Mace::Compiler::ServiceImpl::printAutoTypeForwardDeclares
END

}

sub printTypeDefs {
    my $this = shift;
    my $outfile = shift;

    print $outfile <<END;
    //BEGIN: Mace::Compiler::ServiceImpl::printTypeDefs
END

    foreach my $td ($this->typedefs()) {
	print $outfile "  ".$td->toString()."\n";
    }

    print $outfile <<END;
    //END: Mace::Compiler::ServiceImpl::printTypeDefs
END

}

sub printRoutineObjects {
    my $this = shift;
    my $outfile = shift;

    print $outfile <<END;
    //BEGIN: Mace::Compiler::ServiceImpl::printRoutineObjects
END

    map { print $outfile $_->toString.";\n"; } $this->routineObjects();

    print $outfile <<END;
    //END: Mace::Compiler::ServiceImpl::printRoutineObjects
END

}

sub printAutoTypes {
    my $this = shift;
    my $outfile = shift;

    print $outfile <<END;
    //BEGIN: Mace::Compiler::ServiceImpl::printAutoTypes
END

    foreach my $at ($this->auto_types()) {
	print $outfile "  ".$at->toAutoTypeString(tracelevel=>$this->traceLevel())."\n";
    }

    print $outfile <<END;
    //END: Mace::Compiler::ServiceImpl::printAutoTypes
END

}

sub defineAutoTypeMethods {
    my $this = shift;
    my $outfile = shift;

    print $outfile <<END;
    //BEGIN: Mace::Compiler::ServiceImpl::defineAutoTypeMethods
END

    foreach my $at ($this->auto_types()) {
	print $outfile "  ".$at->defineAutoTypeMethods(scopedlog=>$this->traceLevel(), logparams=>$this->traceLevel())."\n";
    }

    print $outfile <<END;
    //END: Mace::Compiler::ServiceImpl::defineAutoTypeMethods
END

}

sub printMessages {
    my $this = shift;
    my $outfile = shift;

    print $outfile <<END;
    //BEGIN: Mace::Compiler::ServiceImpl::printMessages
END

    my $messagenum = 0;
    for my $at ($this->messages()) {
        $at->setNumber(\$messagenum);
    }

    for my $at (sort { $a->messageNum() <=> $b->messageNum() } $this->messages()) {
	print $outfile "  ".$at->toMessageString("")."\n";
    }

    print $outfile <<END;
    //END: Mace::Compiler::ServiceImpl::printMessages
END

}

sub printEventRequests {
    my $this = shift;
    my $outfile = shift;

    print $outfile <<END;
    //BEGIN: Mace::Compiler::ServiceImpl::printEventRequests
END

    my $messagenum = 0;
    foreach my $method ($this->asyncMethods(), $this->broadcastMethods(), $this->timerMethods(),  $this->usesHandlerMethods() , $this->providedHandlerMethods() , $this->providedMethods(), $this->routines() ){
      my $at = $method->options("serializer");
      next if not defined $at;
      $at->setRequestNumber(\$messagenum);
      print $outfile "  ". $at->toMessageString("")."\n";
    }

    print $outfile <<END;
    //END: Mace::Compiler::ServiceImpl::printEventRequests
END

}

sub printRoutineSerializer {
    my $this = shift;
    my $outfile = shift;

    print $outfile <<END;
    //BEGIN: Mace::Compiler::ServiceImpl::printRoutineSerializer
END

    my $messagenum = 0;
    my @serializers;
    for my $r (@{$this->routines()}) {
      my $serializer = $r->options("serializer");
        $serializer->setNumber(\$messagenum);
        push  @serializers, $serializer;
    }

    foreach my $at ( @serializers ){
      print $outfile "  ".$at->toMessageString("")."\n";
    }

    print $outfile <<END;
    //END: Mace::Compiler::ServiceImpl::printRoutineSerializer
END

}

sub printTimerClasses {
    my $this = shift;
    my $outfile = shift;

    print $outfile <<END;
    //BEGIN: Mace::Compiler::ServiceImpl::printTimerClasses
END

    foreach my $timer ($this->timers()) {
	print $outfile $timer->toString($this->name()."Service",
					traceLevel => $this->traceLevel())."\n";
    }

    print $outfile <<END;
    //END: Mace::Compiler::ServiceImpl::printTimerClasses
END

}

sub printContextClass {
    my $this = shift;
    my $outfile = shift;
    my $contexts = shift;

    my @subcontexts;
    map { push @subcontexts, $_->subcontexts() } @{$contexts};
    if( @subcontexts != 0 ){ # chuangw: print child contexts first because compiler is stupid.
        $this->printContextClass($outfile, \@subcontexts);
    }
    foreach my $context( @{$contexts} ) {
        print $outfile $context->toString($this->name()."Service",traceLevel => $this->traceLevel()) . "\n";
    }
}

sub printContextClasses {
    my $this = shift;
    my $outfile = shift;
    my $contexts = shift;

    print $outfile <<EOF;
    //BEGIN: Mace::Compiler::ServiceImpl::printContextClasses
EOF
    $this->printContextClass($outfile, $contexts );

    print $outfile <<EOF;
    //EOF: Mace::Compiler::ServiceImpl::printContextClasses
EOF
}

sub printTimerDummyClasses {
    my $this = shift;
    my $outfile = shift;

    print $outfile <<END;
    //BEGIN: Mace::Compiler::ServiceImpl::printTimerClasses
END

    foreach my $timer ($this->timers()) {
        print $outfile $timer->toStringDummy($this->name()."Dummy",
                                        traceLevel => $this->traceLevel())."\n";
    }

    print $outfile <<END;
    //END: Mace::Compiler::ServiceImpl::printTimerClasses
END

}


sub isDynamicRegistration {
    my $this = shift;
    return !(($this->registration() eq "") or
	     ($this->registration() eq "static") or
	     ($this->registration() eq "unique"));
}

sub matchStateChange {
    my $this = shift;
    my $l = shift;
    $$l =~ s|\bstate\s*=\s*(\w+)\s*;|state_change($1);|g;
    return $l;
}				# matchStateChange

sub getVarFromMethod {
    my $method = shift;
    my $msig = $method->toString(noreturn=>1, novirtual=>1, noid=>1, nodefaults=>1, noline=>1);
    $msig =~ s/\W/_/g;
    return $msig;
}

sub printDummyClass {
    my $this = shift;
    my @state_variables_cloned;
    foreach my $var ($this->state_variables()) {
        if($var->flags('serialize') || $var->flags('dump')){
           my $clonedVar = ref_clone($var);
           my $type = $clonedVar->type();
           $type->isRef(0);
           push(@state_variables_cloned, $clonedVar);
        }
    }

    my $outfile = shift;
    my $name = $this->name();
    my $servicename = $this->name();
    my $getStateNameCases = join("\n", map{qq/case $_: return "${servicename}::$_";/} $this->states());
    my $timerMethods .= join("\n", map{$_->toString().";"} $this->timerMethods());
    my $timerDeclares = join("\n", map{my $t = $_->name(); qq/ class ${t}_MaceTimer;\n${t}_MaceTimer &$t; /;} $this->timers());
    my $statestring = 'enum _state_type { '.join(',', $this->states())."};\ntypedef Enum<_state_type> state_type;\n";
    my $stateVariables = join("\n", map{$_->toString(nodefaults => 1).";"} $this->state_variables(), $this->onChangeVars()); #nonTimer -> state_var
    my $stateVariablesCloned = join("\n", map{$_->toString(nodefaults => 1).";"} @state_variables_cloned, $this->onChangeVars()); #nonTimer -> state_var
    my $registrationDeclares = join("\n", map{my $n = $_->name(); "typedef std::map<int, $n* > maptype_$n;
                                                                 maptype_$n map_$n;"} $this->providedHandlers);

    my $derives = join(", ", map{"public virtual $_"} (map{"${_}ServiceClass"} $this->provides() ), ($this->usesHandlers()) );

    my $registration = "";
    my $allocateRegistration = "";
    my $downcallRegistration = "";
    if ($this->isDynamicRegistration()) {
      my $regType = $this->registration();
      $registration = ", public virtual DynamicRegistrationServiceClass<$regType>";
      $allocateRegistration = "registration_uid_t allocateRegistration($regType const & object, registration_uid_t rid);
                               void freeRegistration(registration_uid_t, registration_uid_t);
                               bool getRegistration($regType & object, registration_uid_t regId);
                               ";
      $registrationDeclares .= "\nstd::map<registration_uid_t, $regType> _regMap;
                                  ";
    }
    if ($this->count_downcall_registrations()) {
        $downcallRegistration .= qq/void downcall_freeRegistration(registration_uid_t freeRid, registration_uid_t rid = -1);
        /;
        for my $r ($this->downcall_registrations()) {
            $downcallRegistration .= qq/registration_uid_t downcall_allocateRegistration($r const & object, registration_uid_t rid = -1);
            bool downcall_getRegistration($r & object, registration_uid_t rid = -1);
            /;
        }
    }


    print $outfile <<END;
    //BEGIN: Mace::Compiler::ServiceImpl::printDummyClass
END

    my $BaseService = "BaseMaceService";
    if( $Mace::Compiler::Globals::useFullContext ){
      $BaseService = "ContextService";
    }
    print $outfile <<END;
	class ${name}Dummy:public $BaseService, public BinaryLogObject, $derives
{
  private:
    int __inited;
    uint8_t instanceUniqueID;

  protected:
    $statestring
    static mace::LogNode* rootLogNode;

  public:
        //Constructor
        ${name}Dummy();
        //Destructor
        virtual ~${name}Dummy();

    const char* getStateName(state_type state) const {
      switch(static_cast<uint64_t>(state)) {
        $getStateNameCases
         default: ASSERT(false); return "INVALID STATE NUMBER";
      }
    }

    const std::string& getLogType() const {
	static std::string type = "${name}";
        return type;
    }

    const std::string& getLogDescription() const {
        static const std::string desc = "SNAPSHOT::${name}";
        return desc;
    }

    LogClass getLogClass() const {
        return STATE_DUMP;
    }

    mace::LogNode*& getChildRoot() const {
      return rootLogNode;
    }

  public:
    void print(std::ostream& __out) const;
    void printState(std::ostream& logger) const;
    void sqlize(mace::LogNode* node) const;
    void serialize(std::string& __str) const {};
    int deserialize(std::istream& is) throw(SerializationException);

    //State Variables
    $stateVariablesCloned

  protected:
    state_type _actual_state;
    const state_type& state;

    //Timer Vars
    $timerDeclares

    //Timer Methods
    $timerMethods
  };
END
}

sub shouldLogFuncsBody {
    my $this = shift;
    my $out = shift;
    for my $slog ($this->structuredLogs(),
		  grep(!($_->name =~ /^(un)?register.*Handler$/),
		       $this->providedMethods()),
		  @{$this->aspectMethods()}, @{$this->usesHandlerMethods()},
		  @{$this->timerMethods()},
		  @{$this->usesClassMethods()},
		  @{$this->providedHandlerMethods()},
		  $this->routines()) {
	print $out $slog->shouldLogFuncBody($this->name()."Service", $this->traceLevel());
    }
}

sub shouldLogFuncs {
    my $this = shift;
    my $ret = "";

    for my $slog ($this->structuredLogs(),
		  grep(!($_->name =~ /^(un)?register.*Handler$/),
		       $this->providedMethods()),
		  @{$this->aspectMethods()}, @{$this->usesHandlerMethods()},
		  @{$this->timerMethods()},
		  @{$this->usesClassMethods()},
		  @{$this->providedHandlerMethods()},
		  $this->routines()) {
	$ret .= $slog->shouldLogFunc();
    }
    return $ret;
}

sub printService {
    my $this = shift;
    my $outfile = shift;

    my $name = $this->name();
    my $statestring = 'enum _state_type { '.join(',', $this->states())."};\ntypedef Enum<_state_type> state_type;\n";
    my $transitionDeclarations = join("\n", map{$_->toTransitionDeclaration().";\n".$_->toGuardDeclaration().";"} $this->transitions());
    my $publicRoutineDeclarations = join("\n", map{$_->toString().";"} grep($_->isStatic, $this->routines()));
    my $routineDeclarations = join("\n", map{$_->toString().";"} grep(!$_->isStatic, $this->routines()));
    if ($this->traceLevel() > 1) {
      $routineDeclarations .= "\n".join("\n", grep(/./, map{if($_->returnType()->type() ne "void") { $_->toString(methodprefix=>"__mace_log_").";"}} $this->routines()));
    }
    my $hnumber = 1;
    my $defer_routineDeclarations = join("\n", map{"void ".$_->toString(noreturn=>1, methodprefix=>'defer_').";"} $this->routineDeferMethods());
    my $stateVariables = join("\n", map{$_->toString(nodefaults => 1, mutable => 1).";"} $this->state_variables(), $this->onChangeVars()); #nonTimer -> state_var
    my $providedMethodDeclares = join("\n", map{ $_->toString('nodefaults' => 1).";" } $this->providedMethodsAPI());
    my $usedHandlerDeclares = join("\n", map{$_->toString('nodefaults' => 1).";"} $this->usesHandlerMethodsAPI());
    my $serviceVars = join("\n", map{$_->toServiceVarDeclares()} $this->service_variables());
    my $constructorParams = join("\n", map{$_->toString('nodefaults' => 1).';'} $this->constructor_parameters());
    my $timerDeclares = join("\n", map{my $t = $_->name(); qq/ class ${t}_MaceTimer;\n${t}_MaceTimer &$t; /;} $this->timers());
    my $contextDeclares = join("\n", map{ $_->toDeclareString(); } ($this->contexts(),${ $this->contexts() }[0]->subcontexts() )  );
    my $timerMethods = join("\n", map{$_->toString().";"} $this->timerMethods());
    my $timerHelperMethods = join("\n", map{$_->toString().";"} $this->timerHelperMethods());
    my $asyncMethods = join("\n", map{$_->toString().";"} $this->asyncMethods());
    my $asyncHelperMethods = join("\n", map{$_->toString().";"} $this->asyncHelperMethods(), $this->asyncDispatchMethods(), $this->asyncLocalWrapperMethods());
    my $broadcastMethods = join("\n", map{$_->toString().";"} $this->broadcastMethods());
    my $broadcastHelperMethods = join("\n", map{$_->toString().";"} $this->broadcastHelperMethods(), $this->broadcastDispatchMethods(), $this->broadcastLocalWrapperMethods());
    
    my $contextHelperMethods = join("\n",  map{$_->toString().";"} $this->contextHelperMethods());
    my $routineHelperMethods = join("\n",  map{$_->toString().";"} $this->routineHelperMethods());
    my $providesSerialDeclares = join("\n", map{$_->toString("noid" => 0).";"} $this->providedMethodsSerials());
    my $usesHandlersSerialDeclares = join("\n", map{$_->toString("noid" => 0).";"} $this->usesHandlerMethodsSerials());
    my $downcallHelperMethods = join("\n", map{$_->toString("methodprefix"=>'downcall_', "noid" => 0, "novirtual" => 1).";"} $this->usesClassMethods());
    my $ctxdowncallHelperMethods = join("\n", map{$_->toString().";"} $this->downcallHelperMethods());
    my $defer_downcallHelperMethods = join("\n", map{"void ".$_->toString(noreturn=>1, methodprefix=>'defer_downcall_', noid => 0, novirtual => 1).";"} $this->downcallDeferMethods());
    my $upcallHelperMethods = join("\n", map{$_->toString('methodprefix'=>'upcall_', "noid" => 0, "novirtual" => 1).";"} $this->providedHandlerMethods());
    my $ctxupcallHelperMethods = join("\n", map{$_->toString().";"} $this->upcallHelperMethods());
    my $defer_upcallHelperMethods = join("\n", map{"void ".$_->toString(noreturn=>1,methodprefix=>'defer_upcall_', noid=> 0, novirtual => 1).";"} $this->upcallDeferMethods());
    my $derives = join(", ", map{"public virtual $_"} (map{"${_}ServiceClass"} $this->provides() ), ($this->usesHandlers()) );
    my $constructor = $name."Service(".join(", ", (map{$_->serviceclass."ServiceClass& __".$_->name} grep(not($_->intermediate()), $this->service_variables)), (map{$_->type->toString()." _".$_->name} $this->constructor_parameters()), "bool ___shared = true" ).");";
    $constructor .= "\n${name}Service(const ${name}Service& other);";
    my $accessorMethods = "";
    #my $accessorMethods = "const state_type& read_state() const;\n";
    #$accessorMethods .= join("\n", map { my $n = $_->name();if( $n =~ m/^__internal_/ ){ qq// }else { my $t = $_->type()->toString(paramconst => 1, paramref => 1); qq/ $t read_$n() const; / } } $this->state_variables());

    my $registrationDeclares = join("\n", map{my $n = $_->name(); "typedef std::map<int, $n* > maptype_$n;
                                                                 maptype_$n map_$n;
                                                                 std::set< int > apphandler_$n;
                                                                 "} $this->providedHandlers);
    my $changeTrackerDeclare = ($this->count_onChangeVars())?"class __ChangeTracker__;":"";
    my $changeTrackerFriend = ($this->count_onChangeVars())?"friend class __ChangeTracker__;":"";
    my $mergeDeclare = join("\n", map { "class $_;" } $this->mergeClasses());
    my $mergeFriend = join("\n", map { "friend class $_;" } $this->mergeClasses());
    my $autoTypeFriend = join("\n", map { my $n = $_->name(); "friend class $n;"} $this->auto_types());
    my $mergeMethods = $this->declareMergeMethods();
    my $onChangeDeclares = join("\n", map{$_->toString('nodefaults' => 1).";"} $this->aspectMethods());
    my $sLogs = join("\n", map {my $n = $_->toString(); "$n;"} $this->structuredLogs());
    my $shouldLogFuncs = $this->shouldLogFuncs();
    my $modelCheckSafety = join("\n", map{"// ${\$_->toString()}
                                         ${\$_->toMethodDeclare()}"    } $this->safetyProperties);
    my $modelCheckLiveness = join("\n", map{"// ${\$_->toString()}
                                           ${\$_->toMethodDeclare()}"    } $this->livenessProperties);
    my $callSafetyProperties = join("", map{"if(!${\$_->toMethodCall()}) { maceerr << \"Safety property ${\$_->name}: failed\" << Log::endl; description = \"${name}::${\$_->name}\"; return false; } else\n"} $this->safetyProperties()) . "{ maceout << \"Safety Properties: check\" << Log::endl; return true; }";
    my $callLivenessProperties = join("", map{"if(!${\$_->toMethodCall()}) { maceout << \"Liveness property ${\$_->name}: failed\" << Log::endl; description = \"${name}::${\$_->name}\"; return false; } else\n"} $this->livenessProperties()) . "{ maceout << \"Liveness Properties: check\" << Log::endl; return true; }";
    my $registration = "";
    my $allocateRegistration = "";
    my $downcallRegistration = "";
    if ($this->isDynamicRegistration()) {
      my $regType = $this->registration();
      $registration = ", public virtual DynamicRegistrationServiceClass<$regType>";
      $allocateRegistration = "registration_uid_t allocateRegistration($regType const & object, registration_uid_t rid);
                               void freeRegistration(registration_uid_t, registration_uid_t);
                               bool getRegistration($regType & object, registration_uid_t regId);
                               ";
      $registrationDeclares .= "\nstd::map<registration_uid_t, $regType> _regMap;
                                  ";
    }
    if ($this->count_downcall_registrations()) {
        $downcallRegistration .= qq/void downcall_freeRegistration(registration_uid_t freeRid, registration_uid_t rid = -1);
        /;
        for my $r ($this->downcall_registrations()) {
            $downcallRegistration .= qq/registration_uid_t downcall_allocateRegistration($r const & object, registration_uid_t rid = -1);
            bool downcall_getRegistration($r & object, registration_uid_t rid = -1);
            /;
        }
    }

    my $deferVars = "";
    for my $m ($this->upcallDeferMethods(), $this->downcallDeferMethods(), $this->routineDeferMethods()) {
        my $n = getVarFromMethod($m);
        $deferVars .= "mace::deque<__DeferralArgsFor${n}, mace::SoftState> __deferralArgList_${n};\n";
    }

    my $selectorIdInits = "";
    while (my ($sv, $s) = each(%{$this->selectorVars()})) {
	$selectorIdInits .= qq/
	    static const LogIdSet* selectorId_${sv}; /;
    }

    print $outfile <<END;
    //BEGIN: Mace::Compiler::ServiceImpl::printService
END

    my $BaseService = "BaseMaceService";
    if( $Mace::Compiler::Globals::useFullContext ){
      $BaseService = "ContextService";
    }

    my $typeInternalMessageProcessor;
    if( $this->hasContexts() ){
      $typeInternalMessageProcessor = "mace::InternalMessageProcessor";
    }else{
      $typeInternalMessageProcessor = "mace::NullInternalMessageProcessor";
    }

    print $outfile <<END;
    $changeTrackerDeclare
    class ServiceTester;
    class ${name}Service : public $BaseService, public virtual mace::PrintPrintable, public virtual Serializable, public virtual BinaryLogObject, $derives $registration
{
  private:
    $changeTrackerFriend
    friend class ServiceTester;
    $mergeFriend
    $autoTypeFriend
    int __inited;

    $typeInternalMessageProcessor improcessor;
  protected:
    $statestring
    static mace::LogNode* rootLogNode;
    mutable VersionServiceMap versionMap;

END

    $this->printComputeAddress($outfile);

    print $outfile <<END;
  public:
    //Constructor
    $constructor
    //Destructor
    virtual ~${name}Service();

    //Methods for snapshotting
    void snapshot(const uint64_t& ver) const;
    void snapshotRelease(const uint64_t& ver) const;

  private:

    $accessorMethods

  public:
    //Misc.

    const std::string& getLogType() const {
        static std::string type = "${name}";
        return type;
    }

    const std::string& getLogDescription() const {
        static const std::string desc = "SNAPSHOT::${name}";
        return desc;
    }

    LogClass getLogClass() const {
        return STATE_DUMP;
    }

    mace::LogNode*& getChildRoot() const {
      return rootLogNode;
    }

    $selectorIdInits

  protected:
    void dumpState(LOGLOGTYPE& logger) const;
    void processDeferred();
    const char* getStateName(state_type state) const;
    const char* getMessageName(uint8_t msgNum) const;
    void changeState(state_type stateNum, int selectorId);
    $shouldLogFuncs
    $sLogs
  public:
    void printNode(mace::PrintNode& __printer, const std::string& name) const;
    void print(std::ostream& logger) const;
    void printState(std::ostream& logger) const;
    void sqlize(mace::LogNode* node) const;

    void serialize(std::string& str) const;
    int deserialize(std::istream& is) throw(SerializationException);

    //Provided Methods (calls into this service from a higher layer)
    $providedMethodDeclares

    //Used Handlers (calls into this service from a lower layer)
    $usedHandlerDeclares

    //Registration Methods (for dynamic registration)
    $allocateRegistration

  protected:
    state_type _actual_state;
    const state_type& state;
    //XXX: Do we still need fsm_hint?
    //XXX: Generate utility_timer_ as needed.

    //Constructor Parameter (Variables)
    $constructorParams

    //ServiceVariables and Registration UIDs
    $serviceVars

    //Aspect Methods
    $onChangeDeclares

    //Registration Typedefs and Maps
    $registrationDeclares

    //State Variables
    $stateVariables

    //Timer Vars
    $timerDeclares

    //Context Declaration
    $contextDeclares

    //Timer Methods
    $timerMethods

    //Timer Helper Methods
    $timerHelperMethods

    //Async Methods
    $asyncMethods

    //Async Helper Methods
    $asyncHelperMethods

    //Broadcast Methods
    $broadcastMethods

    //Broadcast Helper Methods
    $broadcastHelperMethods

    //Context Helper Methods
    $contextHelperMethods

    //Routine Helper Methods
    $routineHelperMethods

    //Merge Class Declarations
    $mergeDeclare

    //Downcall and Upcall helper methods
    $downcallRegistration
    $downcallHelperMethods
    $upcallHelperMethods
    $ctxdowncallHelperMethods
    $ctxupcallHelperMethods

    //Serialized form Method Helpers
    $providesSerialDeclares
    $usesHandlersSerialDeclares

    //Transition and Guard Method Declarations
    $transitionDeclarations

    //Routines
    $routineDeclarations

    //Deferal Variables
    $deferVars

    //Local Address
    MaceKey __local_address;
    MaceKey downcall_localAddress() const;
    const MaceKey& downcall_localAddress(const registration_uid_t& rid) const;

    //Defer methods
    $defer_downcallHelperMethods
    $defer_upcallHelperMethods
    $defer_routineDeclarations

    //Pre and Post Transition Demux Method Declarations
    $mergeMethods

  public:
    $publicRoutineDeclarations

    static bool checkSafetyProperties(mace::string& description, const _NodeMap_& _nodes_, const _KeyMap_& _keys_) {
        ADD_SELECTORS("${name}::checkSafetyProperties");
        maceout << "Testing safety properties" << Log::endl;
        $callSafetyProperties
    }

    static bool checkLivenessProperties(mace::string& description, const _NodeMap_& _nodes_, const _KeyMap_& _keys_) {
        ADD_SELECTORS("${name}::checkLivenessProperties");
        maceout << "Testing liveness properties" << Log::endl;
        $callLivenessProperties
    }

  protected:
    static _NodeMap_::const_iterator castNode(const mace::MaceKey& key, const _NodeMap_& _nodes_, const _KeyMap_& _keys_) {
        ADD_SELECTORS("${name}::castNode::MaceKey");
        if (key.isNullAddress()) { return _nodes_.end(); }
        if (key.addressFamily() != IPV4) {
            macedbg(0) << "Casting " << key << Log::endl;
            if (_keys_.empty()) { return _nodes_.end(); macedbg(0) << "keys empty, end" << Log::endl; }
            if (key.addressFamily() != _keys_.begin()->first.addressFamily()) { return _nodes_.end(); macedbg(0) << "address family mismatch, end" << Log::endl; }
            _KeyMap_::const_iterator i = _keys_.find(key);
            if (i == _keys_.end()) { return _nodes_.end(); macedbg(0) << "key not found, end" << Log::endl; }
            macedbg(0) << "Returning node " << i->second << Log::endl;
            return _nodes_.find(i->second);
        }
        return _nodes_.find(key.getMaceAddr().local.addr-1);
    }

    static _NodeMap_::const_iterator castNode(const NodeSet::const_iterator& iter, const _NodeMap_& _nodes_, const _KeyMap_& _keys_) {
        if((*iter).isNullAddress()) { return _nodes_.end(); }
        return castNode(*iter, _nodes_, _keys_);
    }

    static _NodeMap_::const_iterator castNode(const mace::map<int, _NodeMap_::const_iterator, mace::SoftState>::const_iterator& iter, const _NodeMap_& _nodes_, const _KeyMap_& _keys_) {
        return iter->second;
    }

    static _NodeMap_::const_iterator castNode(const _NodeMap_::const_iterator& iter, const _NodeMap_& _nodes_, const _KeyMap_& _keys_) {
        return iter;
    }

    //Model Checking Safety Properties
    $modelCheckSafety

    //Model Checking Liveness Properties
    $modelCheckLiveness
    };
END

    print $outfile <<END;
//END: Mace::Compiler::ServiceImpl::printService
END

} # printService

sub varIsClosure {
    my $this = shift;
    my $var = shift;

    return ($var =~ /[^.()*]+\*/);
} # varIsClosure

sub splitVar {
    my $this = shift;
    my $var = shift;
    # NOTE: this will not match function pointers!
    $var =~ /([^.()*]+)(\*|(\([^.()]*\)(\s+const)?))?(\.([^.]*))?/;
    my $firstVar = $1;
    if (defined($3)) {
	$firstVar .= $3;
    }
    return ($firstVar, $6)
} # splitVar

sub compareName {
    my $this = shift;
    my $matchVar = shift;
    my $targetVar = shift;
    my $match = shift;

#    print("$targetVar $matchVar\n");
    if ($match) {
	return ($targetVar =~ /^$matchVar/i);
    }
    else {
	return lc($targetVar) eq lc($matchVar);
#	return ($targetVar =~ /^$matchVar$/i);
    }

} # compareName

sub computeLogObjects {
    my $this = shift;
    my $path = Mace::Util::findPath("mql", @Mace::Compiler::Globals::INCLUDE_PATH);
    if ($path eq "") {
	Mace::Compiler::Globals::error('bad path', $this->queryFile(), $this->queryLine, "Can't find mql in search path.  Make sure to add \"INCLUDE_DIRECTORY(<PATH_TO_MQL>)\" to \$MACE_HOME/services/CMakeLists-service.txt");
	return;
    }
    my $arg = $this->queryArg();
    $arg =~ s/\"/\\\"/g;
    my $output = `$path/mql -m \"$arg\"`;
    my $val = $? >> 8;
    if ($val != 0) {
	Mace::Compiler::Globals::error('mql error', $this->queryFile(),
				       $this->queryLine, "$output");
    }
    else {
	my @logObjs = split(/\n/, $output);
	my $pushWheres = 0;

	for my $var (@logObjs) {
	    if ($var eq "---") {
		$pushWheres = 1;
	    }
	    elsif (!$pushWheres) {
		$this->push_logObjects($var);
	    }
	    else {
		# split the line around the "-"
		# format is objectName - clause
		$var =~ /([^-]*) - (.*)/;
		my $existingWhere = $this->wheres->{$1};
		if (defined($existingWhere)) {
		    $this->wheres($1, "(".$existingWhere." || ".$2.")");
		}
		else {
		    $this->wheres($1, $2);
		}
	    }
	}
    }
}

sub validateLogObjects {
    my $this = shift;
    my @handlerMatches = grep(!($_->name =~ /^(un)?register.*Handler$/),
			      $this->providedMethods());
    # valid children for all log objects
    my %autoVars = (node => '1', time => '1', type => '1', tid => '1');

    if (!@{$this->logObjects()}) {
	# if there are no objects from queries, log everything
	for my $slog (@handlerMatches, @{$this->aspectMethods()},
		      @{$this->usesHandlerMethods()}, @{$this->timerMethods()},
		      @{$this->usesClassMethods()},
		      @{$this->providedHandlerMethods()}, @{$this->routines()}) {
	    # log all method calls
	    $slog->shouldLog(1);
	}
	# log the service
	$this->logService(1);

	for my $var ($this->state_variables()) {
	    # log all state variables
	    $var->shouldLog(1);
	}

	for my $slog ($this->structuredLogs()) {
	    # log all structured logs
	    $slog->shouldLog(1);
	}
	for my $at ($this->auto_types()) {
            my %registeredMethods;

	    # log all auto type methods
	    for my $m ($at->methods()) {
		$m->shouldLog(1);
		$m->doStructuredLog($m->getLogLevel($this->traceLevel()) > 0);
		my $logName = $m->squashedName();
                my $binlogname = $at->name() . "_" . $logName;
                if (defined($registeredMethods{$binlogname})) {
                    my $bn = $binlogname;
                    $binlogname = $binlogname . $registeredMethods{$binlogname};
                    $registeredMethods{$bn}++;
                }
                else {
                    $registeredMethods{$binlogname} = 2;
                }
		$m->options('binlogname', $binlogname);
		my $longName = $m->toString(nodefaults => 1, novirtual => 1, noreturn => 1, noline => 1);
		$longName =~ s/\n/ /g;
		$m->options('binloglongname', $longName);
	    }
	}
	for my $t ($this->timers()) {
	    # log all timer events
	    $t->shouldLog(1);
	}
	return;
    }
    for my $var ($this->logObjects()) {
	my $globalFind = 0;
	my @subVars = $this->splitVar($var);
#	print "first: $subVars[0] second $subVars[1]\n";
	my $closure = $this->varIsClosure($var);
	my $object = $subVars[0];
	my $varName = $subVars[1];

	if ($object eq "graph" && $varName eq "stack") {
	    # need to log all method calls
	    for my $slog (@handlerMatches, @{$this->aspectMethods()},
			  @{$this->usesHandlerMethods()}, @{$this->timerMethods()},
			  @{$this->usesClassMethods()},
			  @{$this->providedHandlerMethods()}, @{$this->routines()}) {
		my $longName = $slog->options('binloglongname');
		$slog->setLogOpts(1, $this->wheres->{$longName});
	    }
	    next;
	}
	if ($this->compareName($object, $this->name(), $closure)) {
	    # Matched service, check state variables
	    my $found = 0;
	    if (!defined($varName)) {
		# no child specified, add all state variables
		$found = 1;
		$globalFind = 1;
		$this->logService(1);
		for my $subVar ($this->state_variables()) {
		    $subVar->shouldLog(1);
		}
	    }
	    else {
		if ($autoVars{$varName}) {
		    $found = 1;
		    $globalFind = 1;
		    $this->logService(1);
		}
		else {
		    for my $subVar ($this->state_variables()) {
			if ($subVar->name() =~ /^$varName$/i) {
			    $found = 1;
			    $globalFind = 1;
			    $this->logService(1);
			    $subVar->shouldLog(1);
			    last;
			}
		    }
		}
	    }
	    if (!$found) {
		# signal error
		Mace::Compiler::Globals::error('invalid query',
					       $this->queryFile(),
					       $this->queryLine(),
					       "No state variable $varName in service $object");
	    }
	    else {
		if (!$closure) {
		    # we found a match, can't match anything else so go to next
		    # var
		    next;
		}
	    }
	}
	my $outerFound = 0;
	# Search structured log methods
	for my $slog ($this->structuredLogs()) {
	    if ($this->compareName($object, $slog->name(), $closure)) {
		# Matched a structured log
		$outerFound = 1;

		if (!defined($varName)) {
		    # no child specified, add all params
		    $globalFind = 1;
		    # this is set in validate
		    $slog->setLogOpts(1, $this->wheres->{$slog->name()});
		    for my $param ($slog->params()) {
			$param->shouldLog(1);
		    }
		    if (!$closure) {
			# we found a match, can't match anything else so go to
			# next var
			last;
		    }
		}
		else {
		    my $param = $slog->getParam($varName);
		    my $slogName = $slog->name();

		    if ($autoVars{$varName} || defined($param)) {
			$globalFind = 1;
			# this is set in validate
			$slog->setLogOpts(1, $this->wheres->{$slog->name()});
			if (defined($param)) {
			    $param->shouldLog(1);
			}
			if (!$closure) {
			    # we found a match, can't match anything else so go to
			    # next var
			    last;
			}
		    }
		    else {
			# signal error
			Mace::Compiler::Globals::error('invalid query', $this->queryFile(), $this->queryLine(), "No parameter named $varName in structured log $slogName");
			  next;
		    }
		}
	    }
	}
	if ($outerFound && !$closure) {
	    # go to next var
	    next;
	}
	$outerFound = 0;
	# Search function prototype methods
	for my $slog (@handlerMatches, @{$this->aspectMethods()},
		      @{$this->usesHandlerMethods()}, @{$this->timerMethods()},
		      @{$this->usesClassMethods()},
		      @{$this->providedHandlerMethods()}, @{$this->routines()}) {
	    my $longName = $slog->options('binloglongname');
	    if ($slog->doStructuredLog() &&
		$this->compareName($object, $longName, $closure)) {
		# Matched a function prototype
		$outerFound = 1;

		if (!defined($varName)) {
		    # no child specified, add all params
		    $globalFind = 1;
		    $slog->setLogOpts(1, $this->wheres->{$longName});
		    for my $param ($slog->params()) {
			$param->shouldLog(1);
		    }
		    if (!$closure) {
			# we found a match, can't match anything else so go to
			# next var
			last;
		    }
		}
		else {
		    my $param = $slog->getParam($varName);

		    if ($autoVars{$varName} || defined($param)) {
			$globalFind = 1;
			$slog->setLogOpts(1, $this->wheres->{$longName});
			if (defined($param)) {
			    $param->shouldLog(1);
			}
			if (!$closure) {
			    # we found a match, can't match anything else so go to
			    # next var
			    last;
			}
		    }
		    else {
			# signal error
			Mace::Compiler::Globals::error('invalid query', $this->queryFile(), $this->queryLine(), "No parameter named $varName in method $longName");
		    }
		}
	    }
	}
	if (!$globalFind) {
	    # nothing left to match, must be an error
	    Mace::Compiler::Globals::error('invalid query',
					   $this->queryFile(),
					   $this->queryLine(),
					   "No match for $var");
	    next;
	}
    }
} # validateLogObjects

sub createContextUtilHelpers {
    my $this = shift;

    my @helpers = (
        {
            return => {type=>"mace::ContextBaseClass*",const=>0,ref=>0},
            #param => [ {type=>"uint64_t",name=>"eventID", const=>1, ref=>0}, {type=>"mace::string",name=>"contextName", const=>1, ref=>1}, {type=>"uint32_t",name=>"contextID", const=>1, ref=>0} ],
            param => [ {type=>"mace::string",name=>"contextTypeName", const=>1, ref=>1} ],
            name => "createContextObject",
            body => "\n" . $this->generateCreateContextCode() . "\n",
        }
    );
    foreach( @helpers ){
        my $returnType = Mace::Compiler::Type->new(type=>$_->{return}->{type},isConst=>$_->{return}->{const},isConst1=>0,isConst2=>0,isRef=>$_->{return}->{ref});
        my @params;
        foreach( @{$_->{param} } ){
            my $type;
            if( defined $_->{const} ){
                $type = Mace::Compiler::Type->new(type=>$_->{type},isConst=>$_->{const},isConst2=>0,isRef=>$_->{ref});
            }elsif (defined $_->{const1} ){
                $type = Mace::Compiler::Type->new(type=>$_->{type},isConst1=>$_->{const1},isConst2=>0,isRef=>$_->{ref});
            }else{
                $type = Mace::Compiler::Type->new(type=>$_->{type},isRef=>$_->{ref});
            }
            my $field = Mace::Compiler::Param->new(name=>$_->{name}, type=>$type);
            if( defined $_->{default} ){
              $field->default( $_->{default} );
            }
            push @params, $field;
        }
        my $method = Mace::Compiler::Method->new(name=>$_->{name},  returnType=>$returnType, body=>$_->{body});
        if( defined $_->{flag} ){
            for ( @{ $_->{flag} } ){
                when ("methodconst") {
                  $method->isConst(1); 
                }
            }
        }
        $method->params(@params);

        $this->push_contextHelperMethods($method);
    }
}
sub validate_findRoutines {
    my $this = shift;
    my $ref_routineMessageNames = shift;
    my $uniqid = 1;
    for my $r ($this->routines()) {
        next if (defined $r->targetContextObject and $r->targetContextObject eq "__internal" );
        next if (defined $r->targetContextObject and $r->targetContextObject eq "__anon" );
        next if (defined $r->targetContextObject and $r->targetContextObject eq "__null" );
        $this->createContextRoutineHelperMethod( $r, $ref_routineMessageNames, $uniqid++  );
    }
}
sub processApplicationUpcalls {
    my $this = shift;
    # for each such upcall transition, create msg/auto_type using the parameters of the transition
    my $mcounter = 1;
    foreach ($this->providedHandlerMethods() ){
        $this->createApplicationUpcallInternalMessage( $_, $mcounter );
        $mcounter ++;
    }
}
sub createContextHelpers {
    my $this = shift;
    if( not $Mace::Compiler::Globals::useFullContext ){
        Mace::Compiler::Globals::error("bad_context", __FILE__, __LINE__ , "Failure recovery must be turned on.");
    }
    my @asyncMessageNames;
    my @syncMessageNames;

    $this->createAsyncExtraField();
    $this->createContextUtilHelpers();

    # TODO: support contexts for aspect/raw_upcall transition?
}
# chuangw: the local async dispatcher provides a centralized handler for same physical-node messages.
# The centralized handler provides type checking (via message id), which is safer than directly typecasting
# the void* pointer into a particular message object type.
#
# In essence, this is similar to the demux method of upcall_deliver( )

sub createRoutineDispatcher_Incontext {
    my $this = shift;

    return "";
}
sub createRoutineDispatcher_Fullcontext {
    my $this = shift;

    my $adWrapperBody = "
      mace::string returnValueStr;
      __beginRemoteMethod( __param->getEvent() );
      mace::vector<uint32_t> snapshotContextIDs; // empty vector

      Message *msg = static_cast< Message * >( __param );
      switch( msg->getType()  ){
    ";

    for my $m ( $this->routines(), $this->usesHandlerMethods(), $this->providedMethods() ) {
      my $msg = $m->options("serializer");
      next if( not defined $msg );
      # create wrapper func
      my $mname = $msg->{name};
      my $call = "";
      given( $msg->method_type ){
        when (Mace::Compiler::AutoType::FLAG_SYNC)        { $call = $m->toRoutineMessageHandler(  ) ; }
        when (Mace::Compiler::AutoType::FLAG_DOWNCALL)    { $call = "" ; }
        when (Mace::Compiler::AutoType::FLAG_UPCALL)      { $call = $m->toRoutineMessageHandler(  ) ; }
      }

      $adWrapperBody .= qq/
        case ${mname}::messageType: {
          $call
        }
        break;
      /;
    }
    $adWrapperBody .= qq/
        default:
          { ABORT("No matched message type is found" ); }
      }
      __finishRemoteMethodReturn(source, returnValueStr );
      delete __param;
    /;

    return $adWrapperBody;
}
sub createRoutineDispatcher {
    my $this = shift;

    my $adWrapperBody = "";
    if( $this->hasContexts() == 0 ){
      $adWrapperBody = $this->createRoutineDispatcher_Incontext();
    }else{
      $adWrapperBody = $this->createRoutineDispatcher_Fullcontext();
    }

    my $adWrapperName = "executeRoutine";
    my $adReturnType = Mace::Compiler::Type->new(type=>"void",isConst=>0,isConst1=>0,isConst2=>0,isRef=>0);
    my $adWrapperParamType = Mace::Compiler::Type->new( type => "mace::Routine_Message*", isConst => 0,isRef => 0 );
    my $adWrapperParam = Mace::Compiler::Param->new( name => "__param", type => $adWrapperParamType );

    my $adWrapperParamType2 = Mace::Compiler::Type->new( type => "mace::MaceAddr", isConst => 1,isRef => 1 );
    my $adWrapperParam2 = Mace::Compiler::Param->new( name => "source", type => $adWrapperParamType2 );

    my $adWrapperMethod = Mace::Compiler::Method->new( name => $adWrapperName, body => $adWrapperBody, returnType=> $adReturnType  );
    $adWrapperMethod->push_params( ( $adWrapperParam, $adWrapperParam2 ) );

    $this->push_asyncLocalWrapperMethods( $adWrapperMethod  );
}
sub createAsyncEventDispatcher {
    my $this = shift;
    my $adWrapperBody = "
    {
      __beginRemoteMethod( __param->getEvent() );
      __ScopedTransition__ st( this, __param->getExtra() );

      Message *msg = static_cast< Message * >( __param );
      switch( msg->getType()  ){
    ";

    PROCMSG: for my $m ( 
      ($this->asyncMethods(), $this->timerMethods(),  $this->usesHandlerMethods()  , $this->providedHandlerMethods(), $this->providedMethods() ) )
 {
      my $msg = $m->options("serializer");
      next if( not defined $msg );
      # create wrapper func
      my $mname = $msg->{name};
      my $call = "";
      given( $msg->method_type ){
        when (Mace::Compiler::AutoType::FLAG_ASYNC)       { $call = $this->asyncCallLocalHandler($msg );}
        when (Mace::Compiler::AutoType::FLAG_DELIVER)     { $call = $this->deliverUpcallLocalHandler( $msg ); }
        when (Mace::Compiler::AutoType::FLAG_TIMER)       { $call = $this->schedulerCallLocalHandler( $msg ); }
        when (Mace::Compiler::AutoType::FLAG_SYNC)        { next PROCMSG; } 
        when (Mace::Compiler::AutoType::FLAG_DOWNCALL)    { next PROCMSG; } 
        when (Mace::Compiler::AutoType::FLAG_UPCALL)      { next PROCMSG; }
        when (Mace::Compiler::AutoType::FLAG_APPUPCALL)   { next PROCMSG; }
      }

      $adWrapperBody .= qq/
        case ${mname}::messageType: {
          $call
        }
        break;
      /;
    }
    $adWrapperBody .= qq/
        default:
          { ABORT("No matched message type is found" ); }
      }
    }
    delete __param;
    /;

    my $adWrapperName = "executeAsyncEvent";
    my $adReturnType = Mace::Compiler::Type->new(type=>"void",isConst=>0,isConst1=>0,isConst2=>0,isRef=>0);
    my $adWrapperParamType = Mace::Compiler::Type->new( type => "mace::AsyncEvent_Message*", isConst => 0,isRef => 0 );
    my $adWrapperParam = Mace::Compiler::Param->new( name => "__param", type => $adWrapperParamType );
    my @adWrapperParams = ( $adWrapperParam );
        my $adWrapperMethod = Mace::Compiler::Method->new( name => $adWrapperName, body => $adWrapperBody, returnType=> $adReturnType, params => @adWrapperParams);
        $this->push_asyncLocalWrapperMethods( $adWrapperMethod  );

}
sub createBroadcastEventDispatcher {
    my $this = shift;
    my $adWrapperBody = "
    {
      __beginRemoteMethod( __param->getEvent() );
      __ScopedTransition__ st( this, __param->getExtra(), __ScopedTransition__::TYPE_BROADCAST_EVENT );

      Message *msg = static_cast< Message * >( __param );
      switch( msg->getType()  ){
    ";

    PROCMSG: for my $m ( 
      ($this->broadcastMethods, $this->timerMethods(),  $this->usesHandlerMethods()  , $this->providedHandlerMethods(), $this->providedMethods() ) )
 {
      my $msg = $m->options("serializer");
      next if( not defined $msg );
      # create wrapper func
      my $mname = $msg->{name};
      my $call = "";
      given( $msg->method_type ){
        when (Mace::Compiler::AutoType::FLAG_ASYNC)       { $call = $this->asyncCallLocalHandler($msg );}
        when (Mace::Compiler::AutoType::FLAG_BROADCAST)   { $call = $this->broadcastCallLocalHandler($msg);}
        when (Mace::Compiler::AutoType::FLAG_DELIVER)     { $call = $this->deliverUpcallLocalHandler( $msg ); }
        when (Mace::Compiler::AutoType::FLAG_TIMER)       { $call = $this->schedulerCallLocalHandler( $msg ); }
        when (Mace::Compiler::AutoType::FLAG_SYNC)        { next PROCMSG; } 
        when (Mace::Compiler::AutoType::FLAG_DOWNCALL)    { next PROCMSG; } 
        when (Mace::Compiler::AutoType::FLAG_UPCALL)      { next PROCMSG; }
        when (Mace::Compiler::AutoType::FLAG_APPUPCALL)   { next PROCMSG; }
      }

      $adWrapperBody .= qq/
        case ${mname}::messageType: {
          $call
        }
        break;
      /;
    }
    $adWrapperBody .= qq/
        default:
          { ABORT("No matched message type is found" ); }
      }
    }

    delete __param;
    /;

    my $adWrapperName = "executeBroadcastEvent";
    my $adReturnType = Mace::Compiler::Type->new(type=>"void",isConst=>0,isConst1=>0,isConst2=>0,isRef=>0);
    my $adWrapperParamType = Mace::Compiler::Type->new( type => "mace::AsyncEvent_Message*", isConst => 0,isRef => 0 );
    my $adWrapperParam = Mace::Compiler::Param->new( name => "__param", type => $adWrapperParamType );
    my @adWrapperParams = ( $adWrapperParam );
    my $adWrapperMethod = Mace::Compiler::Method->new( name => $adWrapperName, body => $adWrapperBody, returnType=> $adReturnType, params => @adWrapperParams);
    $this->push_broadcastLocalWrapperMethods( $adWrapperMethod  );

}
sub createDeferredApplicationUpcallDispatcher {
    my $this = shift;
    
    my $adWrapperBody = "
        uint8_t msgNum = message->getType();
        switch( msgNum ){
      ";
    for( $this->providedHandlerMethods()  ){
      # create wrapper func
      my $serializer = $_->options("serializer");
      next unless defined $serializer;
      my $mname = $serializer->name();
      my @param_name;
      my @declare_copy;

      my $retval = "";
      my $serialize_ret = "";
      if( not $_->returnType->isVoid() ){
        $retval = $_->returnType->type() . " ret = ";
        $serialize_ret = "mace::serialize(returnval, &ret );\n";
      }
      # if the passed argument is non-const reference, take special care.
      # in particular, create a local temporary to store the value and reference to that variable.
      map{ unless( $_->type->isConst() or $_->type->isConst1() or $_->type->isConst2() or not $_->type->isRef() ){
        push @param_name, $_->name;
        push @declare_copy, $_->type->type . " " . $_->name . " = _msg->" . $_->name;

        $serialize_ret .= "mace::serialize(returnval, &${ \$_->name() } );\n";
      }else{
        push @param_name, "_msg->" . $_->name;
      }} $_->params();
      
      my $upcall = "$retval upcall_${\$_->name()} ( " . join(",",  @param_name ) . " );";
      $adWrapperBody .= qq/
        case ${mname}::messageType: {
          ${mname}* const _msg = static_cast< ${mname}* const>( message );
          / . join("", map{$_ . ";\n"} @declare_copy ) . qq/
          $upcall
          $serialize_ret
        }
        break;
      /;
    }
    $adWrapperBody .= qq/
        default:
          { ABORT("No matched message type is found" ); }
      }
    /;

    my $adWrapperName = "executeDeferredUpcall";
    my $adReturnType = Mace::Compiler::Type->new(type=>"void",isConst=>0,isConst1=>0,isConst2=>0,isRef=>0);
    my $paramType1 = Mace::Compiler::Type->new( type => "mace::ApplicationUpcall_Message*", isConst => 1,isRef => 0 );
    my $param1 = Mace::Compiler::Param->new( name => "message", type => $paramType1 );
    my $paramType2 = Mace::Compiler::Type->new( type => "mace::string", isConst => 0,isRef => 1 );
    my $param2 = Mace::Compiler::Param->new( name => "returnval", type => $paramType2 );
    my @adWrapperParams = ( $param1, $param2 );
    my $adWrapperMethod = Mace::Compiler::Method->new( name => $adWrapperName, body => $adWrapperBody, returnType=> $adReturnType );
    $adWrapperMethod->push_params( @adWrapperParams );
    $this->push_asyncLocalWrapperMethods( $adWrapperMethod  );

}
sub createDeferredMessageDispatcher {
    my $this = shift;
    
    my $adWrapperBody ="";
        my $usesRouteDowncall = 0;
        map{ 
          if( $_->serialRemap() and $_->name eq "route"){
            $usesRouteDowncall = 1;
          }
        } $this->usesDowncalls();
    if( $this->useTransport() and $usesRouteDowncall ){
      $adWrapperBody = "
        uint8_t msgNum = Message::getType(messagestr);
        switch( msgNum ){
      ";
      for( $this->messages()  ){
        # create wrapper func
        my $mname = $_->{name};
        $adWrapperBody .= qq/
          case ${mname}::messageType: {
            $mname message;
            message.deserializeStr( messagestr );
            downcall_route ( dest, message, rid );
          }
          break;
        /;
      }
      $adWrapperBody .= qq/
          default:
            { ABORT("No matched message type is found" ); }
        }
      /;
    }

    my $adWrapperName = "dispatchDeferredMessages";
    my $adReturnType = Mace::Compiler::Type->new(type=>"void",isConst=>0,isConst1=>0,isConst2=>0,isRef=>0);
    my $paramType1 = Mace::Compiler::Type->new( type => "MaceKey", isConst => 1,isRef => 1 );
    my $paramType2 = Mace::Compiler::Type->new( type => "mace::string", isConst => 1,isRef => 1 );
    my $paramType3 = Mace::Compiler::Type->new( type => "registration_uid_t", isConst => 1,isRef => 0 );
    my $param1 = Mace::Compiler::Param->new( name => "dest", type => $paramType1 );
    my $param2 = Mace::Compiler::Param->new( name => "messagestr", type => $paramType2 );
    my $param3 = Mace::Compiler::Param->new( name => "rid", type => $paramType3 );
    my @adWrapperParams = ( $param1, $param2, $param3 );
    my $adWrapperMethod = Mace::Compiler::Method->new( name => $adWrapperName, body => $adWrapperBody, returnType=> $adReturnType );
    $adWrapperMethod->push_params( @adWrapperParams );
    $this->push_asyncLocalWrapperMethods( $adWrapperMethod  );

}

sub validate {
    my $this = shift;
    my @deferNames = @_;
    my $i = 0;

    if( $this->count_contexts() == 0 ){ # this is possible if state_variables block is not declared.
        my $context = Mace::Compiler::Context->new(name => "globalContext",className =>"global_Context", isArray => 0);
        my $contextParamType = Mace::Compiler::ContextParam->new(className => "" );
        $context->paramType( $contextParamType );

        my $commContext = Mace::Compiler::Context->new(name => "externalCommContext",className =>"externalComm_Context", isArray => 0);
        my $commContextParamType = Mace::Compiler::ContextParam->new(className => "" );
        $commContext->paramType( $commContextParamType );
        $context->push_subcontexts($commContext);
        $this->push_contexts($context);
    }

    $this->push_deferNames(@_);

    my $name = $this->name();
    my $attr = $this->attributes();
    if ($attr eq "") {
        $attr = $name;
    } else {
        $attr .= ",$name";
    }
    $this->attributes($attr);
    $Mace::Compiler::Globals::MACE_TIME = $this->macetime();

    $this->push_states('init');
    $this->push_states('exited');
    $this->push_ignores('hashState');
    $this->push_ignores('registerInstance');
    $this->push_ignores('maceReset');
    $this->push_ignores('getLogType');
    $this->push_ignores('localAddress');
    $this->push_ignores('requestContextMigration');
    $this->push_ignores('evict');

    for my $det ($this->detects()) {
        $det->validate($this);
    }

    for my $m ($this->routines()) {
        $m->validate( $this->contexts() ) ;
    }

    if ($this->queryFile() ne "") {
        $this->computeLogObjects();
    }
    my @providesMethods;
    my @providesHandlers;
    my @providesHandlersMethods;
    
    my @orig_usesClassMethods;
    my @orig_usesHandlerMethods;

    $this->validate_prepareSelectorTemplates();
    $this->validate_parseProvidedAPIs(\@providesMethods, \@providesHandlers, \@providesHandlersMethods); #Note: Provided APIs can update the impl.  (Mace literal blocks)
    my $transitionNum = 0;

    foreach my $transition ($this->transitions()) { # set a number for each transition
        $transition->transitionNum($transitionNum++);
    }
    $this->validate_parseUsedAPIs( \@orig_usesClassMethods, \@orig_usesHandlerMethods); #Note: Used APIs can update the impl.  (Mace literal blocks)

    #chuangw: I don't quite understand this, but: after validate_parseUsedAPIs(), the implementsUpcalls() array is populated with upcalls
    # defined in the service class.
    my $implementsDeliverHandler= 0;
    map{ 
      if( $_->serialRemap() and $_->name eq "deliver"){
        $implementsDeliverHandler= 1;
      }
    } $this->implementsUpcalls();
    
    my $usesRouteDowncall = 0;
    map{ 
      if( $_->serialRemap() and $_->name eq "route"){
        $usesRouteDowncall = 1;
      }
    } $this->usesDowncalls();

    $this->useTransport(1) if( $implementsDeliverHandler );

    $this->validate_fillStructuredLogs();

    $this->providedMethods(@providesMethods);
    $this->providedMethodsAPI(@providesMethods);

    #providedHandlers is the list of handler classes of the service classes we provide
    $this->providedHandlers(@providesHandlers  );

    # restore usesClassMethods and usesHandlerMethods
    $this->usesClassMethods( @orig_usesClassMethods );
    $this->usesHandlerMethods( @orig_usesHandlerMethods );

    # transform context'ed transitions into basic Mace transitions
    $this->createContextHelpers();

    $this->validate_findAsyncMethods();
    $this->validate_findBroadcastMethods();

    foreach my $message ($this->messages()) {
        $message->validateMessageOptions();
    }

    foreach my $autotype ($this->auto_types()) {
        $autotype->validateAutoTypeOptions();
#        print "VALIDATING autotype ".$autotype->name()." serialized? ".$autotype->serialize()."\n";
# should log all auto type methods
#       for my $slog ($autotype->methods()) {
#           $slog->doStructuredLog($slog->getLogLevel($this->traceLevel()) > 0);
#       }
    }

    foreach my $var ($this->state_variables()) {
        $var->validateTypeOptions({'serialize' => 0, 'recur' => 1});
        my $myhash = $var->type();
#        print $var->name(). " has type ".$var->type()->type()." and is marked for serialization? ".$var->flags('serialize') ."\n";
#        while ( my ($key, $value) = each(%$myhash) ) {
#            print "$key => $value\n";
#        }
    }

    foreach my $context ($this->contexts() ) {
        $context->validateContextOptions($this);
    }

    # create map autotypestr => autotypeobject
    my %autoTypeMap;
    foreach my $autotype ($this->auto_types()) {
        $autoTypeMap{$autotype->name()} = $autotype;
    }

    # updating state variable serialization flag based on whether or not the type is serializable
    # Specifically, if the type is an auto type, and the type is marked no serialize, then don't try
    foreach my $var ($this->state_variables()) {
        my $type = $var->type()->type();
        if(exists $autoTypeMap{$type} && $autoTypeMap{$type}->serialize() == 0){
            $var->flags('serialize', 0);
        }
    }


#    foreach my $var ($this->state_variables()) {
#        print $var->name(). " has type ".$var->type()->type()." and is marked for serialization? ".$var->flags('serialize') ."\n";
#    }

    for my $m ($this->constructor_parameters()) {
        $m->type()->set_isConst();
    }

    #This portion handles the method remappings block by removing pristine methods from state,
    #placing them in a remapping list, and replacing them with ones using remapped types
    $this->validate_genericMethodRemapping("usesDowncalls", "usesClassMethods", requireOPDefault=>0, allowRemapDefault=>1, pushOntoSerials=>0, includeMessageInSerial=>1);
    $this->validate_genericMethodRemapping("usesUpcalls", "providedHandlerMethods", requireOPDefault=>1, allowRemapDefault=>1, pushOntoSerials=>0, includeMessageInSerial=>1);

    $this->validate_genericMethodRemapping("implementsUpcalls", "usesHandlerMethods", requireOPDefault=>0, allowRemapDefault=>0, pushOntoSerials=>1, includeMessageInSerial=>0);
    $this->validate_genericMethodRemapping("implementsDowncalls", "providedMethods", requireOPDefault=>0, allowRemapDefault=>0, pushOntoSerials=>1, includeMessageInSerial=>0);

    $this->validate_setupSelectorOptions("routine", $this->routines());
    $this->validate_setupRoutines(\$i);


    my $multiTypeOption = Mace::Compiler::TypeOption->new(name=>"multi");
    $multiTypeOption->options('yes','yes');
    my @multiTypeOptions = ( $multiTypeOption );
    for my $mh (grep { ! ( $_->messageField() or ! $this->countDeferMatch("upcall_".$_->name())) } $this->providedHandlerMethods()) {
        $this->validate_setBinlogFlags($mh, \$i, "upcall_", $mh->getLogLevel($this->traceLevel()) > 0);
        $this->push_upcallDeferMethods($mh);
    }

    for my $mh (grep { ! ( $_->messageField() or ! $this->countDeferMatch("downcall_".$_->name())) } $this->usesClassMethods()) {
        $this->validate_setBinlogFlags($mh, \$i, "downcall_", $mh->getLogLevel($this->traceLevel()) > 0);
        $this->push_downcallDeferMethods($mh);
    }
    for my $mh (grep { ! ( $_->messageField() or ! $this->countDeferMatch($_->name()) ) } $this->routines()) {
        $this->push_routineDeferMethods($mh);
    }

    for my $timer ($this->timers()) {

        $timer->validateTypeOptions($this);
        my $v = Mace::Compiler::Type->new('type'=>'void');
        my $timerMethodName= "expire_".$timer->name;
        my $m = Mace::Compiler::Method->new('name' => $timerMethodName, 'body' => '{ }', 'returnType' => $v);
        my $i = 0;
        for my $t ($timer->types()) {
            my $dupet = ref_clone($t);
            $dupet->set_isRef();
            my $p = Mace::Compiler::Param->new(name=>"p$i", type=>$dupet);
            $m->push_params($p);
            $i++;
        }
        $m->options('timer' => $timer->name, 'timerRecur' => $timer->recur());

        $this->push_timerMethods($m);
    }

    #this code handles selectors and selectorVars for methods passed to demuxFunction
    $this->validate_setupSelectorOptions("demux", $this->usesHandlerMethods(), $this->providedMethods(), $this->timerMethods(), $this->implementsUpcalls(), $this->implementsDowncalls(), $this->asyncMethods(), $this->broadcastMethods() );

    #this code handles selectors and selectorVars for methods passed to demuxFunction
    $this->validate_setupSelectorOptions("upcall", $this->providedHandlerMethods(), $this->usesUpcalls(),$this->upcallHelperMethods() );

    #this code handles selectors and selectorVars for methods passed to demuxFunction
    $this->validate_setupSelectorOptions("downcall", $this->usesClassMethods(), $this->usesDowncalls(),$this->downcallHelperMethods() );

    #this code handles selectors and selectorVars for methods passed to demuxFunction
    $this->validate_setupSelectorOptions("async", $this->asyncHelperMethods(), $this->asyncDispatchMethods(), $this->asyncLocalWrapperMethods());
    
    #this code handles selectors and selectorVars for methods passed to demuxFunction
    $this->validate_setupSelectorOptions("broadcast", $this->broadcastHelperMethods(), $this->broadcastDispatchMethods(), $this->broadcastLocalWrapperMethods());

    $this->validate_setupSelectorOptions("context",  $this->contextHelperMethods());

    $this->validate_setupSelectorOptions("scheduler",  $this->timerHelperMethods());

    for my $method ($this->usesClassMethods(), $this->usesDowncalls()) {
        $this->validate_setBinlogFlags($method, \$i, "", $method->getLogLevel($this->traceLevel()) > 0);
    }

    #This portion validates that transitions match some legal API -- must determine list of timer names before this block.

    my $filepos = 0;
    foreach my $transition ($this->transitions()) {
        if ($transition->type() eq 'downcall') {
            $this->validate_fillTransition("downcall", $transition, \$filepos, $this->providedMethods());
        }
        elsif ($transition->type() eq 'upcall') {
            $this->validate_fillTransition("upcall", $transition, \$filepos, $this->usesHandlerMethods());
        }
        elsif ($transition->type() eq 'raw_upcall') {
            $this->validate_fillTransition("raw_upcall", $transition, \$filepos, $this->usesHandlerMethodsAPI());
        }
        elsif ($transition->type() eq 'scheduler') {
            $transition->method->name("expire_".$transition->method->name());
            $this->validate_fillTransition("scheduler", $transition, \$filepos, $this->timerMethods());
        }
        elsif ($transition->type() eq 'async') {
            $this->validate_fillTransition("async", $transition, \$filepos, $this->asyncMethods());
        }
        elsif ($transition->type() eq 'broadcast') {
            $this->validate_fillTransition("broadcast", $transition, \$filepos, $this->broadcastMethods());
        }
        elsif ($transition->type() eq 'aspect') {
            $this->validate_fillAspectTransition($transition, \$filepos);
        }
        else {
            my $ttype = $transition->type();
            Mace::Compiler::Globals::error("bad_transition", $transition->method()->filename(), $transition->method->line(), "Transition type '$ttype' invalid.  Expecting upcall/raw_upcall/downcall/scheduler/async/broadcast/sync/aspect");
        }
    }

    for my $p (@{$this->safetyProperties()}, @{$this->livenessProperties()}) {
        $p->validate($this);
    }
    $this->generateInternalTransitions();

    $this->validate_setupSelectorOptions("routine",  $this->routineHelperMethods());

    $this->validate_setupSelectorOptions("async", $this->asyncDispatchMethods);
    $this->validate_setupSelectorOptions("async", $this->asyncHelperMethods);
    $this->validate_setupSelectorOptions("async", $this->asyncLocalWrapperMethods);
    $this->validate_setupSelectorOptions("broadcast", $this->broadcastDispatchMethods);
    $this->validate_setupSelectorOptions("broadcast", $this->broadcastHelperMethods);
    $this->validate_setupSelectorOptions("broadcast", $this->broadcastLocalWrapperMethods);
    $this->validate_setupSelectorOptions("scheduler", $this->timerHelperMethods);
    $this->validate_setupSelectorOptions("upcall", $this->upcallHelperMethods);
    $this->validate_setupSelectorOptions("downcall", $this->downcallHelperMethods);


    $this->annotatedMacFile($this->annotatedMacFile() . substr($this->origMacFile(), $filepos));

    for my $slog (grep(!($_->name =~ /^(un)?register.*Handler$/), $this->providedMethods()), @{$this->aspectMethods()}, @{$this->usesHandlerMethods()}, @{$this->timerMethods()}) {
        $this->validate_setBinlogFlags($slog, \$i, "", ($slog->getLogLevel($this->traceLevel()) > 0 and (defined $slog->options('transitions') or scalar(grep {$_ eq $slog->name} $this->ignores()))));
    }
    $this->validateLogObjects();
} # validate

sub generateInternalTransitions{
  my $this = shift;
  my %messagesHash = ();
  my $uniqid = 0;

  $this->generateAsyncInternalTransitions( \$uniqid, \%messagesHash );
  $this->generateBroadcastInternalTransitions( \$uniqid, \%messagesHash );
  $this->generateSchedulerInternalTransitions( \$uniqid, \%messagesHash );
  $this->generateUpcallTransportDeliverInternalTransitions( \$uniqid, \%messagesHash );
  $this->generateUpcallInternalTransitions( \$uniqid, \%messagesHash );
  $this->generateDowncallInternalTransitions( \$uniqid, \%messagesHash );
  $this->generateAspectInternalTransitions( \$uniqid, \%messagesHash );
  $this->processApplicationUpcalls();


  my @syncMessageNames;
  $this->validate_findRoutines(\@syncMessageNames);

  $this->createAsyncEventDispatcher( );
  $this->createBroadcastEventDispatcher( );
  $this->createRoutineDispatcher( );
  $this->createMethodDeserializer( );

  $this->createDeferredApplicationUpcallDispatcher( );
  $this->createDeferredMessageDispatcher( );
}
sub createMethodDeserializer {
    my $this = shift;
    my $adWrapperBody = "
      uint8_t msgNum_s = static_cast<uint8_t>(is.peek() ) ;
      switch( msgNum_s  ){
    ";
    for my $m ( 
      $this->asyncMethods(), $this->broadcastMethods(), $this->timerMethods(),  
      $this->usesHandlerMethods() , # upcall transitions
      $this->providedMethods(),  # downcall transitions
      $this->providedHandlerMethods(),
      $this->routines()
       )
   {
      my $msg = $m->options("serializer");
      next if( not defined $msg );
      # create wrapper func
      my $mname = $msg->{name};

      $adWrapperBody .= qq!
        case ${mname}::messageType: {
          obj = new $mname;
          return mace::deserialize( is, obj );
        }
        break;
      !;
    }
    $adWrapperBody .= qq/
        default:
          { ABORT("No matched message type is found" ); }
      }
    /;

    my $adWrapperName = "deserializeMethod";
    my $adReturnType = Mace::Compiler::Type->new(type=>"int",isConst=>0,isConst1=>0,isConst2=>0,isRef=>0);

    my $stringstreamParamType = Mace::Compiler::Type->new( type => "std::istream", isConst => 0,isRef => 1 );
    my $stringstreamParam = Mace::Compiler::Param->new( name => "is", type => $stringstreamParamType );

    my $adWrapperParamType = Mace::Compiler::Type->new( type => "mace::Message*", isConst => 0,isRef => 1 );
    my $adWrapperParam = Mace::Compiler::Param->new( name => "obj", type => $adWrapperParamType );

    my @adWrapperParams = ( $stringstreamParam, $adWrapperParam );
    my $adWrapperMethod = Mace::Compiler::Method->new( name => $adWrapperName, body => $adWrapperBody, returnType=> $adReturnType);
    $adWrapperMethod->push_params( @adWrapperParams );
        $this->push_asyncLocalWrapperMethods( $adWrapperMethod  );

}
sub generateAsyncInternalTransitions {
  my $this = shift;
  
  my $ref_uniqid = shift;
  my $ref_messagesHash = shift;

  my $uniqid = $$ref_uniqid;
  foreach my $asyncMethod ( $this->asyncMethods() ){
    # TOdO: post-transitions and pre-transitions? What to do with the contexts? They are declared in interface, so they shouldn't know the internals.
    # TODO: validate all transitions of the same method access the same context
    my $at;
    my $adMethod;
    $asyncMethod->options("base_name", $asyncMethod->name() );
    $asyncMethod->options("async_msgname", $asyncMethod->toMessageTypeName("async",$uniqid ) );
    $asyncMethod->options("event_handler", $asyncMethod->toRealHandlerName("async",$uniqid ) );
    $asyncMethod->createAsyncMessage( $ref_messagesHash, \$at);
    my $helpermethod;
    $asyncMethod->createAsyncHelperMethod( $at, $this->asyncExtraField(), \$helpermethod );
    $at->validateMessageOptions();
    $asyncMethod->options("serializer", $at);
    $this->push_asyncHelperMethods($helpermethod);
    $asyncMethod->createTransitionWrapper( "async",  $at, $this->name(), $this->asyncExtraField() , \$adMethod);
    $this->push_asyncHelperMethods($adMethod);
    next if( not defined $asyncMethod->options("transitions") );
    $uniqid ++;
  }
}
sub generateBroadcastInternalTransitions {
  my $this = shift;
  
  my $ref_uniqid = shift;
  my $ref_messagesHash = shift;

  my $uniqid = $$ref_uniqid;
  foreach my $broadcastMethod ( $this->broadcastMethods() ){
    # TOdO: post-transitions and pre-transitions? What to do with the contexts? They are declared in interface, so they shouldn't know the internals.
    # TODO: validate all transitions of the same method access the same context
    my $at;
    my $adMethod;
    $broadcastMethod->options("base_name", $broadcastMethod->name() );
    $broadcastMethod->options("broadcast_msgname", $broadcastMethod->toMessageTypeName("broadcast",$uniqid ) );
    $broadcastMethod->options("event_handler", $broadcastMethod->toRealHandlerName("broadcast",$uniqid ) );
    $broadcastMethod->createBroadcastMessage( $ref_messagesHash, \$at);
    my $helpermethod;
    $broadcastMethod->createBroadcastHelperMethod( $at, $this->asyncExtraField(), \$helpermethod );
    $at->validateMessageOptions();
    $broadcastMethod->options("serializer", $at);
    $this->push_broadcastHelperMethods($helpermethod);
    $broadcastMethod->createTransitionWrapper( "broadcast",  $at, $this->name(), $this->asyncExtraField() , \$adMethod);
    $this->push_broadcastHelperMethods($adMethod);
    next if( not defined $broadcastMethod->options("transitions") );
    $uniqid ++;
  }
}
sub generateSchedulerInternalTransitions {
  my $this = shift;
  
  my $ref_uniqid = shift;
  my $ref_messagesHash = shift;

  my $uniqid = $$ref_uniqid;
  foreach my $schedulerMethod ( $this->timerMethods() ){
    my $at;
    my $adMethod;
    #my $adHeadMethod;
    my $basename = $schedulerMethod->name();
    $basename =~ s/^expire_//;
    $schedulerMethod->options("base_name", $basename );
    $schedulerMethod->options("scheduler_msgname", $schedulerMethod->toMessageTypeName("scheduler",$uniqid ) );
    $schedulerMethod->options("event_handler", $schedulerMethod->toRealHandlerName("scheduler",$uniqid ) );
    $schedulerMethod->createSchedulerMessage( $ref_messagesHash, \$at);
    
    $schedulerMethod->createTransitionWrapper( "scheduler",  $at, $this->name(), $this->asyncExtraField(), \$adMethod );
    $at->validateMessageOptions();
    $schedulerMethod->options("serializer", $at);
    $this->push_timerHelperMethods($adMethod);

    my $helpermethod = $schedulerMethod->createTimerHelperMethod($at, $this->asyncExtraField() ); # create scheduler_foo()
    $this->push_timerHelperMethods($helpermethod);
    next if( not defined $schedulerMethod->options("transitions") );
    $uniqid ++;
  }
}
sub generateUpcallTransportDeliverInternalTransitions {
  my $this = shift;
  
  my $ref_uniqid = shift;
  my $ref_messagesHash = shift;

  my $uniqid = $$ref_uniqid;

  return unless $this->useTransport() or $this->hasContexts();

  foreach my $upcallMethod ( grep { $_->name eq "deliver" } $this->usesHandlerMethods() ){
    #deal with transport upcall deliver handler
    my $at;
    my $adMethod;
    my $basename = $upcallMethod->name();

    # TODO: copy the $upcallMethod( deliver( src, dest, foo ) to __deliver( src, dest, foo )

    $upcallMethod->options("base_name", $basename );
    $upcallMethod->options("upcall_msgname", $upcallMethod->toMessageTypeName("upcall_deliver",$uniqid ) );
    $upcallMethod->options("event_handler", $upcallMethod->toRealHandlerName("upcall_deliver",$uniqid ) );
    my $service_messages = \@{ $this->messages() };
    $upcallMethod->createUpcallDeliverMessage( \$at, $service_messages );
    $upcallMethod->createTransitionWrapper( "upcall",  $at, $this->name(), $this->asyncExtraField(), \$adMethod );
    $at->validateMessageOptions();
    $upcallMethod->options("serializer", $at );
    $this->push_upcallHelperMethods($adMethod);

    $this->createUpcallMessageRedirectHandler( $upcallMethod );

    next if( not defined $upcallMethod->options("transitions") );
    $uniqid ++;
  }
}
sub generateUpcallInternalTransitions {
  my $this = shift;
  
  my $ref_uniqid = shift;
  my $ref_messagesHash = shift;

  my $uniqid = $$ref_uniqid;

  foreach my $upcallMethod ( grep { $_->name ne "deliver"  } $this->usesHandlerMethods() ){
    if( ($upcallMethod->name eq "error" or $upcallMethod->name eq "messageError" ) and not defined $upcallMethod->options('transitions') ){
# what to do if upcall_error() is defined?
      $upcallMethod->body( "
ThreadStructure::ScopedServiceInstance si( instanceUniqueID );
if( ThreadStructure::isOuterMostTransition() ){
  wasteTicket();
}
      " . $upcallMethod->body() );
    }else{
      $this->generateServiceCallTransitions("upcall", $upcallMethod, $uniqid );
    }
    $uniqid ++;
  }
}

sub generateDowncallInternalTransitions {
  my $this = shift;
  
  my $ref_uniqid = shift;
  my $ref_messagesHash = shift;

  my $uniqid = $$ref_uniqid;

  my @specialDowncalls = ("maceInit", "maceExit", "maceReset");

  for my $downcallMethod (grep(!($_->name =~ /^(un)?register.*Handler$/), $this->providedMethods())) {
    if( $downcallMethod->name eq "maceInit" ){

    }elsif( $downcallMethod->name eq "maceExit" ){

    }
    next if (scalar(grep {$_ eq $downcallMethod->name} $this->ignores() ));
    $this->generateServiceCallTransitions("downcall", $downcallMethod, $uniqid );
    $uniqid ++;
  }
}
sub generateServiceCallTransitions {
    my $this = shift;
    my $transitionType = shift;
    my $demuxMethod = shift;
    my $uniqid = shift;

    $demuxMethod->options("base_name", $demuxMethod->name() );
    $demuxMethod->options( $transitionType . "_msgname", $demuxMethod->toMessageTypeName($transitionType,$uniqid ) );
    $demuxMethod->options("event_handler", $demuxMethod->toRealHandlerName("upcall",$uniqid ) );


    my $helpermethod = Mace::Compiler::Method->new( name=>$demuxMethod->name(), returnType=>$demuxMethod->returnType, isVirtual=>$demuxMethod->isVirtual, isStatic=>$demuxMethod->isStatic, isConst=>$demuxMethod->isConst, throw=>$demuxMethod->throw, line=>$demuxMethod->line, filename=>$demuxMethod->filename, doStructuredLog=>$demuxMethod->doStructuredLog, shouldLog=>$demuxMethod->shouldLog, logClause=>$demuxMethod->logClause, isUsedVariablesParsed=>$demuxMethod->isUsedVariablesParsed, targetContextObject=>$demuxMethod->targetContextObject, body=>"" );
    $helpermethod->params( @{ $demuxMethod->params } ); 
    $helpermethod->usedStateVariables( @{ $demuxMethod->usedStateVariables } );

    if( defined $demuxMethod->options("transitions") ){
      # find the first transition, and match the parameter name
      my $orig_transition = ${ $demuxMethod->options("transitions") }[0];
      foreach my $nparam ( 0..( $orig_transition->method->count_params()  -1) ){
        my $orig_param = ${ $orig_transition->method->params() }[ $nparam ];
        my $helper_param = ${ $helpermethod->params() }[ $nparam ];
        if( $helper_param->name ne $orig_param->name ){
            $helper_param->name( $orig_param->name );
        }
      }
    }



    $demuxMethod->name("demux_${transitionType}_" . $demuxMethod->options("base_name") );
    if( $transitionType eq "downcall" ){
      $this->push_downcallHelperMethods( $helpermethod );
    }elsif( $transitionType eq "upcall" ){
      $this->push_upcallHelperMethods( $helpermethod );
    }
    my $at;
    my $routineMessageName = $demuxMethod->options( $transitionType . "_msgname" );
    $helpermethod->options("routine_name", $demuxMethod->name() );
    $demuxMethod->options("routine_name", $demuxMethod->name() );

    $demuxMethod->createContextRoutineMessage($transitionType, \$at, $routineMessageName);
    $at->validateMessageOptions();
    $demuxMethod->options("serializer", $at );

    my $methodName = "";
    $helpermethod->createContextRoutineHelperMethod( $transitionType,  $at, $routineMessageName, $this->hasContexts(), $this->name, $methodName );

    given( $demuxMethod->options("base_name") ){
      when (["maceInit", "maceReset", "maceExit"]){
        $this->generateSpecialTransitions( $helpermethod );
      }
      default { 
        if( not defined $demuxMethod->options('transitions') ){
          $helpermethod->body( "
ThreadStructure::ScopedServiceInstance si( instanceUniqueID );
if( ThreadStructure::isOuterMostTransition() ){
  wasteTicket();
}
" . $helpermethod->body() );
        }
      }
    }

}
sub generateSpecialTransitions {
    my $this = shift;
    my $helpermethod = shift;

    if( $helpermethod->name eq "maceInit" ){
        my $initServiceVars = join("\n", map{my $n = $_->name(); qq/
            _$n.maceInit();
            if ($n == -1) {
                $n  = NumberGen::Instance(NumberGen::HANDLER_UID)->GetVal();
            }
                                             /;
                                         } grep(not($_->intermediate()), $this->service_variables()));

        my $initResenderTimer = "";
        if($Mace::Compiler::Globals::useFullContext && $this->hasContexts() ){
            $initResenderTimer = "//resender_timer.schedule(params::get<uint32_t>(\"FIRST_RESEND_TIME\", 1000*1000) );";
        }
        my $registerHandlers = "";
        for my $sv ($this->service_variables()) {
            my $svn = $sv->name();
            for my $h ($this->usesHandlerNames($sv->serviceclass)) {
                if ($sv->doRegister($h)) {
                    if ($helpermethod->getLogLevel($this->traceLevel()) > 0) {
                        $registerHandlers .= qq{macecompiler(0) << "Registering handler with regId " << $svn << " and type $h for service variable $svn" << Log::endl;
                                            };
                    }
                    $registerHandlers .= qq{_$svn.registerHandler(($h&)*this, $svn, false);
                                        };
                }
            }
        }
        # chuangw: Only one physical node in a logical node will create STARTEVENT.
        my $apiBody = "
        if(__inited++ == 0) {
            //TODO: start utility timer as necessary
                registerInstanceID();
                improcessor.initChannel();
                contextStructure.constructContextStructure();

                $initServiceVars
                $initResenderTimer
                $registerHandlers
                if( mace::ContextMapping::getHead( contextMapping ) == Util::getMaceAddr() ){
                  $helpermethod->{body}
                }
        }";
      $helpermethod->body( $apiBody );
    }elsif ( $helpermethod->name eq "maceExit" ){
        my $stopTimers = join("\n", map{my $t = $_->name(); "$t.cancel();"} $this->timers());
        
        my $cleanupServices = "";
        for my $sv ($this->service_variables()) {
            my $svn = $sv->name();


            for my $h ($this->usesHandlerNames($sv->serviceclass)) {
                if ($sv->doRegister($h)) {
                    $cleanupServices .= qq{_$svn.unregisterHandler(($h&)*this, $svn);\n};
                }
            }

            if( not $sv->intermediate() ){
              $cleanupServices .= qq{_$svn.maceExit();\n};
            }

            # Before the internal transport channel is closed, the head notifies other internal nodes to exit, and 
            # these internal nodes respond the message.

        } # $this->service_variables()
        $cleanupServices .= qq/
            notifyHeadExit();
            improcessor.exitChannel();
        /;

        my $apiBody = "
        if(--__inited == 0) {

            if( mace::ContextMapping::getHead( contextMapping ) == Util::getMaceAddr() ){
              $helpermethod->{body}
            }

            //TODO: stop utility timer as necessary
            _actual_state = exited;
            $stopTimers
            $cleanupServices
        } ";

      $helpermethod->body( $apiBody );
    } elsif ( $helpermethod->name eq "maceReset" ){
        my $stopTimers = join("\n", map{my $t = $_->name(); "$t.cancel();"} $this->timers());
        my $resetServiceVars = join("\n", map{my $n = $_->name(); qq{_$n.maceReset();}} grep(not($_->intermediate()), $this->service_variables()));
        my $clearHandlers = "";
        for my $h ($this->providedHandlers()) {
            my $hname = $h->name();
            $clearHandlers .= "map_${hname}.clear();\n";
        }

        my $resetVars = "";
        for my $var ($this->state_variables(), $this->onChangeVars()) {
            if (!$var->flags("reset")) {
            next;
            }
            my $head = "";
            my $tail = "";
            my $init = $var->name();
            my $depth = 0;
            for my $size ($var->arraySizes()) {
            $head .= "for(int i$depth = 0; i$depth < $size; i$depth++) {\n";
            $init .= "[i$depth]";
            $tail .= "}\n";
            }
            $init .= " = " . $var->getDefault() . ";\n";
            $resetVars .= "$head $init $tail";
        }

        my $apiTail = "
            $helpermethod->{body}
            //TODO: stop utility timer as necessary
            _actual_state = init;
            $stopTimers
            $clearHandlers
            $resetServiceVars
            $resetVars
            __inited = 0;
            instanceUniqueID = 0;
            ";

        $helpermethod->body( $apiTail );
    }}
sub generateAspectInternalTransitions {
  my $this = shift;
  
  my $ref_uniqid = shift;
  my $ref_messagesHash = shift;

  my $uniqid = $$ref_uniqid;
  foreach my $aspectMethod ( $this->aspectMethods() ){

  }
}
sub createUpcallMessageRedirectHandler {
    my $this = shift;
    my $m = shift;

    # TODO: this is necessary only if Transport is used
    return if (not $this->useTransport() );
    my $msgtype = ${ $m->params }[2]->type->type;
    my $name = $this->name();
    my $ptype = $m->options("upcall_msgname");
    my $origmsg;
    my $redirectmsg;
    map { $origmsg = $_ if $_->name eq $msgtype }   $this->messages() ;
    $redirectmsg = $m->options("serializer");
    my @msgparams;
    my $param_source = ${ $m->params }[0]->name;
    my $param_destination = ${ $m->params }[1]->name;
    my $param_rid = ${ $m->params }[3]->name;

    my @params = ($param_source, $param_destination, $param_rid );
    my $param_msg = ${ $m->params }[2]->name;
    map { push @params, "$param_msg." . $_->name } @{ $origmsg->fields() };
    #push @params, "extra";
    #push @params, "dummyEvent";

    my $mt = $m->options('transitions');
    my %match_param;
    foreach my $mtransition ( @{ $mt } ){
      # TODO if there are multiple transitions of the same method, they must use exactly the same variable names
      for my $nparam ( 0 .. $mtransition->method()->count_params () -1 ){
        $match_param{ ${ $m->params() }[ $nparam ]->name } = ${ $mtransition->method()->params }[ $nparam ]->name;
      }
    }

    unshift @params, "mace::imsg";

    my $targetContextNameMapping = "";
    if( $m->targetContextObject() ){
      #my $ref_matched_params;
      $targetContextNameMapping = "oss<<" . join(qq/ << "." << /, $m->targetContextToString(\%match_param) ) . ";\n";
    }

    $m->validateLocking();
    my $locking = $m->options("locking");
    my $deliverRedirectBody;

    if( defined $m->options('transitions') ){
      my $contextToStringCode = $m->generateContextToString(\%match_param);
      $deliverRedirectBody = "
ThreadStructure::ScopedServiceInstance si( instanceUniqueID );
if( mace::Event::isExit ){
  wasteTicket();
  return;
}else if( ThreadStructure::isOuterMostTransition() ){
  /*$contextToStringCode*/
  std::ostringstream oss;
  $targetContextNameMapping  
  //mace::Event dummyEvent( static_cast<uint64_t>(0) );
  //__asyncExtraField extra(targetContextID, snapshotContextIDs, true);
  $ptype* mcopy = new $ptype( " . join(",", @params ) . " );
  mcopy->getExtra().targetContextID = oss.str();
  mcopy->getExtra().lockingType = $locking;
  mcopy->getExtra().methodName = \"$msgtype\";
  addTransportEventRequest( mcopy, source );

  return;
}
      ";
    }else{
      $deliverRedirectBody = "
ThreadStructure::ScopedServiceInstance si( instanceUniqueID );
if( ThreadStructure::isOuterMostTransition() || mace::Event::isExit ){
  wasteTicket();
}
";
    }
    $m->options("redirect", $deliverRedirectBody );
}
sub validate_findAsyncMethods {
    my $this = shift;
    my $name = $this->name();

    my $uniqid = 0;
    foreach my $transition ($this->transitions()) {
        if ($transition->type() eq 'async') {
            my $origmethod;
            unless(ref ($origmethod = Mace::Compiler::Method::containsTransition($transition->method, $this->asyncMethods()))) {
                my $pname = $transition->method->name;

                $origmethod = ref_clone($transition->method());
                my $v = Mace::Compiler::Type->new('type'=>'void');
                $origmethod->returnType($v);
                $origmethod->body("");
                $this->push_asyncMethods($origmethod);

            }
        }
    }

}

sub validate_findBroadcastMethods {
    my $this = shift;
    my $name = $this->name();

    my $uniqid = 0;
    foreach my $transition ($this->transitions()) {
        if ($transition->type() eq 'broadcast') {
            my $origmethod;
            unless(ref ($origmethod = Mace::Compiler::Method::containsTransition($transition->method, $this->broadcastMethods()))) {
                my $pname = $transition->method->name;

                $origmethod = ref_clone($transition->method());
                my $v = Mace::Compiler::Type->new('type'=>'void');
                $origmethod->returnType($v);
                $origmethod->body("");
                $this->push_broadcastMethods($origmethod);

            }
        }
    }

}

sub validate_fillStructuredLogs {
    my $this = shift;
    my $name = $this->name();

    for my $slog ($this->structuredLogs()) {
        if (scalar(grep($_->name() eq $slog->name(), $this->structuredLogs())) > 1) {
            Mace::Compiler::Globals::error("bad_structured_log", $slog->filename(), $slog->line(), "Structured log named ".$slog->name()." specified multiple times");
        }
        else {
            my $funcName = $slog->name();
            my $body = "{\nstd::string str;\nstatic int logId = Log::getId(\"${name}::$funcName\");\n";
            my @funcparams;
            for my $param ($slog->params()) {
                my $varName = $param->name();
                push @funcparams, $varName;
            }
            my $clause = $this->wheres->{$slog->name()};
            my $logName = $slog->name();
            $slog->options('binlogname', $logName);
            my $longName = $slog->toString(nodefaults => 1, novirtual => 1, noreturn => 1, noline => 1);
            $longName =~ s/\n/ /g;
            $slog->options('binloglongname', $longName);

            $body .= "if (mace::LogicalClock::instance().shouldLogPath()) {\n";
            if (defined($clause)) {
                $slog->setLogOpts(1, $clause);
            }
#               $body .= "if $clause {\n";
            my $paramList = $slog->paramsToString(notype => 1,
                                                  noline => 1,
                                                  nodefaults => 1);
            $body .= "if (shouldLog_$logName($paramList)) {\n";
#           }
            $body .= "Log::binaryLog(logId, new ${funcName}Dummy(".join(", ", @funcparams)."), 0);\n";
#           if (defined($clause)) {
            $body .= "}\n";
#           }
            $body .= "}\n}";
            $slog->body($body);
            $slog->returnType(Mace::Compiler::Type->new(type => "void"));
            $slog->isConst(1);
        }
        $slog->doStructuredLog($this->traceLevel() > 0);
    }
}

sub generateCreateContextCode {
  my $this = shift;

  my $condstr = join("", map{ $_->locateChildContextObj( ); } ${ $this->contexts() }[0]->subcontexts() );

  my $globalContextClassName = ${ $this->contexts()}[0]->className();
  return  qq@
  if( contextTypeName == mace::ContextMapping::GLOBAL_CONTEXT_NAME ){ // global context id
      ASSERT( globalContext == NULL );
      globalContext = new $globalContextClassName();
      return globalContext ;
  }
/*
  if( contextTypeName == mace::ContextMapping::EXTERNAL_COMM_CONTEXT_NAME ) { // external comm context
    return new externalComm_Context();
  }
*/
  $condstr

  ASSERT("Context type name not match!");
  return NULL;
  @;
}
=begin
sub generateCreateContextCode {
    my $this = shift;

    my $condstr= "";
    if( $this->hasContexts()  ){
        $condstr = "
        const mace::ContextMapping& snapshotMapping = contextMapping.getSnapshot(eventID);
        size_t ctxStrsLen = ctxStrs.size();\n";
    }
    $condstr .= join("else ", map{ $_->locateChildContextObj( 0, "this"); } ${ $this->contexts() }[0]->subcontexts() );

    my $globalContextClassName = ${ $this->contexts()}[0]->className();

    my $findContextStr = qq@
    ScopedLock sl(getContextObjectMutex);
    wakeupWaitingThreads( contextID );
    wakeupWaitingThreads( contextName );

    if( contextName.empty() ){ // global context id
        ASSERT( globalContext == NULL );
        globalContext = new $globalContextClassName(contextName, eventID, instanceUniqueID, contextID );
        setContextObject( globalContext, contextID, contextName );
        return globalContext ;
    }
    mace::string contextDebugID, contextDebugIDPrefix;
    std::vector<std::string> ctxStrs;
    boost::split(ctxStrs, contextName, boost::is_any_of("."), boost::token_compress_on);

    std::vector<std::string> ctxStr0;
    boost::split(ctxStr0, ctxStrs[0], boost::is_any_of("[,]") );
    $condstr
    ABORT( "createContextObject shouldn't reach here!");
    return NULL;
    @;

    return $findContextStr;
}
=cut

sub createRoutineDowngradeHelperMethod {
    my $this = shift;
    my $routine = shift;
    my $uniqid = shift;

    if( defined $routine->options('origtransition') ){
      my $helpermethod = $routine->createRoutineDowngradeHelperMethod();
      $this->push_routineHelperMethods($helpermethod);
    }
}

sub createContextRoutineHelperMethod {
    my $this = shift;
    my $routine = shift;
    my $routineMessageNames = shift;
    my $uniqid = shift;

    my $pname = $routine->name;
    my $returnType = $routine->returnType->type;
    my $routineMessageName = "__routine_at_${pname}_${uniqid}";

    my $helpermethod = ref_clone($routine);

    my $at;
    if( $this->hasContexts() > 0 ){
        push( @{$routineMessageNames}, $routineMessageName );
        $routine->createContextRoutineMessage("routine", \$at, $routineMessageName);
        $at->validateMessageOptions();
        $routine->options("serializer", $at);
    }
    $routine->createContextRoutineHelperMethod( "routine", $at, $routineMessageName, $this->hasContexts(),$this->name, $pname );
    $this->createRoutineDowngradeHelperMethod( $routine, $this->hasContexts, $uniqid);

    $helpermethod->name("routine_$pname");

    my @currentContextVars = ();
    $helpermethod->printTargetContextVar(\@currentContextVars, ${ $this->contexts() }[0] );
    $helpermethod->printSnapshotContextVar(\@currentContextVars, ${ $this->contexts() }[0] );

    my $contextAlias = join("\n", @currentContextVars);

    my $realBody = qq#{
        $contextAlias
        $helpermethod->{body}
    }
    #;
    $this->matchStateChange(\$realBody);
    $helpermethod->body( $realBody );
    $this->push_routineHelperMethods($helpermethod);
}
sub createAsyncExtraField {
    my $this = shift;

    # if there are no async transitions, don't do it.
    # create AutoType used by async calls. 
    for( $this->auto_types() ){
        if( $_->name() eq "__asyncExtraField" ){
          Mace::Compiler::Globals::error("reserved_autotypes", $_->filename() , $_->line(), "auto type '__asyncExtraField' is a reserved name.");
        }
    }
    my $contextIDType = Mace::Compiler::Type->new(type=>"mace::string",isConst=>0,isConst1=>0,isConst2=>0,isRef=>0);
    my $snapshotContextIDType = Mace::Compiler::Type->new(type=>"mace::set<mace::string>",isConst=>0,isConst1=>0,isConst2=>0,isRef=>0);
    my $eventType = Mace::Compiler::Type->new(type=>"mace::Event",isConst=>0,isConst1=>0,isConst2=>0,isRef=>0);

    my $targetContextField = Mace::Compiler::Param->new(name=>"targetContextID",  type=>$contextIDType);
    my $snapshotContextsField = Mace::Compiler::Param->new(name=>"snapshotContextIDs",  type=>$snapshotContextIDType);
    my $eventField = Mace::Compiler::Param->new(name=>"event",  type=>$eventType);

    my $asyncExtraField = Mace::Compiler::AutoType->new(name=> "__asyncExtraField", line=>__LINE__, filename => __FILE__ );

    $asyncExtraField->fields( ($targetContextField, $snapshotContextsField ) );
    # TODO: chuangw: move asyncExtraField to lib/ because all fullcontext services will use the same auto type.
    $this->asyncExtraField( $asyncExtraField );
}

sub validate_prepareSelectorTemplates {
    my $this = shift;
    my $name = $this->name();

    #Replace $service with the service name in parsed selector strings.
    while (my ($t,$selector) = each (%{$this->selectors()})) {
        $selector =~ s/\$service/$name/g;
        $this->selectors($t, $selector);
    }

    #If not set (should be the case!), provide default selectors.
    if (! exists($this->selectors->{'default'})) {
        $this->selectors('default', qq/"${name}::\$function::\$state"/);
    }
    ;
    if (! exists($this->selectors->{'message'})) {
        $this->selectors('message', qq/"${name}::\$function::\$message::\$state"/);
    }
    ;
}

sub validate_parseProvidedAPIs {
    my $this = shift;
    my $ref_providesMethods = shift;
    my $ref_providesHandlers = shift;
    my $ref_providesHandlersMethods = shift;

    my @provides = Mace::Compiler::ClassCache::getServiceClasses($this->provides());

    #Parse code snips from .mh files.  ProvidesHash prevents double including.
    my %providesHash;
    my %providesNamesHash;
    for my $p (@provides) {
        if ($p->maceLiteral() and not $providesHash{$p->maceLiteral()}) {
            $providesHash{$p->maceLiteral()} = 1;
            $Mace::Compiler::Grammar::text = $p->maceLiteral();
            $this->parser()->Update($p->maceLiteral(), 0, "type" => "provides");
        }
        my $pn = $p->name();
        $pn =~ s/ServiceClass//;
        if ($pn ne "") {
            $providesNamesHash{$pn} = 1;
        }
    }

    my @providesNames = keys(%providesNamesHash);
    $this->provides(@providesNames);

    #Flatten methods.  Only include virtual methods and ignore if it's name is getLogType
    @{ $ref_providesMethods } = map {$_->isVirtual(0); $_} (grep {$_->isVirtual() && $_->name() ne "getLogType"} Mace::Compiler::ClassCache::unionMethods(@provides));
    for my $m (@{ $ref_providesMethods} ) {
	if ($m->name eq "hashState") {
	    $m->body(qq{{
		static hash_string hasher;
		std::string s = toString();  
		macedbg(0) << s << Log::endl;
		return hasher(s);
	    }});
	}
        elsif ($m->name eq "localAddress") {
            #$m->body($this->localAddress());
            $m->options('trace','off');
            $m->body("\n{ return __local_address; }\n");
        } elsif ($m->name eq "requestContextMigration") {

    my $requestContextMigrationMethod= "";
    if( $this->hasContexts() ) {
        $requestContextMigrationMethod = qq#
requestContextMigrationCommon(serviceID, contextID, destNode, rootOnly);
        #;
    }else{
        $requestContextMigrationMethod = qq#
          maceerr<<"Single context service does not support migration."<<Log::endl;
          ABORT("Single context service does not support migration.");
        #;
    }
    my $lowerServiceMigrationRequest = join("\n", map{my $n = $_->name(); qq/
        _$n.requestContextMigration( serviceID, contextID, destNode, rootOnly); /;
     } grep( (not($_->intermediate()) and $_->serviceclass ne "Transport"), $this->service_variables()));
        $m->body( qq#
      if( serviceID == instanceUniqueID ){
        $requestContextMigrationMethod
      }else{
        $lowerServiceMigrationRequest
      }
      # );
        } elsif ($m->name eq "evict") {

        my $lowerServiceEvict = join("\n", map{my $n = $_->name(); qq/
            _$n.evict( ); /;
         } grep( (not($_->intermediate()) and $_->serviceclass ne "Transport"), $this->service_variables()));

          $m->body("\n{ 
if( mace::ContextMapping::getHead( contextMapping ) == Util::getMaceAddr() ){ // if head gets SIGTERM, initiates head migration 
  // TODO: decide a new node
  // let the new node to be prepared
  // --> notify the job scheduler. job scheduler tells the new head to stand by.


  // TODO: If there are other contexts shared on the head node, migrate them as well.

  return;
}


/*__event_evict e;
ASYNCDISPATCH( contextMapping.getHead() , __ctx_dispatcher, __event_evict , e );*/
$lowerServiceEvict
          }\n");
        }
#        elsif ($m->name eq "registerInstance") {
#            $m->options('trace','off');
#            my $registerInstance = join("\n", map{ qq/mace::ServiceFactory<${_}ServiceClass>::registerInstance("$name", this);/ } $this->provides());
#            $m->body("\n{\n $registerInstance \n}\n");
#        }
    }

    #Two copies, one for remapped methods, one for the original (and external) API
    #Provided Methods are the API methods of service classes we provide.
    $this->providedMethods(@{$ref_providesMethods});
    $this->providedMethodsAPI(@{$ref_providesMethods});

    #providedHandlers is the list of handler classes of the service classes we provide
    @{ $ref_providesHandlers } = Mace::Compiler::ClassCache::getHandlers($this->provides());
    $this->providedHandlers(@{ $ref_providesHandlers } );

    #providesHandlersMethods is the flattening of those methods
    @{ $ref_providesHandlersMethods } = Mace::Compiler::ClassCache::unionMethods(@{ $ref_providesHandlers} );
    $this->providedHandlerMethods(@{ $ref_providesHandlersMethods} );
}

sub validate_parseUsedAPIs {
    my $this = shift;
    my $orig_usesClassMethods = shift;
    my $orig_usesHandlerMethods = shift;

    my %svClassHash;
    my %svRegHash;
    my %usesHandlersMap = ();
    for my $sv ($this->service_variables()) {
        my $sc = $sv->serviceclass();
        unless($sv->intermediate()) {
            if ($sv->registration()) {
                $svRegHash{$sv->registration()}=1;
            }
            unless (defined($svClassHash{$sc})) {
                my @h = Mace::Compiler::ClassCache::getHandlers($sc);
                for my $h (@h) {
                    if ($sv->doRegister($h->name())) {
                        $usesHandlersMap{$h->name()} = $h;
                    }
                }
                $svClassHash{$sc}=1; # XXX: track handlers!
            }
        }
    }
    my @usesHandlers = values(%usesHandlersMap);
    $this->downcall_registrations(keys(%svRegHash));

    my @serviceVarClasses = keys(%svClassHash);

    my @uses = Mace::Compiler::ClassCache::getServiceClasses(@serviceVarClasses);
    my %usesHash;
    for my $u (@uses) {
        if ($u->maceLiteral() and not $usesHash{$u->maceLiteral()}) {
            $usesHash{$u->maceLiteral()} = 1;
            $Mace::Compiler::Grammar::text = $u->maceLiteral();
            $this->parser()->Update($u->maceLiteral(), 0, "type" => "services");
        }
    }

    $this->usesHandlers(map{$_->name} @usesHandlers);
    @{ $orig_usesHandlerMethods } = map {$_->isVirtual(0); $_} (grep {$_->isVirtual()} Mace::Compiler::ClassCache::unionMethods(@usesHandlers));
    $this->usesHandlerMethods( @{ $orig_usesHandlerMethods } );
    $this->usesHandlerMethodsAPI( @{ $orig_usesHandlerMethods } );

    #my @usesMethods = grep(!($_->name =~ /^((un)?register.*Handler)|(mace(Init|Exit|Resume|Reset))|localAddress|hashState|registerInstance|getLogType$/), Mace::Compiler::ClassCache::unionMethods(@uses));
    @{ $orig_usesClassMethods } = grep(!($_->name =~ /^((un)?register.*Handler)|(mace(Init|Exit|Resume|Reset))|localAddress|hashState|requestContextMigration|evict|registerInstance|getLogType$/), Mace::Compiler::ClassCache::unionMethods(@uses));
    $this->usesClassMethods( @{ $orig_usesClassMethods } );

    for my $sv (@serviceVarClasses) {
        my @svc = Mace::Compiler::ClassCache::getServiceClasses($sv);
        my @svcm = Mace::Compiler::ClassCache::unionMethods(@svc);
        my @svcn = map { $_->name(); } @svc;
        $this->usesClasses_push($sv, @svcm);
        my @svch = Mace::Compiler::ClassCache::getHandlers($sv);
        my @svchn = map { $_->name() } @svch;
        $this->usesHandlerNames_push($sv, @svchn);
    }


}

#This portion handles the method remappings block by removing pristine methods from state,
#placing them in a remapping list, and replacing them with ones using remapped types

sub validate_genericMethodRemapping {
    my $this = shift;
    my $methodset = shift;
    my $methodapiset = shift;
    my %args = @_;


    my $requireOPDefault = $args{"requireOPDefault"};
    my $allowRemapDefault = $args{"allowRemapDefault"};
    my $pushOntoSerials = $args{"pushOntoSerials"};
    my $includeMessageInSerial = $args{"includeMessageInSerial"};

    my $doGrep = 0;

    for my $method ($this->$methodset()) {
        my $origmethod;
        unless(ref ($origmethod = Mace::Compiler::Method::containsTransition($method, $this->$methodapiset()))) {
            if($method->options("mhdefaults")) {
              $method->options("delete", 1);
              $doGrep = 1;
            }
            else {
              Mace::Compiler::Globals::error("bad_method_remap", $method->filename(), $method->line(), $origmethod);
            }
            next;
        }
        if (defined $method->options('trace')) {
            $origmethod->options('trace', $method->options('trace'));
        }
        $method->returnType($origmethod->returnType);
        $method->body($origmethod->body);
        $method->isConst($origmethod->isConst);
        my @op = $origmethod->params();
        my $n = 0;
        for my $p ($method->params()) {
            my $op = shift(@op);
            unless($p->hasDefault) {
                $p->hasDefault($op->hasDefault);
                $p->default($op->default);
            }
            if ($allowRemapDefault and $p->hasDefault()) {
                if ($requireOPDefault and not $op->hasDefault()) {
                    my $pstr = $p->toString(noline => 1);
                    my $ostr = $op->toString(noline => 1);
                    Mace::Compiler::Globals::error("bad_method_remap", $p->filename(), $p->line(), "Cannot assign new default to arg $n [$pstr] because API [$ostr] has no default (see Mace/Compiler/ServiceImpl.pm [1])");
                    #[1] : Because we allow and regularly use non-const items as defaults
                    #here, we do not allow you to define default values for parameters
                    #which do not have default values in the API.  The actual
                    #implementation looks to see if the API default parameter is
                    #supplied, and if so, the default specified here is evaluated.  A
                    #"smarter" compiler could "trust" that you know what you are doing,
                    #and if there is no default, just use the one you supply (which will
                    #work as long as its a constant expression).
                    $n++;
                    next;
                }
                if (!$op->hasDefault() || ($p->default ne $op->default) ) {
                    $origmethod->options('remapDefault', 1);
                    $op->flags('remapDefault', $p->default);
                    if ($method->serialRemap) {
                        $method->options('remapDefault', 1);
                        $p->flags('remapDefault', $p->default);
                        $p->default($op->default);
                    }
                }
            }
            if ($p->name =~ /noname_(\d+)/) {
                $p->name($op->name);
            }
            $n++;
        }
        $method->push_params(@op);
        if ($method->serialRemap) {
            my @m = (grep { $origmethod != $_ } @{$this->$methodapiset()});
            if(scalar(@m)) {
                $this->$methodapiset(@m);
            } else {
                my $fn = "clear_".$methodapiset;
                $this->$fn();
            }
            my @serialForms;
            if ($includeMessageInSerial) {
              @serialForms = $method->getSerialForms("Message", map{$_->name()} $this->messages());
            } else{
              @serialForms = $method->getSerialForms(map{$_->name()} $this->messages());
            }
            map { $_->options('class', $origmethod->options('class')) } @serialForms;
            my $fn = "push_".$methodapiset;
            my $fnS = "push_".$methodapiset."Serials";
            $this->$fn(@serialForms);
            if ($pushOntoSerials) { $this->$fnS(@serialForms); }
        }
    }
    if($doGrep) {
      my @m = grep(!$_->options("delete"), @{$this->$methodset});
      if(scalar(@m)) {
        $this->$methodset(@m);
      } else {
        my $fn = "clear_".$methodset;
        $this->$fn();
      }
    }
}

sub manageSelectorString {
    my $this = shift;
    my $method = shift;
    my $type = shift;
    my $count = shift;

    if (! defined $method) {
        die("Huh?");
    }

    my $selector;
    if ($method->options('message')) {
        $selector = $this->selectors('message');
        my $msg = $method->options('message');
        $selector =~ s/\$message/$msg/g;
    } else {
        $selector = $this->selectors('default');
    }
    my $fnName = $method->name();
    $selector =~ s/\$function/$fnName/;
    $selector =~ s/\$state/($type)/;
    my $selectorVar = "${fnName}_$type";
    if ($count >= 0) {
        $selectorVar .= "_$count";
    }
    $this->selectorVars($selectorVar,$selector);
    $method->options('selector', $selector);
    $method->options('selectorVar', $selectorVar);
}

#This method creates the selector strings for a set of methods of a given type.
#It specifically sets the selector option (the selector string) and the
#selectorVar option (the variable name for the selector variables.
sub validate_setupSelectorOptions {
    my $this = shift;
    my $type = shift;

    my @methods = @_;

    my $typecount = 0;
    for my $method (@methods) {
        $this->manageSelectorString($method, $type, $typecount);
        $typecount++;
    }

}

sub validate_setBinlogFlags {
    my $this = shift;
    my $method = shift;
    my $i = shift;
    my $prefix = shift;
    my $logThresh = shift;

    $method->options('binlogname', "$prefix".$method->name."$$i");
    $method->options('binlogshortname', $method->name);
    my $longName = $method->toString(nodefaults => 1, novirtual => 1, noreturn => 1, noline => 1);
    $longName =~ s/\n/ /g;
    $method->options('binloglongname', $longName);
    $$i++;

    $method->doStructuredLog($logThresh);
    return 1;
}

sub validate_setupRoutines {
    my $this = shift;
    my $i = shift;


    for my $routine ($this->routines()) {
        my $t = $routine->body();
        #$this->matchStateChange(\$t);
        $routine->body($t);
        $routine->options('minLogLevel', 2);
        $this->validate_setBinlogFlags($routine, $i, "", $routine->getLogLevel($this->traceLevel()) > 1);
#        if ($transition->method->name() eq 'getLocalAddress' || $transition->method->name() eq 'localAddress') {
#            Mace::Compiler::Globals::error("bad_transition", $transition->method()->filename(), $transition->method->line(), "Mace now includes a local_address block in lieu of a transition.  You are no longer allowed to define a getLocalAddress transition.\n");
#        } else {
#            my $origmethod;
#            unless(ref ($origmethod = Mace::Compiler::Method::containsTransition($transition->method, $this->providedMethods()))) {
#               Mace::Compiler::Globals::error("bad_transition", $transition->method()->filename(), $transition->method->line(), $origmethod);
#               next;
#            }
#        }
    }
}

sub validate_processMatchedTransition {
    my $this = shift;
    my $transition = shift;
    my $filepos = shift;

    $transition->validate( $this->contexts()  ,$this->selectors());
    $this->selectorVars($transition->getSelectorVar(), $transition->getSelector());

    if(defined($transition->startFilePos()) and $transition->startFilePos() >= 0) {
        $this->annotatedMacFile($this->annotatedMacFile() . substr($this->origMacFile(), $$filepos, $transition->startFilePos()-$$filepos) . "//" . $transition->toString(noline=>1) . "\n" . (" " x ($transition->columnStart()-1)));
        $$filepos = $transition->startFilePos();
    }

    my $t = $transition->method->body();
    $this->matchStateChange(\$t);
    $transition->method->body($t);

    if ($transition->isOnce()) {
        my $once = "once_".$transition->getTransitionMethodName();
        $this->push_state_variables(Mace::Compiler::Param->new(name => $once,
                    type => Mace::Compiler::Type->new(type => "bool"),
                    hasDefault => 1,
                    default => "false"));
        #$transition->unshift_guards(Mace::Compiler::Guard->new(guardStr => "(!$once)"));
        $transition->unshift_guards(Mace::Compiler::Guard->new(type => "expr", guardStr => "(!$once)"));
    }
    if ($transition->isRaw()) {
        if (defined($this->rawTransitions($transition->name()))) {
            Mace::Compiler::Globals::error("bad_transition", $transition->method()->filename(), $transition->method->line(), "Duplicate raw transition for " . $transition->name());
        }
        $this->rawTransitions($transition->name(), $transition);
    }
}

sub validate_fillTransition {
    my $this = shift;
    my $kind = shift;
    my $transition = shift;
    my $filepos = shift;
    my @methodset = @_;

    # Check for localAddress
    if ($transition->type() eq 'downcall') {
        if ($transition->method->name() eq 'getLocalAddress' || $transition->method->name() eq 'localAddress') {
            Mace::Compiler::Globals::error("bad_transition(".$kind.")", $transition->method()->filename(), $transition->method->line(), "Mace now includes a local_address block in lieu of a transition.  You are no longer allowed to define a getLocalAddress transition.\n");
            return;
        }
    }

    # Check for other errors
    my $origmethod;
    unless(ref ($origmethod = Mace::Compiler::Method::containsTransition($transition->method, @methodset))) {
        Mace::Compiler::Globals::error("bad_transition(".$kind.")", $transition->method()->filename(), $transition->method->line(), $origmethod);
        return;
    }
    $this->fillTransition($transition, $origmethod);
    $this->validate_processMatchedTransition($transition, $filepos);
}

sub validate_fillAspectTransition {
    my $this = shift;
    my $transition = shift;
    my $filepos = shift;

    my $origmethod;
    $transition->method->name('aspect_'.$transition->method->name());
    unless(scalar(@{$transition->options('monitor')}) == scalar(@{$transition->method->params()})) {
        Mace::Compiler::Globals::error("bad_transition", $transition->method()->filename(), $transition->method->line(), "aspect transition parameter mismatch -- same parameters as those to monitor (different number)"); #comparing aspect template count against function count
        return;
    }
    my $err = 0;
    my $pi = 0;
    my $mvars = $transition->options('monitor');
    my @mpars;
    for my $monitorVar (@$mvars) {
#Find state variable for each montiored name
        my $stateType = Mace::Compiler::Type->new(type=>"state_type");
        my $stateParam = Mace::Compiler::Param->new(name=>"state", type=>$stateType);
        #my @svar = grep($monitorVar eq $_->name, @{$this->state_variables()}, $stateParam);
        my @svar = grep($monitorVar eq $_->name, @{ ${ $this->contexts() }[0]->ContextVariables() }, $stateParam);
        my $fsvar = $svar[0];
        if(scalar(@svar) != 1) {
            Mace::Compiler::Globals::error("bad_transition", $transition->method()->filename(), $transition->method->line(), "aspect transition: variable $monitorVar cannot be found in state variables");
            $err = 1;
            last;
        } else {
#put state variable on the monitor/shadow list
            push(@mpars, $fsvar);
            my @shvar = grep("_MA_".$transition->method->name()."_".$monitorVar eq $_->name, @{$this->onChangeVars()});
            unless(scalar(@shvar)) {
                my $sv = ref_clone($fsvar);
                $sv->name("_MA_".$transition->method->name()."_".$sv->name());
                $sv->flags("originalVar", $fsvar);
                $this->push_onChangeVars($sv);
            }
        }
        if($err) { $err=0; next; }
#test found state variable against type from the method
        if($transition->method->params()->[$pi]->type()) {
            if($transition->method->params()->[$pi]->type->type ne $fsvar->type->type) {
                Mace::Compiler::Globals::error("bad_transition", $transition->method()->filename(), $transition->method->line(), "aspect transition: state variable $monitorVar has type ".$fsvar->type->type." and does not match parameter $pi: ".$transition->method->params()->[$pi]->toString());
                next;
            }
            if(!$transition->method->params()->[$pi]->type->isConst() or !$transition->method->params()->[$pi]->type->isRef()) {
                Mace::Compiler::Globals::error("bad_transition", $transition->method()->filename(), $transition->method->line(), "aspect transition parameters must be const and ref parameters (parameter $pi: ".$transition->method->params()->[$pi]->toString().")");
                next;
            }
        }
        $pi++;
    }

    if($transition->method->returnType->toString()) {
        unless($transition->method->returnType->type()->isVoid()) {
            my $rtype = $transition->method->returnType->type();
            Mace::Compiler::Globals::error("bad_transition", $transition->method()->filename(), $transition->method->line(), "aspect transitions must return 'void', return type: $rtype");
            next;
        }
    } else {
        $transition->method->returnType(Mace::Compiler::Type->new(type => "void"));
    }
    unless(ref ($origmethod = Mace::Compiler::Method::containsTransition($transition->method, $this->aspectMethods()))) {
#add transition.
        my @othermatches;
        if(scalar(@othermatches = grep($_->name eq $transition->method->name, $this->aspectMethods))) {
            Mace::Compiler::Globals::error("bad_transition", $transition->method()->filename(), $transition->method->line(), "aspect transition ".$transition->method->toString()." does not match other aspects with the same name.  Options are:\n".join("\n", @othermatches->toString()));
            next;
        }
        $origmethod = ref_clone($transition->method);
        $origmethod->options('monitor', $transition->options('monitor'));
        $origmethod->body("{ }\n");
        my $i = 0;
        for my $p ($origmethod->params()) {
            unless($p->type()) {
                $p->type(ref_clone($mpars[$i]->type()));
                $p->type->isConst(1);
                $p->type->isRef(1);
            }
            $i++;
        }

        $this->manageSelectorString($origmethod, "demux", -1);

#$a->body("{ return; }"); #XXX What goes here?
# XXX: This allows mutiple aspects of same name with different
# parameter lists to be created, and one of each will execute.
# If that's bad we need to do some extra checking.
        $this->push_aspectMethods($origmethod);
    }
    $this->fillTransition($transition, $origmethod);
    $this->validate_processMatchedTransition($transition, $filepos);
}

sub countDeferMatch {
    my $this = shift;
    my $name = shift;

    return scalar(grep($_ eq $name, $this->deferNames()));
}


sub completeTransitionDefinition {
  my $this = shift;
  my $transition = shift;
  my $origmethod = shift;

  $transition->method->returnType($origmethod->returnType);
  $transition->method->isConst($origmethod->isConst);
  my $i = 0;
  for my $p ($transition->method->params()) {
    unless($p->type) {
      $p->type($origmethod->params()->[$i]->type());
    }
    $i++;
  }
}

sub fillTransition {
    my $this = shift;
    my $transition = shift;
    my $origmethod = shift;
    if ($transition->type() eq 'scheduler') {
        $transition->method->options('timer', $transition->method->name());
    }
    else {
        while (my ($k, $v) = each(%{$origmethod->options})) {
            $transition->method->options($k, $v);
        }
    }

    my $merge = $transition->getMergeType();
    my $lockingType = $transition->getLockingType();

    $this->completeTransitionDefinition($transition, $origmethod);

    $origmethod->targetContextObject( $transition->method->targetContextObject() );

    $origmethod->options($merge.'transitions', []) unless($origmethod->options($merge.'transitions'));

    push(@{$origmethod->options($merge.'transitions')}, $transition);
}

sub printStructuredLogs {
    my $this = shift;
    my $outfile = shift;
    my $name = $this->name();

    for my $slog ($this->structuredLogs()) {
        if ($slog->doStructuredLog()) {
            my $lname = $slog->name();
            print $outfile "mace::LogNode* ${lname}Dummy::rootLogNode = NULL;\n";
        }
        else {
            $slog->body("{ }\n");
        }
        my $func = $slog->toString(methodprefix => "${name}Service::", body => 1);
        print $outfile <<END;
        $func
END
    }
    for my $slog (grep(!($_->name =~ /^(un)?register.*Handler$/), $this->providedMethods()), @{$this->aspectMethods()}, @{$this->usesHandlerMethods()}, @{$this->timerMethods()}) {
#        if (not $slog->messageField() and $slog->getLogLevel($this->traceLevel()) > 0 and (defined $slog->options('transitions') or scalar(grep {$_ eq $slog->name} $this->ignores()))) {
        if ($slog->doStructuredLog()) {
#&& $slog->shouldLog()) {
            my $lname = (defined $slog->options('binlogname')) ? $slog->options('binlogname') : $slog->name();
            print $outfile "mace::LogNode* ${lname}Dummy::rootLogNode = NULL;\n";
        }
    }
    for my $slog (@{$this->usesClassMethods()}, @{$this->providedHandlerMethods()}) {
#        if (not $slog->messageField() and $slog->getLogLevel($this->traceLevel()) > 0) {
        if ($slog->doStructuredLog()) {
# && $slog->shouldLog()) {
            my $lname = (defined $slog->options('binlogname')) ? $slog->options('binlogname') : $slog->name();
            print $outfile "mace::LogNode* ${lname}Dummy::rootLogNode = NULL;\n";
        }
    }
    for my $slog ($this->routines()) {
#        if (not $slog->messageField() and $slog->getLogLevel($this->traceLevel()) > 1) {
        if ($slog->doStructuredLog()) {
# && $slog->shouldLog()) {
            my $lname = (defined $slog->options('binlogname')) ? $slog->options('binlogname') : $slog->name();
            print $outfile "mace::LogNode* ${lname}Dummy::rootLogNode = NULL;\n";
        }
    }
    for my $type ($this->auto_types()) {
        for my $m ($type->methods()) {
            if ($m->doStructuredLog()) {
                my $lname = $m->options('binlogname');
                print $outfile "mace::LogNode* ${lname}Dummy::rootLogNode = NULL;\n";
            }
        }
    }
}

sub printTransitions {
    my $this = shift;
    my $outfile = shift;

    my $name = $this->name();

    print $outfile "//BEGIN Mace::Compiler::ServiceImpl::printTransitions\n";

    for my $t ($this->transitions()) {
        my @currentContextVars = ();
        $t->method->printTargetContextVar(\@currentContextVars, ${ $this->contexts() }[0] );
        $t->method->printSnapshotContextVar(\@currentContextVars, ${ $this->contexts() }[0] );
          $t->contextVariablesAlias(join("\n", @currentContextVars));

        $t->printGuardFunction($outfile, $this, "methodprefix" => "${name}Service::", "serviceLocking" => $this->locking());

        #global state
        my @usedVar = array_unique($t->method()->usedStateVariables());

        my @declares = ();
        push(@declares, "// isUsedVariablesParsed = ".$t->method()->isUsedVariablesParsed()."\n");
        push(@declares, "// used variables within transition = @usedVar\n");
        push(@declares, "// Refer to ServiceImpl.pm:printTransitions()\n");
        push(@declares, "__eventContextType = ".$this->locking().";\n");

        $t->readStateVariable(join("\n", @declares));

        my $onChangeVarsRef = $this->onChangeVars();

        $t->printTransitionFunction($outfile, "methodprefix" => "${name}Service::", "onChangeVars" => $onChangeVarsRef, "serviceLocking" => $this->locking());
    }
    print $outfile "//END Mace::Compiler::ServiceImpl::printTransitions\n";
}


sub printMethodMapFile {
    my $this = shift;
    my $outFile = shift;
    my $slog = shift;
    my $isSlog = shift;

    if ($slog->doStructuredLog()) {
        if ($isSlog) {
            print $outFile "slog--";
        }
        print $outFile $slog->options('binloglongname')."--".$slog->options('binlogname')."--";
        if ($slog->logClause() ne "") {
            print $outFile $slog->logClause();
        }
        else {
            print $outFile $slog->shouldLog();
        }
        print $outFile "\n";
    }
}

sub printMethodMap {
    my $this = shift;
    my $outfile = shift;
    my $slog = shift;

    if ($slog->doStructuredLog()) {
        print $outfile "Log::binaryLog(mid, MethodMap_namespace::MethodMap(\"".$this->name()."_".$slog->options('binlogname')."\", \"".$slog->options('binloglongname')."\"), 0);\n";
    }
}

sub printRoutines {
    my $this = shift;
    my $outfile = shift;
    my $mmFileName = $this->name().".mm";
    my $methodMapFile;

    open($methodMapFile, ">", $mmFileName) or die;
    my $name = $this->name();

    print $outfile "//BEGIN Mace::Compiler::ServiceImpl::printRoutines\n";
    print $outfile join("\n", $this->generateAddDefer("", my $ref = $this->routineDeferMethods()));

    my $initSelectors = "";
    for my $sv (keys(%{$this->selectorVars()})) {
        $initSelectors .= qq/${name}Service::selectorId_${sv} = new LogIdSet(selector_${sv});
                         /;
    }

    print $outfile qq/
        static void initializeSelectors() {
          static bool _inited = false;
          if (!_inited) {
            log_id_t mid __attribute((unused)) = Log::getId("MethodMap");
            _inited = true;
            $initSelectors
            /;
            for my $slog (grep(!($_->name =~ /^(un)?register.*Handler$/), $this->providedMethods()), @{$this->aspectMethods()}, @{$this->usesHandlerMethods()}, @{$this->timerMethods()}) {
		$this->printMethodMap($outfile, $slog);
		$this->printMethodMapFile($methodMapFile, $slog, 0);
	    }
	    for my $slog (@{$this->usesClassMethods()}) {
		$this->printMethodMap($outfile, $slog);
		$this->printMethodMapFile($methodMapFile, $slog, 0);
	    }
	    for my $slog (@{$this->providedHandlerMethods()}) {
		$this->printMethodMap($outfile, $slog);
		$this->printMethodMapFile($methodMapFile, $slog, 0);
	    }
	    for my $r ($this->routines()) {
		$this->printMethodMap($outfile, $r);
		$this->printMethodMapFile($methodMapFile, $r, 0);
	    }
	    for my $slog ($this->structuredLogs()) {
		$this->printMethodMapFile($methodMapFile, $slog, 1);
	    }
	    for my $type ($this->auto_types()) {
		for my $m ($type->methods()) {
		    $this->printMethodMap($outfile, $m);
		    $this->printMethodMapFile($methodMapFile, $m, 0);
		}
	    }

    print $outfile qq/
          }
        }
    /;

    close $methodMapFile;

    my @routineMessageNames;
    for my $r ($this->routines()) {

        my $under = "";
        my $selectorVar = $r->options('selectorVar');
        my $shimroutine = "";
        my $routine = "";
        my $routineLogLevel = $r->getLogLevel($this->traceLevel());
        if ( not $r->returnType->isVoid and $routineLogLevel > 1) {
          my $fnNameSquashed = $r->squashedName();
          my $paramlist = $r->paramsToString(notype=>1, nodefaults=>1);
          my $rt = $r->returnType->toString();

	      my $type = "\"rv_" . Mace::Util::typeToTable($rt) . "\"";
          $shimroutine = $r->toString("methodprefix" => "${name}Service::", nodefaults => 1, nostatic => 1,
                                      selectorVar => 1, traceLevel => $this->traceLevel, binarylog => 1, initsel => 1,
                                      usebody => qq/
                return mace::logVal(__mace_log_$fnNameSquashed($paramlist), selectorId->compiler, $type);
                    /);
          $under = "__mace_log_";
          $routine = $r->toString("methodprefix" => "${name}Service::${under}", nodefaults => 1, nostatic => 1, selectorVar => 1, nologs => 1, prepare => 1, body => 1);
        }
        else {
          $routine = $r->toString("methodprefix" => "${name}Service::${under}", nodefaults => 1, nostatic => 1, selectorVar => 1, prepare => 1, traceLevel => $this->traceLevel, binarylog => 1, initsel => 1, body => 1);
        }
	print $outfile <<END
            $shimroutine
	    $routine
END
    }
    print $outfile "//END Mace::Compiler::ServiceImpl::printRoutines\n";
} # printRoutines


sub asyncCallLocalHandler {
  my $this = shift;
  my $message = shift;
  
  my $name = $this->name();
  my $msgname = $message->name();
  my $event_handler = $this->asyncCallHandler( $message->name() );
  my $event_head_handler = $message->name(); 
  $event_head_handler =~ s/^__async_at/__async_head_fn/;

  return 
"$msgname* __msg = static_cast< $msgname *>( msg ) ;
$event_handler ( *__msg );";
}

sub broadcastCallLocalHandler {
  my $this = shift;
  my $message = shift;
  
  my $name = $this->name();
  my $msgname = $message->name();
  my $event_handler = $this->broadcastCallHandler( $message->name() );
  my $event_head_handler = $message->name(); 
  $event_head_handler =~ s/^__broadcast_at/__broadcast_head_fn/;

  return 
    "$msgname* __msg = static_cast< $msgname *>( msg ) ;
    $event_handler ( *__msg );";
}

sub schedulerCallLocalHandler {
    my $this = shift;
    my $msg = shift;

    my $name = $this->name();

    my $msgname = $msg->name();
    my $event_handler = $msgname;
    $event_handler =~ s/^__scheduler_at/__scheduler_fn/;

    my $deliverBody = 
"$msgname* __msg = static_cast< $msgname *>( msg ) ;
$event_handler ( *__msg );";

    return $deliverBody;
}
sub deliverUpcallLocalHandler {
  my $this = shift;
  my $message = shift;


  my $methodIdentifier = "[_a-zA-Z][_a-zA-Z0-9]*";
  my $msgname = $message->name();
  my $pname;
  my $messageName = $message->name();
  if($messageName =~ /__deliver_at_($methodIdentifier)/){
      $pname = $1;
  }else{
      Mace::Compiler::Globals::error('upcall error', __FILE__, __LINE__, "can't find the upcall deliver handler using message name '$messageName'");
  }
  my $event_handler = "__deliver_fn_$pname";

  my $deliverBody = 
"$msgname* __msg = static_cast< $msgname *>( msg ) ;
$event_handler ( *__msg );";

  return $deliverBody;
}
sub demuxMethod {
    my $this = shift;
    my $outfile = shift;
    my $m = shift;
    my $transitionType = shift;
    my $name = $this->name();

    my $locking = -1;
    if (defined ($m->options("transitions"))) {
        my $t = $this->checkTransitionLocking($m, "transitions");
        $locking = ($locking > $t ? $locking : $t);
    }
    if (defined ($m->options("pretransitions"))) {
        my $t = $this->checkTransitionLocking($m, "pretransitions");
        $locking = ($locking > $t ? $locking : $t);
    }
    if (defined ($m->options("posttransitions"))) {
        my $t = $this->checkTransitionLocking($m, "posttransitions");
        $locking = ($locking > $t ? $locking : $t);
    }

    if ( $m->name eq 'maceInit' ||
         $m->name eq 'maceExit' ||
         $m->name eq 'hashState' ||
	 $m->name eq 'registerInstance' ||
         $m->name eq 'maceReset') {
        # Exclusive locking if the transition is of these types, regardless of any other specification.
        $locking = 1;
    }

    # Demux locking should borrow from its

    #print STDERR "[ServiceImpl.pm demuxMethod()]            " . $m->name . "  locking = " . $locking."\n";

    my $apiBody = "";

    if( defined $m->options("redirect") ){
      $apiBody .= $m->options("redirect");
    }

    if (defined($m->options("pretransitions")) || defined($m->options("posttransitions"))) {
	$apiBody .= "Merge_" . $m->options("selectorVar") . " __merge(" .
	    join(", ", ("this", map{$_->name()} $m->params())) . ");\n";
    }
    if (defined $m->options('transitions')) {
        $apiBody .= qq/ if(state == exited) {
            ${\$m->body()}
        } else
            /;
        $apiBody .= $this->checkGuardFireTransition($m, "transitions", "else");

        #TODO: Fell Through No Processing
    } elsif (!scalar(grep {$_ eq $m->name} $this->ignores() )) {
        my $tname = $m->name;
        if($transitionType eq "scheduler") {
          $tname = substr($tname, 7);
        }
        my @messages = ();
        for my $p ($m->params()) {
            if ($p->flags("message")) {
            push(@messages, $p->type()->type());
            }
        }
        if (scalar(@messages)) {
            $tname .= "(" . join(",", @messages) . ")";
        }

        Mace::Compiler::Globals::warning('undefined', $this->transitionEndFile(), $this->transitionEnd(), "Transition $transitionType ".$tname." not defined!", $this->transitionEndFile());
            $this->annotatedMacFile($this->annotatedMacFile . "\n//$transitionType ".$m->toString(noline=>1, nodefaults=>1, methodname=>$tname)." {\n//ABORT(\"Not Implemented\");\n// }\n");
        my $mn = $m->name;
        if ($m->getLogLevel($this->traceLevel()) > 0) {
            $apiBody .= qq{macecompiler(1) << "COMPILER RUNTIME NOTICE: $mn called, but not implemented" << Log::endl;\n};
        }
    }
    my $resched = "";

    if ($m->options('timer')) {
        my $timer = $m->options('timer');
        #TODO Pip Stuff
        if ($m->options('timerRecur')) {
            my $recur = $m->options('timerRecur');
            my $pstring = join("", map{", ".$_->name} $m->params());
            $resched .= qq~ $timer.reschedule($recur$pstring);
                  ~;
        }
    }
    $apiBody .= "{\n";
    if ($m->getLogLevel($this->traceLevel()) > 0 and !scalar(grep {$_ eq $m->name} $this->ignores() )) {
        $apiBody .= qq!macecompiler(1) << "RUNTIME NOTICE: no transition fired" << Log::endl;
        !;
    }
    $apiBody .= $resched .  $m->body() . "\n}\n";

    my $methodprefix = "${name}Service::";
    print $outfile $m->toString(methodprefix => $methodprefix, nodefaults => 1, prepare => 1,
                               selectorVar => 1, traceLevel => $this->traceLevel(), binarylog => 1,
                               locking => $locking, fingerprint => 1, usebody=>$apiBody);

    for my $el ("pre", "post") {
        if ($m->options($el . "transitions")) {
            print $outfile $m->toString("methodprefix" => "${name}Service::_${el}_", "nodefaults" => 1,
                                 "usebody" => $this->checkGuardFireTransition($m, "${el}transitions"),
                                 selectorVar => 1, nologs => 1
                                 );
        }
    }

}

sub mergeClasses {
    my $this = shift;
    my @r = ();
    for my $m ($this->providedMethods(), $this->usesHandlerMethods(), $this->timerMethods()) {
	if ($m->options("pretransitions") or $m->options("posttransitions")) {
	    my $name = "Merge_" . $m->options("selectorVar");
	    push(@r, $name);
	}
    }
    return @r;
}

sub generateMergeClasses {
    my $this = shift;
    my $c = "//BEGIN Mace::Compiler::ServiceImpl::generateMergeClasses\n";
    for my $m ($this->providedMethods(), $this->usesHandlerMethods(), $this->timerMethods()) {
	if ($m->options("pretransitions") or $m->options("posttransitions")) {
            my $svName = $this->name();
	    my $name = "Merge_" . $m->options("selectorVar");
	    $c .= "class ${svName}Service::$name {\n";
	    $c .= "private:\n";
	    $c .= $this->name() . "Service* sv;\n";
	    $c .= join("\n", map {$_->toString(nodefaults=>1) . ";"} $m->params()) . "\n";
	    $c .= "public:\n";
	    $c .= "$name(" . join(", ", ($this->name() . "Service* sv", map{$_->toString} $m->params())) . ") : ";
	    $c .= join(", ", ("sv(sv)", map{$_->name . "(" . $_->name . ")"} $m->params())) . " {\n";
	    if ($m->options("pretransitions")) {
		$c .= "sv->_pre_" . $m->name() . "(" . join(",", map{$_->name()} $m->params()) . ");\n";
	    }
	    $c .= "}\n";
	    $c .= "virtual ~$name() {\n";
	    if ($m->options("posttransitions")) {
		$c .= "sv->_post_" . $m->name() . "(" . join(",", map{$_->name()} $m->params()) . ");\n";
	    }
	    $c .= "}\n";
	    $c .= "}; // class $name\n\n";
	}
    }
    $c .= "//END Mace::Compiler::ServiceImpl::generateMergeClasses\n";
    return $c;
}

sub declareMergeMethods {
    my $this = shift;
    my $r = "";
    for my $m ($this->providedMethods(), $this->usesHandlerMethods(), $this->timerMethods()) {
	for my $el ("pre", "post") {
	    if ($m->options("${el}transitions")) {
		$r .= $m->toString("methodprefix" => "_${el}_", "nodefaults" => 1) . ";\n";
	    }
	}
    }
    return $r;
}

sub checkGuardFireTransition {

    my $this = shift;
    my $m = shift;
    my $key = shift;
    my $else = shift || "";

    my $r = "";
    map {
	my $guardCheck = $_->getGuardMethodName()."(".join(",", map{$_->name} $m->matchedParams($_->method)).")";
	my $transitionCall = $_->getTransitionMethodName()."(".join(",", map{$_->name} $m->matchedParams($_->method)).")";
	my $setOnce = "";
	if ($_->isOnce()) {
	    $setOnce = "once_".$_->getTransitionMethodName()." = true;";
	}
	my $return = '';
	my $returnend = '';
	if (!$m->returnType->isVoid()) {
            if($_->method()->getLogLevel($this->traceLevel) > 1) {
		my $rt = $m->returnType->toString();
		my $type = "\"rv_" . Mace::Util::typeToTable($rt) . "\"";
		#$return = "$rt __ret =";
		#$returnend = ";\nLog::binaryLog(selectorId->compiler, mace::SimpleLogObject<" . $rt . " >(" . $type . ", __ret));\nreturn __ret;\n";
                $return = 'return mace::logVal(';
                $returnend = ", selectorId->compiler, $type)";
            }
            else {
              $return = 'return';
            }
	}
        my $called = "";
        if ($_->method()->getLogLevel($this->traceLevel) > 1) {
            $called = qq/macecompiler(1) << "Firing Transition ${\$_->toString()}" << Log::endl;\n/;
        }

	my $resched = "";
	if ($m->options('timer')) {
	    my $timer = $m->options('timer');
	    #TODO Pip Stuff
	    if ($m->options('timerRecur')) {
		my $recur = $m->options('timerRecur');
		my $pstring = join("", map{", ".$_->name} $m->params());
		$resched = qq{$timer.reschedule($recur$pstring);
			   };
	    }
	}

	$r .= qq/ if($guardCheck) {
                $called
		$setOnce
		    $resched
		    $return $transitionCall$returnend;
	    } $else
	    /;
    } @{$m->options($key)};

    return $r;

}

# Needs to start by assuming service specified locking (default on).
#
# Locking options: exclusive, shared, none
# Concern: what if not all transitions of the same type specify the same kind of locking.
#
# ANS: Need to grab the highest lock level of any transition.
#
# Plan for execution: Can go from none to any type, and a higher type to a lower type, but not v/v.
#
# Yoo : Now it takes service-wide global locking type over no-locking specified transition.
#       Since demux function takes service-wide global locking, it causes error.
#       Please check with the previous version.

sub checkTransitionLocking {
    my $this = shift;
    my $m = shift;
    my $key = shift;
    my $else = shift || "";

    my $r = -1;
    map {
        $r = ($r >= $_->getLockingType($this->locking())) ? $r : $_->getLockingType($this->locking());
        #print STDERR "[ServiceImpl.pm checkTransitionLocking()] ".$_->name()."  locking = ".$r."\n";
    } @{$m->options($key)};

    return $r;
}

sub printAPIDemux {
    my $this = shift;
    my $outfile = shift;

    my $name = $this->name();

    print $outfile "//BEGIN Mace::Compiler::ServiceImpl::printAPIDemux\n";
    for my $m (grep(!($_->name =~ /^(un)?register.*Handler$/), $this->providedMethods())) {

        $this->demuxMethod($outfile, $m, "downcall");

    }
    print $outfile "//END Mace::Compiler::ServiceImpl::printAPIDemux\n";
}

sub printAsyncDemux {
    my $this = shift;
    my $outfile = shift;

    my $name = $this->name();

    print $outfile "//BEGIN Mace::Compiler::ServiceImpl::printAsyncDemux\n";
    for my $m ($this->asyncMethods()) {

        $this->demuxMethod($outfile, $m, "async");

    }
    print $outfile "//END Mace::Compiler::ServiceImpl::printAsyncDemux\n";
}

sub printBroadcastDemux {
    my $this = shift;
    my $outfile = shift;

    my $name = $this->name();

    print $outfile "//BEGIN Mace::Compiler::ServiceImpl::printBroadcastDemux\n";
    for my $m ($this->broadcastMethods()) {

        $this->demuxMethod($outfile, $m, "broadcast");

    }
    print $outfile "//END Mace::Compiler::ServiceImpl::printBroadcastDemux\n";
}

sub printAspectDemux {
    my $this = shift;
    my $outfile = shift;

    my $name = $this->name();

    print $outfile "//BEGIN Mace::Compiler::ServiceImpl::printAspectDemux\n";
    for my $m ($this->aspectMethods()) {
        $this->demuxMethod($outfile, $m, "aspect");
    }
    print $outfile "//END Mace::Compiler::ServiceImpl::printAspectDemux\n";
}

sub printHandlerRegistrations {
    my $this = shift;
    my $outfile = shift;

    my $name = $this->name();

    print $outfile "//BEGIN Mace::Compiler::ServiceImpl::printHandlerRegistrations\n";
    my $isUniqueReg = ($this->registration() eq "unique");

    for my $h ($this->providedHandlers()) {
        my $hname = $h->name();

	my $assertUnique = "";
	if ($isUniqueReg) {
	    $assertUnique = "ASSERT(map_${hname}.empty());";
	}

	print $outfile <<END;
	registration_uid_t ${name}Service::registerHandler(${hname}& handler, registration_uid_t regId, bool isAppHandler = true) {
	    if(regId == -1) { regId = NumberGen::Instance(NumberGen::HANDLER_UID)->GetVal(); }
	    $assertUnique
            ASSERT(map_${hname}.find(regId) == map_${hname}.end());
	    map_${hname}[regId] = &handler;
        if( isAppHandler ){ apphandler_${hname}.insert( regId ); }
	    return regId;
	}

	void ${name}Service::registerUniqueHandler(${hname}& handler, bool isAppHandler = true) {
	    ASSERT(map_${hname}.empty());
	    map_${hname}[-1] = &handler;
        if( isAppHandler ){ apphandler_${hname}.insert( -1 ); }
	}

	void ${name}Service::unregisterHandler(${hname}& handler, registration_uid_t regId) {
	    ASSERT(map_${hname}[regId] == &handler);
	    map_${hname}.erase(regId);
        apphandler_${hname}.erase( regId );
	}
END
    }
    if ($this->isDynamicRegistration()) {
        my $regType = $this->registration();
        print $outfile qq/
        registration_uid_t ${name}Service::allocateRegistration($regType const & object, registration_uid_t regId) {
            std::ostringstream out;
            out << object << regId;
            MaceKey hashed(sha32,out.str());
            registration_uid_t retval = hashed.getNthDigit(0,32);
            _regMap[retval] = object;
        /;

        for my $h ($this->providedHandlers()) {
            my $hname = $h->name();
            print $outfile qq/
            {
                maptype_${hname}::const_iterator i = map_${hname}.find(regId);
                maptype_${hname}::const_iterator j = map_${hname}.find(retval);
                ASSERT(j == map_${hname}.end() || (i != map_${hname}.end() && i->second == j->second));
                if (i != map_${hname}.end()) {
                    map_${hname}[retval] = i->second;
                }
            }
            /;
        }

        print $outfile qq/
            return retval;
        }
        void ${name}Service::freeRegistration(registration_uid_t regId, registration_uid_t baseRid) {
            ASSERT(_regMap.find(regId) != _regMap.end());
            _regMap.erase(regId);
        /;

        for my $h ($this->providedHandlers()) {
            my $hname = $h->name();
            print $outfile qq/
                ASSERT(map_${hname}.find(baseRid) != map_${hname}.end());
                ASSERT(map_${hname}[regId] == map_${hname}[baseRid]);
                map_${hname}.erase(regId);
            /;
        }

        print $outfile qq/
        }

        bool ${name}Service::getRegistration($regType & object, registration_uid_t regId) {
            if (_regMap.find(regId) != _regMap.end()) {
                object = _regMap[regId];
                return true;
            }
            return false;
        }
        /;
    }
    print $outfile "//END Mace::Compiler::ServiceImpl::printHandlerRegistrations\n";
}

sub printHandlerDemux {
    my $this = shift;
    my $outfile = shift;

    my $name = $this->name();

    print $outfile "//BEGIN Mace::Compiler::ServiceImpl::printHandlerDemux\n";
    for my $m ($this->usesHandlerMethods()) {
#        print "DEBUG-DEMUX: ".$m->toString(noline=>1)."\n";
        $this->demuxMethod($outfile, $m, "upcall");
    }
    print $outfile "//END Mace::Compiler::ServiceImpl::printHandlerDemux\n";
}

sub printTimerDemux {
    my $this = shift;
    my $outfile = shift;

    my $name = $this->name();

    print $outfile "//BEGIN Mace::Compiler::ServiceImpl::printTimerDemux\n";
    for my $m ($this->timerMethods()) {
        $this->demuxMethod($outfile, $m, "scheduler");
    }
    print $outfile "//END Mace::Compiler::ServiceImpl::printTimerDemux\n";
}

sub printChangeTracker {
    my $this = shift;
    my $outfile = shift;

    print $outfile "//BEGIN Mace::Compiler::ServiceImpl::printChangeTracker\n";
    if ($this->count_onChangeVars()) {

	my $name = $this->name();
        my $ctVarsCheck = "";
        for my $asp ($this->aspectMethods()) {
          my $aname = $asp->name();
#Wei-Chiu: temporary hack
=begin
          $ctVarsCheck .= "if(" . join("||", map{ "(sv->$_ != sv->_MA_${aname}_$_)" } @{$asp->options('monitor')}) . ") {
                            somethingChanged = true;
                            sv->$aname(".join(",", map { "sv->_MA_${aname}_$_" } @{$asp->options('monitor')}).");
                            ".join("\n", map{ "if(sv->$_ != sv->_MA_${aname}_$_) { sv->_MA_${aname}_$_ = sv->$_; } " } @{$asp->options('monitor')})."
                           }
                           ";
=cut
          $ctVarsCheck .= "if(" . join("||", map{ "(sv->globalContext->$_ != sv->_MA_${aname}_$_)" } @{$asp->options('monitor')}) . ") {
                            somethingChanged = true;
                            sv->$aname(".join(",", map { "sv->_MA_${aname}_$_" } @{$asp->options('monitor')}).");
                            ".join("\n", map{ "if(sv->globalContext->$_ != sv->_MA_${aname}_$_) { sv->_MA_${aname}_$_ = sv->globalContext->$_; } " } @{$asp->options('monitor')})."
                           }
                           ";
        }


	print $outfile <<END;
	class __ChangeTracker__ {
	  private:
	    //Pointer to the service for before/after inspection
		${name}Service* sv;
	    //Copies of the variables initial conditions for 'var' method variables
	      public:
		__ChangeTracker__(${name}Service* service) : sv(service) {}
	    ~__ChangeTracker__() {
              bool somethingChanged = false;
              do {
                somethingChanged = false;
		$ctVarsCheck
              } while(somethingChanged);
		}
	};
END
    }
    print $outfile "//END Mace::Compiler::ServiceImpl::printChangeTracker\n";
}
sub printSerialHelperDemux {
    my $this = shift;
    my $outfile = shift;

    my $name = $this->name();

    print $outfile "//BEGIN Mace::Compiler::ServiceImpl::printSerialHelperDemux\n";
    for my $m ($this->implementsUpcalls(), $this->implementsDowncalls()) {
	if ($m->serialRemap) {
	    $this->demuxSerial($outfile, $m);
	}
    }
    print $outfile "//END Mace::Compiler::ServiceImpl::printSerialHelperDemux\n";
}

sub demuxSerial {
    my $this = shift;
    my $outfile = shift;
    my $m = shift;
    my $name = $this->name();

    my $fnName = $m->name();

    my $return = '';
    my $vreturn = "return;";
    if (!$m->returnType->isVoid()) {
	$return = 'return';
	$vreturn = "";
    }

    my $apiBody = qq{//TODO: try/catch Serialize
		     };
    my $rawt = $this->rawTransitions($fnName);
    if ($rawt) {
        my $rawm = $rawt->getTransitionMethodName();
        for my $sv ($this->service_variables()) {
            if ($sv->raw()) {
                my $rid = $sv->registrationUid();
                my $lock = "";
                #if ($this->locking() > -1) {
                #    $lock = "mace::AgentLock __rawlock(".$this->locking().");";
                #}
#chuangw: TODO
# if the service uses contexts, it should use ContextLock instead of AgentLock,
# but I am not sure when demuxSerial is called so I do not plan to change this part of code now.
                $apiBody .= "if (rid == $rid) {
  $lock
  $return $rawm(".$m->paramsToString(notype=>1, nodefaults=>1).");
  $vreturn
}\n";
	    }
	}
    }

    for my $p ($m->params()) {
        if ($p->typeSerial and $p->typeSerial->type ne 'Message') {
            my $dstype = $p->type()->type();
            my $typeSerial = $p->typeSerial()->type();
            my $pname = $p->name();
            $apiBody .= qq/
 $typeSerial ${pname}_deserialized;
 ScopedDeserialize<$dstype, $typeSerial > __sd_$pname(${pname}, ${pname}_deserialized);
			/;
	}
    }
    my $msgDeserialize = "$return $fnName(".join(",", map{$_->name.(($_->typeSerial)?"_deserialized":"")}$m->params()).");\n";
    for my $p ($m->params()) {
        if ($p->typeSerial and $p->typeSerial->type eq 'Message') {
            my $pname = $p->name();
            my $msgTraceNum = "//TODO -- trace num\n";
            my $msgTrace = "//TODO -- trace msg\n";
            my $body = $m->body();
            my $msgDeserializeTmp = qq/
uint8_t msgNum_$pname = Message::getType($pname);
$msgTraceNum
switch(msgNum_$pname) {
/;
            for my $msg ($this->messages()) {
                my $msgName = $msg->name;
                $msgDeserializeTmp .= qq/
                    case ${msgName}::messageType: {
                        ${msgName} ${pname}_deserialized;
                        ${pname}_deserialized.deserializeStr($pname);
                        $msgTrace
                            $msgDeserialize
                        }
                    break;
                /;
            }
            $msgDeserializeTmp .= qq/ default: {
                maceerr << "FELL THROUGH NO PROCESSING -- INVALID MESSAGE NUMBER: " << msgNum_$pname << Log::endl;
                $body
                    ABORT("INVALID MESSAGE NUMBER");
            }
              break;
          /;
            $msgDeserializeTmp .= "}\n";
            $msgDeserialize = $msgDeserializeTmp;
        }
    }
    $apiBody .= $msgDeserialize;
    unless($m->returnType->isVoid()) {
        $apiBody .= qq/\nABORT("Should never reach here - should have returned value from call to serialized form");\n/;
    }

    print $outfile $m->toString("methodprefix" => "${name}Service::", "nodefaults" => 1, usebody => $apiBody,
                                notextlog => 1, selectorVar => 1, traceLevel => $this->traceLevel());

}

sub printDowncallHelpers {
    my $this = shift;
    my $outfile = shift;

    my $name = $this->name();

    print $outfile "//BEGIN Mace::Compiler::ServiceImpl::printDowncallHelpers\n";

    print $outfile join("\n", $this->generateAddDefer("downcall", my $ref = $this->downcallDeferMethods()));

    if ($this->count_downcall_registrations()) {
        print $outfile qq/void ${name}Service::downcall_freeRegistration(registration_uid_t freeRid, registration_uid_t rid) {
            /;

        my @regSv = grep($_->registration(), $this->service_variables());
        if (scalar(@regSv) == 1) {
            my $svname = $regSv[0]->name();
            print $outfile qq/if (rid == -1) {
                                rid = $svname;
                              }
                             /;
            #print $outfile "// DEBUG:: ".$regSv[0]."\n";
        }

        for my $sv (@regSv) {
            my $svname = $sv->name();
            print $outfile qq/if (rid == $svname) {
                                ASSERT(_registered_$svname.find(freeRid) != _registered_$svname.end());
                                _d_$svname.freeRegistration(freeRid, rid);
                                _registered_$svname.erase(freeRid);
                              }
                             /;
        }

        print $outfile qq/
        }
        /;
    }
    for my $r ($this->downcall_registrations()) {

        print $outfile qq/
          registration_uid_t ${name}Service::downcall_allocateRegistration($r const & object, registration_uid_t rid) {
            /;

        my @regSv = grep($_->registration() eq $r, $this->service_variables());
        if (scalar(@regSv) == 1) {
            my $svname = $regSv[0]->name();
            print $outfile qq/if (rid == -1) {
                                rid = $svname;
                              }
                             /;
        }

        for my $sv (@regSv) {
            my $svname = $sv->name();
            print $outfile qq+if (rid == $svname) {
                                registration_uid_t newrid = _d_$svname.allocateRegistration(object, rid);
                                //ASSERT(_registered_$svname.find(newrid) == _registered_$svname.end());
                                _registered_$svname.insert(newrid);
                                return newrid;
                              }
                             +;
        }

        print $outfile qq/
            ABORT("allocateRegistration called on invalid registration_uid_t");
          }
          bool ${name}Service::downcall_getRegistration($r & object, registration_uid_t rid) {
            /;

        if (scalar(@regSv) == 1) {
            my $svname = $regSv[0]->name();
            print $outfile qq/if (rid == -1) {
                                rid = $svname;
                              }
                             /;
        }
        for my $sv (@regSv) {
            my $svname = $sv->name();
            print $outfile qq/if (_registered_$svname.find(rid) != _registered_$svname.end()) {
                                ASSERT(_d_$svname.getRegistration(object, rid));
                                return true;
                              }
                             /;
        }

        print $outfile qq/
            return false;
          }
        /;

    }

    #localAddress downcall helper method
    my $svAddr = "";
    my $svVal = "";
    my $svCheck = "MaceKey tmp = MaceKey::null;";
    my $svOne = "";
    my $num = 0;
    for my $sv ($this->service_variables) {
        if (not $sv->intermediate() ) {
            $num++;
            my $svn = $sv->name();
            $svOne = "return _$svn.localAddress();";
            $svCheck .= qq/
                if (tmp.isNullAddress()) {
                    tmp = _$svn.localAddress();
                }
                else {
                    MaceKey tmp2 = _$svn.localAddress();
                    ASSERTMSG(tmp2.isNullAddress() || tmp == tmp2, "Requesting lower level address from service $name, but lower level service $svn does not return same address as another service.  If this occurs due to default computing a local address you may need a local_address block to disambiguate.  Otherwise, either you are using incompatible lower level services, or you need to specify a service to ask for the local address from by passing in the service parameter.");
                } /;
            $svAddr .= qq/
                if (&rid == &$svn) {
                    return _$svn.localAddress();
                } /;
            $svVal .= qq/
                if (rid == $svn) {
                    return _$svn.localAddress();
                } /;
        }
    }
    if ($num == 1) {
        $svCheck = $svOne;
    } else {
        $svCheck .= "
            return tmp;"
    }

    print $outfile qq/
        MaceKey ${name}Service::downcall_localAddress() const {
            $svCheck
        }
        const MaceKey& ${name}Service::downcall_localAddress(const registration_uid_t& rid) const {
            $svAddr
            $svVal
            return MaceKey::null;
        }
    /;


    #downcall helper methods
    my $usesTransport;
    for( $this->service_variables() ){
         if( $_->serviceclass() eq "Transport" ){ $usesTransport = 1 ; }
    }
    
    my %messagesHash = ();
    map { $messagesHash{ $_->name() } = $_ } $this->messages();
    for my $m ($this->usesClassMethods()) {
        
        my $routine = $this->createUsesClassHelper( $m, \%messagesHash );

        print $outfile $m->toString("methodprefix"=>"${name}Service::downcall_", "noid" => 0, "novirtual" => 1, "nodefaults" => 1, selectorVar => 1, binarylog => 1, traceLevel => $this->traceLevel(), usebody => "$routine");
    }
    print $outfile "//END Mace::Compiler::ServiceImpl::printDowncallHelpers\n";
}
sub createUsesClassHelper {
    my $this = shift;
    my $m = shift;
    my $ref_messagesHash = shift;

    my $origmethod = $m;
    my $serialize = "";
    my $checkDefer = "";
    my $defaults = "";
    if ($m->options('original')) {
        #TODO: try/catch Serialization
        $origmethod = $m->options('original');
        my @oparams = $origmethod->params();
        for my $p ($m->params()) {
            my $op = shift(@oparams);
            if (! $op->type->eq($p->type)) {
                my $optype = $op->type->type();
                my $opname = $op->name;
                my $ptype = $p->type->type();
                my $pname = $p->name;
                $serialize .= qq{ $optype $opname;
                                  ScopedSerialize<$optype, $ptype > __ss_$pname($opname, $pname);
                              };
            }
        }
    }
    if ($m->options('remapDefault')) {
        for my $p ($m->params()) {
            if ($p->flags('remapDefault')) {
                my $pn = $p->name();
                my $pd = $p->default();
                my $pd2 = $p->flags('remapDefault');

                $defaults .= qq{ if($pn == $pd) { $pn = $pd2; }
                             };
            }
        }
    }
    my @matchedServiceVars;
    for my $sv ($this->service_variables) {
        if (not $sv->intermediate() and ref Mace::Compiler::Method::containsTransition($origmethod, $this->usesClasses($sv->serviceclass))) {
            push(@matchedServiceVars, $sv);
        }
    }
    if ( (scalar(@matchedServiceVars) == 1) ) {
        my $rid = $m->params()->[-1]->name();
        my $svn = $matchedServiceVars[0]->name();
        $defaults .= qq{ if($rid == -1) { $rid = $svn; }
                     };
    }
    my $callString = "";
    for my $sv (@matchedServiceVars) {
        my $rid = $m->params()->[-1]->name();
        my $callm = $origmethod->name."(".join(",", map{$_->name} $origmethod->params()).")";
        my $svname = $sv->name;
        my $regtest = "";
        if ($sv->registration()) {
            $regtest = qq{ || _registered_$svname.find($rid) != _registered_$svname.end()};
        }
        my $return = (!$m->returnType->isVoid())?"return":"";
        $callString .= qq/if($rid == $svname$regtest) {
            $return _$svname.$callm;
        } else
        /;
    }
    #TODO: Logging, etc.
    $callString .= qq/{ ABORT("Did not match any registration uid!"); }/;

    return "
            $serialize
            $defaults
            $callString
    ";
}

sub createNonConstCopy {
  my ($this, $param) = @_;

  my $p = ref_clone( $param );
  $p->type->isConst(0);
  $p->type->isConst1(0);
  $p->type->isConst2(0);
  $p->type->isRef(0);

  return $p;
}
sub createApplicationUpcallInternalMessage {
  my $this = shift;
  my $origmethod = shift;
  my $mnumber = shift;

  my $at = $origmethod->createApplicationUpcallInternalMessage( $mnumber  );
  if ( $this->hasContexts() ){
    $at->validateMessageOptions();
  }else{
    my $serializeOption = Mace::Compiler::TypeOption->new(name=> "serialize");
    $serializeOption->options("no","no");
    $at->push_typeOptions( $serializeOption );
    my $constructorOption = Mace::Compiler::TypeOption->new(name=> "constructor");
    $constructorOption->options("default","no");
    $at->push_typeOptions( $constructorOption );
  }
  $origmethod->options("serializer", $at );

  $at->options('appupcall_method', $origmethod );
}
sub printUpcallHelpers {
    my $this = shift;
    my $outfile = shift;

    my $name = $this->name();

    print $outfile "//BEGIN Mace::Compiler::ServiceImpl::printUpcallHelpers\n";

    print $outfile join("\n", $this->generateAddDefer("upcall", my $ref = $this->upcallDeferMethods()));

    my $mcounter = 1;
    for my $m ($this->providedHandlerMethods()) {
        my $origmethod = $m;
        my $serialize = "";
        my $defaults = "";
        my @serializedParamName;
        if ($m->options('original')) {
            #TODO: try/catch Serialization
            $origmethod = $m->options('original');
            my @oparams = $origmethod->params();
            for my $p ($m->params()) {
                my $op = shift(@oparams);
                if (! $op->type->eq($p->type)) {
                    my $optype = $op->type->type();
                    my $opname = $op->name;
                    my $ptype = $p->type->type();
                    my $pname = $p->name;
                    $serialize .= qq{ $optype $opname;
                                      ScopedSerialize<$optype, $ptype > __ss_$pname($opname, $pname);
                                  };
                    push @serializedParamName, $pname;
                }else{
                    push @serializedParamName, $op->name;
                }
            }
        }else{
            map{ push @serializedParamName, $_->name() } $m->params() ;
        }
        if ($m->options('remapDefault')) {
            for my $p ($m->params()) {
                if ($p->flags('remapDefault')) {
                    my $pn = $p->name();
                    my $pd = $p->default();
                    my $pd2 = $p->flags('remapDefault');

                    #print "$pn $pd $pd2\n";

    #chuangw: a temporary hack.
    # if the service var matches a (global) state variable, get to the global context object
                    my $globalContext = "";
                    for my $globalVar ( @{ ${ $this->contexts() }[0]->ContextVariables() } ){
                      #print $globalVar->name  . " $pd2\n";
                      if( $globalVar->name() eq  $pd2 ){
                        #print " matched\n";
                        $globalContext = "globalContext->";
                        last;
                      }
                    }
                    $defaults .= qq{ if($pn == $pd) { $pn = $globalContext$pd2; }
                                 };
                }
            }
        }
        my $callString = "";

        my @handlerArr = @{$m->options('class')};
        unless(scalar(@handlerArr) == 1) {
            Mace::Compiler::Globals::error("ambiguous_upcall", $m->filename(), $m->line(), "Too many possible Handler types for this method (see Mace::Compiler::ServiceImpl [2])");
            #[2] : In the present implementation, if an upcall could map to more than
            #one handler type, we do not support it.  In theory, we could have a
            #stratgegy where we search the upcall maps in a given order, upcalling to
            #the first one we find.  Especially since if they had the same rid, they
            #should refer to the same bond at least.  However, for simplicity, for
            #now we just drop it as an error.
            next;
        }
        my $handler = shift(@handlerArr);
        my $hname = $handler->name;
        my $mname = $m->name;
        my $body = $m->body;
        my $rid = $m->params()->[-1]->name();
        my $return = '';
        if (!$m->returnType->isVoid()) {
            $return = 'return';
        }
        my $callm = $origmethod->name."(".join(",", map{$_->name} $origmethod->params()).")";
        my $iterator = "iterator";
        if ($m->isConst()) {
            $iterator = "const_iterator"
        }
        my $atname = "__appupcall_at${mcounter}_$origmethod->{name}";
        my $deferAction="";
        my @deferMsgParams = ( "mace::imsg", @serializedParamName );
        if ($m->returnType->isVoid()) {
          $deferAction=  "$atname* msg = new $atname( " . join(", ", @deferMsgParams  ) . " );
                          deferApplicationUpcall( msg );
                          return;";
        }else{
          $deferAction="$atname* msg = new $atname( " . join(", ", @deferMsgParams  ) . " );
                        return returnApplicationUpcall< ${ \${ \$m->returnType() }->name() } >( msg );";
        }
        # An external world application has registered with this upcall.
        # this upcall is going out of the fullcontext world into the outer-world application.
        my $deferApplicationHandler = qq#
            if( ThreadStructure::getCurrentContext() != mace::ContextMapping::HEAD_CONTEXT_ID && apphandler_${hname}.count( rid ) > 0 ){
                $deferAction
            }
        #;
        if ($this->registration() eq "unique") {
            $callString .= qq{
                          ASSERT(map_${hname}.size() <= 1);
                          maptype_${hname}::$iterator iter = map_${hname}.begin();
                          if(iter == map_${hname}.end()) {
                              maceWarn("No $hname registered with uid %"PRIi32" for upcall $mname!", $rid);
                              $body
                              }
                          else {
                              $return iter->second->$callm;
                          }
                      };
        }
        else {
            $callString .= qq{maptype_${hname}::$iterator iter = map_${hname}.find($rid);
                          if(iter == map_${hname}.end()) {
                              maceWarn("No $hname registered with uid %"PRIi32" for upcall $mname!", $rid);
                              $body
                              }
                          else {
                              $return iter->second->$callm;
                          }
                      };
        }
        my $routine = $m->toString("methodprefix"=>"${name}Service::upcall_", "noid" => 0, "novirtual" => 1, "nodefaults" => 1, selectorVar => 1, binarylog => 1, traceLevel => $this->traceLevel(), usebody => "
                $serialize
                $defaults
                $deferApplicationHandler
                $callString
        ");
        print $outfile $routine;
        $mcounter ++;
    }
    print $outfile "//END Mace::Compiler::ServiceImpl::printUpcallHelpers\n";
}
# XXX: chuangw: printDummyConstructor subroutine is not used anywhere.
sub printDummyConstructor {
    my $this = shift;
    my $outfile = shift;

    my $name = $this->name();
    my @svo = grep( not($_->intermediate), $this->service_variables());

    my $BaseService = "BaseMaceService";
    if( $Mace::Compiler::Globals::useFullContext ){
      $BaseService = "ContextService";
    }

    #TODO: utility_timer
    my $constructors = "${name}Dummy::${name}Dummy() : \n//(\n$BaseService(), __inited(0)";
#    my $constructors = "${name}Dummy::${name}Dummy(".join(", ", (map{$_->type->toString()." _".$_->name} $this->constructor_parameters()) ).") : \n//(\nBaseMaceService(), __inited(0)";
    $constructors .= ", _actual_state(init), state(_actual_state)";
    map{ my $timer = $_->name(); $constructors .= ",\n$timer(*(new ${timer}_MaceTimer(this)))"; } $this->timers();
    $constructors .= qq|{
    }
    |;

    print $outfile $constructors;
    print $outfile "//END Mace::Compiler::ServiceImpl::printDummyConstructor\n";
}

sub printLoadProtocol {
    my $this = shift;
    my $outfile = shift;

    my $name = $this->name();

    print $outfile "//BEGIN Mace::Compiler::ServiceImpl::printLoadProtocol\n";

    foreach my $scProvided ($this->provides()) {
        print $outfile "${scProvided}ServiceClass& configure_new_${name}_${scProvided}(bool ___shared);\n";
    }

    my $attrs = $this->attributes();
    print $outfile "\n  void load_protocol() {\n";
    print $outfile qq/StringSet attr = mace::makeStringSet("$attrs", ",");\n/;

    foreach my $scProvided ($this->provides()) {
        print $outfile qq/mace::ServiceFactory<${scProvided}ServiceClass>::registerService(&configure_new_${name}_${scProvided}, "$name");\n/;
        print $outfile qq/mace::ServiceConfig<${scProvided}ServiceClass>::registerService("$name", attr);\n/;
    }

    print $outfile "  }\n";

    print $outfile "//END Mace::Compiler::ServiceImpl::printLoadProtocol\n";
}

sub printConstructor {
    my $this = shift;
    my $outfile = shift;

    my $name = $this->name();
    my @svo = grep( not($_->intermediate), $this->service_variables());
    my @svp = grep( not($_->intermediate or $_->final), $this->service_variables());

    print $outfile "//BEGIN Mace::Compiler::ServiceImpl::printConstructor\n";
    foreach my $scProvided ($this->provides()) {
        my $realMethod = "real_new_${name}_$scProvided";
	print $outfile "${scProvided}ServiceClass& $realMethod(".join(", ", (map{$_->serviceclass."ServiceClass& ".$_->name} @svo), (map{$_->toString("nodefaults" => 1)} $this->constructor_parameters()), "bool ___shared" ).") {\n";
#	print $outfile "return *(new ${name}Service(".join(",", (map{$_->name} @svp, $this->constructor_parameters)).") );\n";
	print $outfile "return *(new ${name}Service(".join(",", (map{$_->name} @svp, $this->constructor_parameters), "___shared")."));\n";
        print $outfile "}\n";
    }


    my $BaseService = "BaseMaceService";
    if( $Mace::Compiler::Globals::useFullContext ){
      $BaseService = "ContextService";
    }

    #TODO: utility_timer
    my $constructors = "${name}Service::${name}Service(".join(", ", (map{$_->serviceclass."ServiceClass& __".$_->name} @svo), (map{$_->type->toString()." _".$_->name} $this->constructor_parameters()), "bool ___shared" ).") : \n//(\n$BaseService( &improcessor ), __inited(0), improcessor( this )";
    $constructors .= ", _actual_state(init), state(_actual_state)";
    map{
        my $n = $_->name();
        $constructors .= ",\n$n(_$n)";
    } $this->constructor_parameters();
    map{
        my $n = $_->name();
        my $rid = $_->registrationUid();
        $constructors .= ",\n_$n(__$n), $n($rid)";
        if ($_->registration()) {
            my $regType = $_->registration(); $constructors .= ",\n_d_$n(dynamic_cast<DynamicRegistrationServiceClass<$regType>& >(_$n))";
        }
    } @svo;
    map{
        my $n = $_->name();
        my $default = "";
        if ($_->hasDefault()) {
            $default = $_->default();
        }
        $constructors .= ",\n$n($default)";
    } $this->state_variables(), $this->onChangeVars(); #nonTimer => state_Var
    map{ my $timer = $_->name(); $constructors .= ",\n$timer(*(new ${timer}_MaceTimer(this)))"; } $this->timers();

    # Initialize contexts
    map {
      if( $_->isArray ){
        $constructors .= ",\n$_->{name} ( )";
      }else{
        $constructors .= ",\n$_->{name} (NULL)";
      }
    } ${ $this->contexts() }[0], ${ $this->contexts() }[0]->subcontexts();

    my $registerInstance = "
    if (___shared) {
    ".join("\n", map{ qq/mace::ServiceFactory<${_}ServiceClass>::registerInstance("$name", this);/ } $this->provides())."
    }
    ServiceClass::addToServiceList(*this);
    ";
    my $propertyRegister = "";
    if ($this->count_safetyProperties() or $this->count_livenessProperties()) {
        $propertyRegister = qq/
            if (macesim::SimulatorFlags::simulated()) {
                static bool defaultTest = params::get<bool>("AutoTestProperties", 1);
                static bool testThisService = params::get<bool>("AutoTestProperties.$name", defaultTest);
                if (testThisService) {
                    static int testId = NumberGen::Instance(NumberGen::TEST_ID)->GetVal();
                    macesim::SpecificTestProperties<${name}Service>::registerInstance(testId, this);
                }
            }
        /;
    }
    $constructors .= ", __local_address(MaceKey::null)";
    $constructors .= qq|\n{
	initializeSelectors();
        this->loadContextMapping( mace::ContextMapping::getInitialMapping( "${name}" ) );
        __local_address = computeLocalAddress();
        $registerInstance
        $propertyRegister
        ADD_SELECTORS("${name}::(constructor)");
        macedbg(1) << "Created queued instance " << this << Log::endl;
    }
    //)
    |;

    $constructors .= "${name}Service::${name}Service(const ${name}Service& _sv) : \n//(\n$BaseService( &improcessor, false), __inited(_sv.__inited), improcessor( this )";
    $constructors .= ", _actual_state(_sv.state), state(_actual_state)";
    map{
        my $n = $_->name();
        $constructors .= ",\n$n(_sv.$n)";
    } $this->constructor_parameters();
    map{
        my $n = $_->name();
        my $rid = $_->registrationUid();
        $constructors .= ",\n_$n(_sv._$n), $n(_sv.$n)";
        # if ($_->registration()) {
        #     my $regType = $_->registration(); $constructors .= ",\n_d_$n(dynamic_cast<DynamicRegistrationServiceClass<$regType>& >(_$n))";
        # }
    } @svo;
    map{
        my $n = $_->name();
        $constructors .= ",\n$n(_sv.$n)";
    } $this->state_variables(), $this->onChangeVars(); #nonTimer => state_Var
#    map{ my $timer = $_->name(); $constructors .= ",\n$timer(_sv.$timer)"; } $this->timers(); # Note: timer sv pointer will point to "main" service, not this copy...
    map{ my $timer = $_->name(); $constructors .= ",\n$timer(*(new ${timer}_MaceTimer(this)))"; } $this->timers(); # These pointers are new copies - don't share data...  Would probably prefer not to have them at all?
    map {
      if( $_->isArray ){
        $constructors .= ",\n$_->{name} ( _sv.$_->{name} )";
      }else{
        $constructors .= ",\n$_->{name} ( new $_->{className} ( *(_sv.$_->{name}) ) )";
      }
    } ${ $this->contexts() }[0], ${ $this->contexts() }[0]->subcontexts();
    $constructors .= qq|{
        ADD_SELECTORS("${name}::(constructor)");
        macedbg(1) << "Created non-queued instance " << this << Log::endl;
    }
    //)
    |;

    print $outfile $constructors;
    print $outfile "//END Mace::Compiler::ServiceImpl::printConstructor\n";
}

sub traceLevel {
    my $this = shift;
    if ($Mace::Compiler::Globals::traceOverride > -2) {
        return $Mace::Compiler::Globals::traceOverride;
    }
    if ($this->trace eq 'off') {
        return -1;
    }
    if ($this->trace eq 'manual') {
        return 0;
    }
    if ($this->trace eq 'low') {
        return 1;
    }
    if ($this->trace eq 'med') {
        return 2;
    }
    if ($this->trace eq 'high') {
        return 3;
    }
}

sub printDummyFactory {
    my $this = shift;
    my $body = shift;
    my $factories = shift;
    my $log = shift;

    my $name = $this->name();
    my $logName = (defined $log->options('binlogname')) ? $log->options('binlogname') : $log->name();

    if (not $log->messageField()) {
        if ($log->getLogLevel($this->traceLevel()) > 0) {
            $$body .= "mapper.addFactory(\"${name}_${logName}\", &${logName}DummyFactory);\n";
            $$factories .= "mace::BinaryLogObject* ${logName}DummyFactory() {\n";
            $$factories .= "return new ${logName}Dummy();\n";
            $$factories .= "}\n";
        }
    }
}

sub printDummyInitCCFile {
    my $this = shift;
    my $outfile = shift;
    my $name = $this->name();
    my $body = "";
    my $factories = "";

    $body .= "mapper.addFactory(\"${name}\", &${name}DummyFactory);\n";
    $factories .= "mace::BinaryLogObject* ${name}DummyFactory() {\n";
    $factories .= "return new ${name}Dummy();\n";
    $factories .= "}\n";
    for my $log ($this->structuredLogs()) {
        $this->printDummyFactory(\$body, \$factories, $log);
    }
    if ($this->traceLevel() > 0) {
        for my $log (grep(!($_->name =~ /^(un)?register.*Handler$/), $this->providedMethods()), @{$this->aspectMethods()}, @{$this->usesHandlerMethods()}, @{$this->timerMethods()}, @{$this->usesClassMethods()}, @{$this->providedHandlerMethods()}) {
            $this->printDummyFactory(\$body, \$factories, $log);
        }
    }
    if ($this->traceLevel() > 1) {
        for my $log ($this->routines()) {
            $this->printDummyFactory(\$body, \$factories, $log);
        }
    }

    print $outfile <<END;
#ifndef ${name}_dummy_init_h
#define ${name}_dummy_init_h
    #include "DummyServiceMapper.h"
    #include "${name}.h"
    using mace::DummyServiceMapper;

    namespace ${name}_namespace {
      $factories
      void init() __attribute__((constructor));
      void init() {
        DummyServiceMapper mapper;
END

    print $outfile $body;
    print $outfile <<END;
      }
    }
#endif // ${name}_dummy_init_h
END
}

sub printInitHFile {
    my $this = shift;
    my $outfile = shift;
    my $name = $this->name();

    my @svp = grep( not($_->intermediate or $_->final), $this->service_variables());

    my $r = Mace::Compiler::GeneratedOn::generatedOnHeader("$name init declaration file");
    print $outfile $r;

    print $outfile <<END;
#ifndef ${name}_init_h
#define ${name}_init_h

    #include "${name}-constants.h"
END
    $this->printIncludeBufH($outfile);

    foreach my $scProvided ($this->provides()) {
        print $outfile <<END;
#include "${scProvided}ServiceClass.h"
END
    }
    foreach my $scUsed ($this->service_variables()) {
        print $outfile $scUsed->returnSCInclude();
    }
    print $outfile <<END;

    namespace ${name}_namespace {
END
    foreach my $scProvided ($this->provides()) {
        print $outfile "${scProvided}ServiceClass& new_${name}_$scProvided(".join(", ", (map{my $svline = $_->line(); my $svfile = $_->filename(); qq{\n#line $svline "$svfile"\n}.$_->serviceclass."ServiceClass& ".$_->name." = ".$_->serviceclass."ServiceClass::NULL_\n// __INSERT_LINE_HERE__\n"} @svp), (map{$_->toString()} $this->constructor_parameters()) ).");\n";
	print $outfile "${scProvided}ServiceClass& private_new_${name}_$scProvided(".join(", ", (map{my $svline = $_->line(); my $svfile = $_->filename(); qq{\n#line $svline "$svfile"\n}.$_->serviceclass."ServiceClass& ".$_->name." = ".$_->serviceclass."ServiceClass::NULL_\n// __INSERT_LINE_HERE__\n"} @svp), (map{$_->toString()} $this->constructor_parameters()) ).");\n";

        if ($this->count_constructor_parameters() and scalar(@svp)) {
            print $outfile "${scProvided}ServiceClass& new_${name}_$scProvided(";
            my @p = $this->constructor_parameters();
            my $p1 = shift(@p);
            print $outfile $p1->type->toString()." ".$p1->name.", ";
            print $outfile join(", ", (map{$_->toString()} @p), (map{my $svline = $_->line(); my $svfile = $_->filename(); qq{\n#line $svline "$svfile"\n}.$_->serviceclass."ServiceClass& ".$_->name." = ".$_->serviceclass."ServiceClass::NULL_\n// __INSERT_LINE_HERE__\n"} @svp) );
            print $outfile ");\n";
	    print $outfile "${scProvided}ServiceClass& private_new_${name}_$scProvided(";
	    #my @p = $this->constructor_parameters();
	    #my $p1 = shift(@p);
	    print $outfile $p1->type->toString()." ".$p1->name.", ";
	    print $outfile join(", ", (map{$_->toString()} @p), (map{my $svline = $_->line(); my $svfile = $_->filename(); qq{\n#line $svline "$svfile"\n}.$_->serviceclass."ServiceClass& ".$_->name." = ".$_->serviceclass."ServiceClass::NULL_\n// __INSERT_LINE_HERE__\n"} @svp) );
	    print $outfile ");\n";
        }
    }

    print $outfile <<END;
} //end namespace
#endif // ${name}_init_h
END
}

sub printInitCCFile {
    my $this = shift;
    my $outfile = shift;
    my $name = $this->name();
    my @svo = grep( not($_->intermediate), $this->service_variables());
    my @svp = grep( not($_->intermediate or $_->final), $this->service_variables());

    my $r = Mace::Compiler::GeneratedOn::generatedOnHeader("$name init source file");
    print $outfile $r;

    print $outfile <<END;
  #include "${name}-init.h"
  #include "params.h"
  #include "ServiceConfig.h"
END
    print $outfile join("", map{my $sv=$_->service; qq{#include "$sv-init.h"\n}} grep($_->service && $_->service ne "auto", $this->service_variables()));
    print $outfile <<END;
  namespace ${name}_namespace {
END
    foreach my $scProvided ($this->provides()) {
        my $realMethod = "real_new_${name}_$scProvided";
	print $outfile "${scProvided}ServiceClass& $realMethod(".join(", ", (map{$_->serviceclass."ServiceClass& ".$_->name} @svo), (map{$_->toString("nodefaults" => 1)} $this->constructor_parameters()), "bool ___shared" ).");\n";
        print $outfile "${scProvided}ServiceClass& new_${name}_$scProvided(".join(", ", (map{$_->serviceclass."ServiceClass& _".$_->name} @svp), (map{$_->toString("nodefaults" => 1)} $this->constructor_parameters()) ).") {\n";
	$this->printInitStuff($outfile, $realMethod, 1);
	print $outfile "}\n";
	print $outfile "${scProvided}ServiceClass& private_new_${name}_$scProvided(".join(", ", (map{$_->serviceclass."ServiceClass& _".$_->name} @svp), (map{$_->toString("nodefaults" => 1)} $this->constructor_parameters()) ).") {\n";
	$this->printInitStuff($outfile, $realMethod, 0);
	print $outfile "}\n";

        if ($this->count_constructor_parameters() and scalar(@svp)) {
            print $outfile "${scProvided}ServiceClass& new_${name}_$scProvided(";
            my @p = $this->constructor_parameters();
            my $p1 = shift(@p);
            print $outfile $p1->type->toString()." ".$p1->name.", ";
            print $outfile join(", ", (map{$_->toString("nodefaults"=>1)} @p), (map{$_->serviceclass."ServiceClass& _".$_->name} @svp) );
            print $outfile ") {\n";
	    $this->printInitStuff($outfile, $realMethod, 1);
	    print $outfile "}\n";
	    print $outfile "${scProvided}ServiceClass& private_new_${name}_$scProvided(";
	    #my @p = $this->constructor_parameters();
	    #my $p1 = shift(@p);
	    print $outfile $p1->type->toString()." ".$p1->name.", ";
	    print $outfile join(", ", (map{$_->toString("nodefaults"=>1)} @p), (map{$_->serviceclass."ServiceClass& _".$_->name} @svp) );
	    print $outfile ") {\n";
	    $this->printInitStuff($outfile, $realMethod, 0);
            print $outfile "}\n";
        }

        print $outfile "${scProvided}ServiceClass& configure_new_${name}_$scProvided(bool ___shared) {\n";

        print $outfile "if (___shared) { return new_${name}_$scProvided(); }
                        else { return private_new_${name}_$scProvided(); }\n";
        print $outfile "}\n";
    }
    print $outfile <<END;
  } //end namespace
END
}

sub printInitStuff {
    my $this = shift;
    my $outfile = shift;
    my $realMethod = shift;
    my $shared = shift;
    my $name = $this->name();
    my @svo = grep( not($_->intermediate), $this->service_variables());
    my @svp = grep( not($_->intermediate or $_->final), $this->service_variables());

    for (my $i = $this->count_service_variables()-2; $i >= 0; $i--) {
        my $s1 = $this->service_variables()->[$i];
        my $s1n = $this->service_variables()->[$i]->name();
        my @joins;
        for (my $j = $this->count_service_variables()-1; $j > $i; $j--) {
            my $s2 = $this->service_variables()->[$j];
            my $s2n = $this->service_variables()->[$j]->name();
            if ($s2->service and grep($s1n eq $_, $s2->constructionparams())) {
                if ($s2->final) {
                    @joins = [ 'true' ];
                    last;
                }
                elsif ($s2->intermediate) {
                    push @joins, "later_dep_$s2n";
                }
                else {
                    push @joins, "(&_".$s2n." == &".$s2->serviceclass."ServiceClass::NULL_)";
                }
            }
        }
        if ($s1->intermediate()) {
            if (@joins) {
                print $outfile "const bool later_dep_$s1n = ". join(' || ', @joins) . ";\n";
            }
            else {
                Mace::Compiler::Globals::error("invalid_service", $s1->filename, $s1->line(), "'intermediate' may only be applied to services which are used later in the services block; $s1n is not used later!");
            }
        }
    }

    for my $sv ($this->service_variables()) {
        my $svline = $sv->line();
        my $svfile = $sv->filename();
        my $name = $this->name();
#        if ($sv->service() or $sv->doDynamicCast()) {
	    print $outfile qq{\n#line $svline "$svfile"\n};
	    if ($sv->intermediate) {
		print $outfile $sv->serviceclass."ServiceClass& ${\$sv->name} = (later_dep_${\$sv->name}) ? ".$sv->toNewMethodCall()." : ".$sv->serviceclass."ServiceClass::NULL_;\n";
	    } elsif ($sv->final) { # or $sv->service eq "auto") {
		print $outfile $sv->serviceclass."ServiceClass& ${\$sv->name} = ".$sv->toNewMethodCall($name).";\n";
	    } else {
		print $outfile $sv->serviceclass."ServiceClass& ${\$sv->name} = (&_${\$sv->name} == &".$sv->serviceclass."ServiceClass::NULL_) ? ".$sv->toNewMethodCall($name)." : _${\$sv->name};\n";
	    }
	    print $outfile qq{\n// __INSERT_LINE_HERE__\n};
#	}
#	else {
#	    print $outfile qq{\n#line $svline "$svfile"\n};
#	    print $outfile $sv->serviceclass."ServiceClass& ${\$sv->name} = _${\$sv->name};\n";
#	    print $outfile qq{\n// __INSERT_LINE_HERE__\n};
#	}
    }

    print $outfile "return $realMethod(".join(", ", (map{$_->name} @svo), (map{ "(".$_->name." == ".$_->default()." ? mace::ServiceConfig<void*>::get<".$_->type->type.">(\"${\$this->name}.${\$_->name}\", ${\$_->name}) : ${\$_->name})" } $this->constructor_parameters()), $shared).");\n";

}

sub printMacrosFile {
    my $this = shift;
    my $outfile = shift;
    my $name = $this->name();

    my $r = Mace::Compiler::GeneratedOn::generatedOnHeader("$name macros file");
    print $outfile $r;

    my $undefCurtime = "";
    if ($this->macetime()) {
        #      $undefCurtime = '#undef curtime';
    }

    print $outfile <<END;
#ifndef ${name}_macros_h
#define ${name}_macros_h

#include "lib/mace-macros.h"
$undefCurtime

#define state_change(s) changeState(s, selectorId->log)

END

    for my $m ($this->providedHandlerMethods()) {
        my $fnName = $m->name;
        my $clName = $m->options('class')->[0]->name();
        print $outfile <<END;
    #define typeof_upcall_$fnName $clName
    #define map_typeof_upcall_$fnName map_$clName
END
    }

    print $outfile <<END;

#endif //${name}_macros_h
END

}

sub printComputeAddress() {
    my $this = shift;
    my $outfile = shift;

    if (defined($this->localAddress()) or $this->count_service_variables()) {

        print $outfile "
        MaceKey computeLocalAddress() const {
        ";

        if ($this->localAddress() ne "") {
            print $outfile $this->localAddress();
        } else {
            print $outfile "return downcall_localAddress();";
        }
    }

    print $outfile "
    }
    ";
}

sub array_unique
{
    my %seen = ();
    @_ = grep { ! $seen{ $_ }++ } @_;
}

sub asyncCallHandler {
    my $this = shift;
    my $ptype = shift;

    my $adName = $ptype;
    $adName =~ s/^__async_at/__async_fn/;
    return $adName;
}

sub broadcastCallHandler {
    my $this = shift;
    my $ptype = shift;

    my $adName = $ptype;
    $adName =~ s/^__broadcast_at/__broadcast_fn/;
    return $adName;
}

1;
