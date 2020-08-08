# 
# Context.pm : part of the Mace toolkit for building distributed systems
# 
# Copyright (c) 2012, Wei-Chiu Chuang
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


package Mace::Compiler::Context;

use strict;
use Mace::Compiler::Type;
use Mace::Compiler::Timer;
use Mace::Compiler::Param;
use Mace::Compiler::ContextParam;

use Mace::Util qw(:all);

use constant {
    TYPE_SINGLE           => 0,  
    TYPE_ARRAY            => 1,
    TYPE_MULTIARRAY       => 2,
};

use Class::MakeMethods::Template::Hash
    (
     'new' => 'new',
     'string' => "name",
     'string' => "className",
     'boolean' => 'serialize',
     'array_of_objects' => ["subcontexts" => {class =>"Mace::Compiler::Context"}],
     'array_of_objects' => ["ContextVariables" => { class => "Mace::Compiler::Param" }],
     'array_of_objects' => ["ContextTimers" => { class => "Mace::Compiler::Timer" }],
     'boolean' => "isArray",
     'object' => ["paramType" => { class => "Mace::Compiler::ContextParam" } ],
     'array' => 'downgradeto',
     );

sub isValidRepresentation {
    my $this = shift;
    my $contextStr = shift;   # = 
    # strip leaveonly  context ame

    for my $subcontext ( $this->subcontexts ) {
        $subcontext->isValidRepresentation($contextStr);
    }
}

sub validateContextOptions {
    my $this = shift;
    my $sv = shift;
    $this->serialize(1);

    for my $var ( $this->ContextVariables() ){
        $var->validateTypeOptions({'serialize' => 0, 'recur' => 1});
    }
    for my $timer ( $this->ContextTimers() ){
        $timer->validateTypeOptions($sv);
    }

    for my $subcontext ( $this->subcontexts ) {
        $subcontext->validateContextOptions();
    }
}

sub toSerialize {
  my $this = shift;
  my $str = shift;
  if($this->serialize()) {
    my $name = $this->name();
    my $s;
    if( $this->isArray() ){
        $s = qq/${\$this->name}.serialize($str);/;
    }else{
        $s = qq/${\$this->name}-> serialize($str);/;
    }
    return $s;
  }
  return '';
}

sub toDeserialize {
    my $this = shift;
    my $in = shift;
    my %opt = @_;
    my $prefix = "";
    if($opt{prefix}) { $prefix = $opt{prefix}; }
    if ($this->serialize()) {
        my $name = $this->name();
        my $s;
        if( $this->isArray() ){
            $s = qq/$prefix ${\$this->name}.deserialize($in);/;
        }else{
            $s = qq/$prefix ${\$this->name}-> deserialize($in);/;
        }
        return $s;
    }
    return '';
}

sub toDeclareString {
    my $this = shift;
    my $t = $this->className();
    my $n = $_->name();

    my $r = "";
    if( $this->isArray == 0 ){
        $r .= "mutable ${t} *$n;\n";
    } else {
        $r .= "mutable mace::map<" . $_->paramType->className() . "," . $_->className() . " *, mace::SoftState> " . $_->name() . ";\n";
    }
    return $r;
}
use Data::Dumper;
sub toString {
    my $this = shift;
    my $serviceName = shift;
    my %args = @_;
    my $r = $this->paramType->toString();
    my $n = $this->className();

    my $contextVariableDeclaration = "";
    my $contextTimerDeclaration = "";
    my $contextTimerDefinition = "";
    my $deserializeFields = "";
    my $printcontextVariable = "";
    my $printNodecontextVariable = "";
    my $printcontextTimerDeclaration = "";
    my $printNodecontextTimerDeclaration = "";
    foreach( $this->ContextVariables ){
      $contextVariableDeclaration .= $_->toString(nodefaults => 1, semi => 0) . ";\n";
      my $vn = $_->name();
      $printcontextVariable .= qq/__out<<"$vn="; mace::printItem(__out, &($vn ) ); __out<<", ";\n/;
      $printNodecontextVariable .= qq/mace::printItem( __printer, "$vn", &($vn ) );\n/;
    }
    foreach( $this->ContextTimers ){
        $contextTimerDeclaration .= $_->toString($serviceName, traceLevel => $args{traceLevel}, isContextTimer => 1 ) . ";\n";
      my $vn = $_->name();
      $printcontextTimerDeclaration .= qq/__out<<"$vn="; mace::printItem(__out, &($vn ) ); __out<<", ";\n/;
      $printNodecontextTimerDeclaration .= qq/mace::printItem( __printer, "$vn", &($vn ) );\n/;
    }

      my $serializeFields = 
          join("\n", (grep(/./, map { $_->toSerialize("__str") } $this->ContextVariables()))) . 
          join("\n", map { $_->toSerialize("__str") } $this->ContextTimers());

      $deserializeFields = 
          join("\n", (grep(/./, map { $_->toDeserialize("__in", prefix => "serializedByteSize += ") } $this->ContextVariables()))) . 
          join("\n", map { $_->toDeserialize("__in", prefix => "serializedByteSize += " ) } $this->ContextTimers());
    my $maptype="";
    my $keytype="";
    my $callParams="";
    my $serializeMethods = "";
    if ($this->serialize()) {
        my $serializeBody = qq/
            $serializeFields
        /;
        my $deserializeBody = qq/
              int serializedByteSize = 0;
              $deserializeFields
              return serializedByteSize;
        /;
        $serializeMethods = qq/
              void serialize(std::string& __str) const {
                $serializeBody
              }
              int deserialize(std::istream& __in)  throw (mace::SerializationException){
                $deserializeBody
              }
/;
    }
    # XXX: deep copy do not include child contexts.
    # only do deep copy when there are at least one timers or variables.
    my $deepCopy="";
    my $colon = "";
    if( $this->count_ContextTimers() + $this->count_ContextVariables() > 0 ){
        $deepCopy .= join(",\n", map{ "${\$_->name()}(_ctx.${\$_->name()})" } @{ $this->ContextTimers(),$this->ContextVariables() }   );
        $colon = ":";
    }

    # default value of context variables.

    # XXX WC: I want to support default value for context variables. Unfortunately, some default value is set to constructor parameter,
    # so it gets complicated.
    my $default_param = "";
=begin
    my $default_param = join("", 
      map { 
        my $dv;
        if( $_->hasDefault() ){
          $dv = $_->default();
        }else{
          $dv = "";
        }
        ", ${\$_->name()} ( $dv )"
      } $this->ContextVariables() 
    );
=cut
    $r .= qq#
class ${n} : public mace::ContextBaseClass {
public:
    // add timers declaration
    $contextTimerDefinition
    $contextTimerDeclaration
    // add state var declaration
    $contextVariableDeclaration

    void print(std::ostream& __out) const {
      __out<<"$n(";
      mace::ContextBaseClass::print( __out ); __out<<", ";
      $printcontextVariable
      $printcontextTimerDeclaration
      __out<<")";
    }

    void printNode(mace::PrintNode& __pr, const std::string& name) const {
        mace::PrintNode __printer(name, "${n}");
        const mace::ContextBaseClass* base = static_cast<const mace::ContextBaseClass*>(this);
        mace::printItem(__printer, "ContextBaseClass", base);
        $printNodecontextVariable
        $printNodecontextTimerDeclaration
        __pr.addChild(__printer);
    }
public:
    ${n}(const mace::string& contextName="$this->{name}", const uint64_t createTicketNumber = 1, const uint64_t executeTicketNumber = 1,
        const uint64_t create_now_committing_ticket = 1, const mace::OrderID& execute_now_committing_eventId = mace::OrderID(), 
        const uint64_t execute_now_committing_ticket = 1, const mace::OrderID& now_serving_eventId = mace::OrderID(),
        const uint64_t now_serving_execute_ticket = 1, const bool execute_serving_flag = false, const uint64_t lastWrite = 0, 
        const uint8_t serviceId = 0, const uint32_t contextId = 1, const uint8_t contextType = mace::ContextBaseClass::CONTEXT): 
        mace::ContextBaseClass(contextName, createTicketNumber, executeTicketNumber, create_now_committing_ticket,
        execute_now_committing_eventId, execute_now_committing_ticket, now_serving_eventId, now_serving_execute_ticket,
        execute_serving_flag, lastWrite, serviceId, contextId, contextType) $default_param
    { }
    ${n}( const ${n}& _ctx ) $colon $deepCopy
    { }

    virtual ~${n}() { }
      $serializeMethods

    void snapshot( const uint64_t& ver ) const {
        ${n}* _ctx = new ${n}(*this);
        mace::ContextBaseClass::snapshot( ver, _ctx );
    }
    // get snapshot using the current event id.
    const ${n}& getSnapshot() const {
        return static_cast< const ${n}& >(  mace::ContextBaseClass::getSnapshot()  );
    }

    void setSnapshot(const uint64_t ver, const mace::string& snapshot){
        std::istringstream in( snapshot );
        ${n} *obj = new ${n} (this->contextName,1  );
        mace::deserialize(in, obj);
        versionMap.push_back( std::make_pair(ver, obj) );
    }

private:
};
    #;
    # XXX: if migration takes place, should it take the snapshot of the previous snapshot?
    return $r;
}

sub locateChildContextObj {
    my $this = shift;

    my $subcontextConditionals = join("", map{ $_->locateChildContextObj(   )}$this->subcontexts());
    

    return qq/
    if( contextTypeName == "$this->{name}" ){
      return new $this->{className} ( );
    }
    $subcontextConditionals
    /;
}

=begin
sub locateChildContextObj {
    my $this = shift;
    my $contextDepth = shift;
    my $parentContext = shift;

    my $getContextObj;

    my $declareParams = "";
    my $contextName = $this->{name};
    my $nextContextDepth = $contextDepth+1;
    my $allocateContextObject;
    if( $this->isArray() ) {
        my $keys = $this->paramType->count_key();
        if( $keys  == 1  ){
            my $keyType = ${ $this->paramType->key() }[0]->type->type();
            $getContextObj = qq#
            $keyType keyVal = boost::lexical_cast<$keyType>( ctxStr${contextDepth}[1] );
            contextDebugID = contextDebugIDPrefix+ "$contextName\[" + boost::lexical_cast<mace::string>(keyVal)  + "\]";
            mace::vector< uint32_t > parentContextIDs;
            uint32_t parentID = contextID;
            while( (parentID = snapshotMapping.getParentContextID( parentID ) ) != 0 ){
              parentContextIDs.push_back( parentID );
            }
            
            #;
            $allocateContextObject = "$this->{className}* newctx = new $this->{className} ( contextDebugID, eventID , instanceUniqueID, contextID, parentContextIDs );";
        } elsif ( $keys > 1 ){
            my $paramid=1;
            my @params;
            my @paramid;
            for( @{ $this->paramType->key() } ){
                my $keyType = $_->type->type();
                push @params, "$keyType param$paramid = boost::lexical_cast<$keyType>( ctxStr${contextDepth}[$paramid] )";
                push @paramid, "param$paramid";
                $paramid++;
            }
            my $ctxParamClassName = $this->paramType->className();
            # declare parameters of the _ index
            $declareParams = join(";\n", @params) . ";";
            $getContextObj = qq"
            $ctxParamClassName keyVal(" .join(",", @paramid) . ");
            " . qq#
            contextDebugID = contextDebugIDPrefix+ "$contextName\[" + boost::lexical_cast<mace::string>(keyVal)  + "\]";

            mace::vector< uint32_t > parentContextIDs;
            uint32_t parentID = contextID;
            while( (parentID = snapshotMapping.getParentContextID( parentID ) ) != 0 ){
              parentContextIDs.push_back( parentID );
            }
            
            #;
            $allocateContextObject = "$this->{className}* newctx = new $this->{className} ( contextDebugID, eventID , instanceUniqueID, contextID, parentContextIDs );";
        }
    }else{
        $getContextObj = qq#
            contextDebugID = contextDebugIDPrefix + "${contextName}";

            mace::vector< uint32_t > parentContextIDs;
            uint32_t parentID = contextID;
            while( (parentID = snapshotMapping.getParentContextID( parentID ) ) != 0 ){
              parentContextIDs.push_back( parentID );
            }
        #;
        $allocateContextObject = "$this->{className}* newctx = new $this->{className} ( contextDebugID, eventID , instanceUniqueID, contextID, parentContextIDs );";
    }
    my $subcontextConditionals = join("else ", (map{ $_->locateChildContextObj( $nextContextDepth, "parentContext$contextDepth"  )}$this->subcontexts()), qq/ABORT("Unexpected context name");/);
    # FIXME: need to deal with the condition when a _ is allowed to downgrade to non-subcontexts.
    my $tokenizeSubcontext = "";
    if( $this->count_subcontexts() ){
        $tokenizeSubcontext= qq/
        std::vector<std::string> ctxStr$nextContextDepth;
        boost::split(ctxStr$nextContextDepth, ctxStrs[$nextContextDepth], boost::is_any_of("[,]") ); /;
    }
    my $s = qq/if( ctxStr${contextDepth}[0] == "$this->{name}" ){
        $declareParams
        $getContextObj
        if( ctxStrsLen == $nextContextDepth ){
          $allocateContextObject
          setContextObject( newctx, contextID, contextName );
          return newctx;
        }
        contextDebugIDPrefix = contextDebugID + ".";
        $tokenizeSubcontext
        $subcontextConditionals
    }
    /;
    return $s;
}
=cut
1;
