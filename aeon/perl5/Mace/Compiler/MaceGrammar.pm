# 
# MaceGrammar.pm : part of the Mace toolkit for building distributed systems
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
package Mace::Compiler::MaceGrammar;

use strict;
use Mace::Compiler::Grammar;
use Mace::Compiler::CommonGrammar;

use Data::Dumper;
use constant MACE => Mace::Compiler::CommonGrammar::COMMON . q[

Preamble : FileLine CopyLookaheadString[rule=>'ServiceName'] 
{ $thisparser->{'local'}{'service'}->header("#line ".$item{'FileLine'}->[0]." \"".$item{FileLine}->[1]."\"\n".$item{CopyLookaheadString}."\n// __INSERT_LINE_HERE__\n"); }

PrintError : MFile 
| {
    my $lastLine = "";
    my $lastMsg = "";
    for my $i (@{$thisparser->{errors}}) {
	if ($i->[1] ne $lastLine || $i->[0] ne $lastMsg) {
	    Mace::Compiler::Globals::error('invalid syntax', $thisparser->{local}{filemap}->[$i->[1]], $thisparser->{local}{linemap}->[$i->[1]], $i->[0]);
	    $lastLine = $i->[1];
	    $lastMsg = $i->[0];
	}
    }
    $thisparser->{errors} = undef;
}
MFile : <skip: qr{(\xef\xbb\xbf)?\s* ((/[*] .*? [*]/|(//[^\n]*\n)|([#]line \s* \d+ \s* ["][^"\r\n]+["])) \s*)*}sx> 
        { 
          $thisparser->{'local'}{'service'} = Mace::Compiler::ServiceImpl->new(); 
          $thisparser->{local}{'mincludes'} = {};
        } 
        Preamble Service EOFile 
        { 
          $return = $thisparser->{'local'}{'service'}; 
        }
        | <error>

Service : ServiceName Provides Attributes Registration TraceLevel MaceTime Locking ServiceBlock(s) ...EOFile 
{ 
  $thisparser->{'local'}{'service'}->name($item{ServiceName}); 
  $thisparser->{'local'}{'service'}->provides(@{$item{Provides}}); 
  $thisparser->{'local'}{'service'}->attributes($item{Attributes}); 
  $thisparser->{'local'}{'service'}->registration($item{Registration});
  $thisparser->{'local'}{'service'}->trace($item{TraceLevel});
  $thisparser->{'local'}{'service'}->macetime($item{MaceTime});
  $thisparser->{'local'}{'service'}->locking($item{Locking});
}
        | <error>

ServiceName : /service\s/ Id <commit> ';' 
                  { 
                    if ($item{Id} ne $thisparser->{local}{servicename}) {
                        Mace::Compiler::Globals::error('invalid syntax', $thisparser->{local}{filemap}->[$thisline], $thisparser->{local}{linemap}->[$thisline], "Service name (".$item{Id}.") must match from filename (".$thisparser->{local}{servicename}.")");
                    }
                    $return = $item{Id}; 
                  }
            | /service\b/ <commit> ';'
                  { 
                    my $svName = $thisparser->{local}{servicename};
                    $svName =~ s|^.*/||;
                    $svName =~ s|[.].*||;
                    $return = $svName;
                  }

Provides : /provides\s/ <commit> Id(s /,/) ';'
                  { $return = $item[3]; }
            | 
                  { $return = [ "Null" ]; }

Attributes : /attributes\s/ <commit> Id(s /,/) ';'
                  { $return = join(",",@{$item[3]}); }
            | 
                  { $return = ""; }

RegType : 'dynamic' '<' ScopedId '>' {$return = $item{ScopedId}->toString();}
            | /static\b/ {$return = "";}
            | /unique\b/ {$return = "unique";}
Registration : /registration\b/ <commit> '=' RegType ';' { $return = $item{RegType}; }
            |  { $return = ""; }

Level : 'off' | 'manual' | 'low' | 'high' | 'medium' { $return='med'; } | 'med' | <error>
TraceLevel : 'trace' <commit> '=' Level ';' { $return = $item{Level}; } | { $return = 'manual'; }

MaceTimeLevel : /uint64_t\b/ | /MaceTime\b/ | <error>
MaceTime : 'time' <commit> '=' MaceTimeLevel ';' { $return = $item{MaceTimeLevel} eq "MaceTime"; } | { $return = 0; }

LockingType : /(write|on|global)\b/ { $return = 1; } | /read|anonymous|anon\b/ { $return = 0; } | /off|none|null\b/ { $return = -1; } | <error>
Locking : 'locking' <commit> '=' LockingType ';' { $return = $item{LockingType}; } | { $return = 1; }

ServiceBlockName : 'log_selectors' | 'constants' | 'services'|
                   'constructor_parameters' | 'transitions' | 'routines' |
                   'states' | 'auto_types' | 'typedefs' | 'state_variables' |
                   'method_remappings' | 'messages' | 'properties' | 'contexts'
                   'detect' | 'structured_logging' | 'queries' | 'local_address' | <error>

ServiceBlock : ServiceBlockName Block[rule => $item{'ServiceBlockName'}] 

Filename : /[^"\n\t]+/
#" -- ignore this line!
Block : '{' <matchrule: $arg{'rule'}> '}' (';')(?)
        { Mace::Compiler::Globals::msg("PARSE_BLOCK", $thisparser->{local}{filemap}->[$thisline], $thisparser->{local}{linemap}->[$thisline], "Matched block $arg{'rule'}"); $thisparser->{local}{$arg{'rule'}} = $item[2]; }
      | <error>

local_address : StartPos FileLineEnd SemiStatement(s) EndPos ...'}'
{
    my $startline = "";
    my $endline = "";
    $startline = "\n#line ".$item{FileLineEnd}->[0]." \"".$item{FileLineEnd}->[1]."\"\n";
    $endline = "\n// __INSERT_LINE_HERE__\n";
    $thisparser->{local}{service}->localAddress($startline.substr($Mace::Compiler::Grammar::text, $item{StartPos}, 1 + $item{EndPos} - $item{StartPos}).$endline);
}

log_selectors : <reject: do{$thisparser->{local}{update}}> Selector(s?) ...'}' 
             <defer: Mace::Compiler::Globals::warning('deprecated', $thisparser->{local}{filemap}->[$thisline], $thisparser->{local}{linemap}->[$thisline], "The log_selectors block is deprecated since post-processing tools make assumptions about the format of the log_selectors.")> 
SelectorName : 'default' | 'message' | <error>
Selector : <rulevar: $selname = ""> 
Selector : SelectorName { $selname = $item{SelectorName}; } '=' QuotedString ';' { $thisrule->{'local'}{'selector_'.$item{'SelectorName'}}++; } <commit> <reject: do{$thisrule->{'local'}{'selector_'.$item{'SelectorName'}} > 1}>
{
  $thisparser->{'local'}{'service'}->selectors($item{SelectorName}, $item{QuotedString});
}
      | <error?: Cannot define selector string for "$selname" more than once> <error>

ConstantsIncludes : '#include' /.*\n/ { $return = $item[1]." ".$item[2]; }
constants : ConstantsIncludes(s?) Parameter[mustinit => 1, semi => 1](s?) ...'}' 
{
    $thisparser->{'local'}{'service'}->push_constants_includes(@{$item[1]});
    $thisparser->{'local'}{'service'}->push_constants(@{$item[2]});
}

PLinesParameter : PLines Parameter[%arg] PLines {$return = $item{Parameter}; }

constructor_parameters : <reject: do{$thisparser->{local}{update}}> Parameter[mustinit => 1, semi => 1](s?) ...'}' 
{ 
  $thisparser->{'local'}{'service'}->constructor_parameters(@{$item[2]});
}

states : <reject: do{$thisparser->{local}{update}}> StateName(s?) ...'}' 
{
  $thisparser->{'local'}{'service'}->states(@{$item[2]});
}
StateName : <rulevar: $id = ""> 
          | Id <commit> { $id = $item{'Id'}; $thisrule->{'local'}{$item{'Id'}}++; } <reject: do{$item{'Id'} eq "init" ||  $thisrule->{'local'}{$item{'Id'}} > 1}> <uncommit> Id(?) ';' { $return = $item{Id}; }
          | <error?: Invalid state '$id'.  States cannot be reused nor be 'init'.> <error>

services : <reject: do{$thisparser->{local}{update}}> ServiceUsed(s?) ...'}'
{
  $thisparser->{'local'}{'service'}->service_variables(@{$item[2]});
}
InlineFinal : /inline\b/ { $return = 2; } | /final\b/ /raw\b/ { $return = 3; } | /final\b/ { $return = 1; } | /raw\b/ { $return = 4; } | { $return = 0; }
HandlerList : '[' Id(s? /,/) ']' { $return = $item[2]; } | '[' '*' ']' { $return = -1; } | { $return = -1; }
#RegistrationUid : '::' <commit> (ScopedId|Number) { $return = $item[3]; } | <error?> <reject> | { $return = "-1"; }
RegistrationUid : '::' ScopedId
    { $return = $item{ScopedId}->toString(); } 
| '::' Number
    { $return = $item{Number}; } 
| <error?> <reject> | { $return = "-1"; }
DynamicRegistration : '<' <commit> Type '>' { $return = $item{Type}->toString() } | <error?> <reject> | { $return = ""; }
#XXX: Use more intelligent service name checking? -- as in existance of file
SharedOrPrivate : 'shared' { $return = 1; } | 'private' { $return = 0; }
ServiceUsed : FileLine InlineFinal Id HandlerList Id RegistrationUid DynamicRegistration '=' FileLine 'auto' <commit> '(' SharedOrPrivate ',' '[' Expression(s? /,/) ']' ',' '[' Expression(s? /,/) ']' ')' ';' 
{ 
  $return = Mace::Compiler::ServiceVar->new(name => $item[5], serviceclass => $item[3], service => $item[10], defineLine => $item[9]->[0], defineFile => $item[9]->[1], line => $item[1]->[0], filename => $item[1]->[1], intermediate => ($item{InlineFinal}==2?1:0), final => (($item{InlineFinal}%2)==1?1:0), raw => ($item{InlineFinal} == 4 || $item{InlineFinal} == 3)?1:0, registrationUid => $item{RegistrationUid}, registration => $item{DynamicRegistration}, autoshared => $item{SharedOrPrivate});

  if (scalar($item{HandlerList}) == -1) {
    $return->allHandlers(1);
  } else {
    $return->allHandlers(0);
    $return->handlerList(@{$item{HandlerList}});
  }
  $return->push_autoattr(map { $_->toString() } @{$item[16]});
  #$return->push_autoattr(@{$item[16]});
  $return->push_autooptattr(map { $_->toString() } @{$item[20]});
  #$return->push_autooptattr(@{$item[20]});
}
        | FileLine InlineFinal Id HandlerList Id RegistrationUid DynamicRegistration '=' FileLine Id '(' <commit> Expression(s? /,/) ')' ';' 
{ 
  $return = Mace::Compiler::ServiceVar->new(name => $item[5], serviceclass => $item[3], service => $item[10], defineLine => $item[9]->[0], defineFile => $item[9]->[1], line => $item[1]->[0], filename => $item[1]->[1], intermediate => ($item{InlineFinal}==2?1:0), final => (($item{InlineFinal}%2)==1?1:0), raw => ($item{InlineFinal} == 4 || $item{InlineFinal} == 3)?1:0, registrationUid => $item{RegistrationUid}, registration => $item{DynamicRegistration});
  $return->constructionparams(map { $_->toString() } @{$item[13]});
  #$return->constructionparams(@{$item[13]});
  if(scalar($item{HandlerList}) == -1) {
    $return->allHandlers(1);
  } else {
    $return->allHandlers(0);
    $return->handlerList(@{$item{HandlerList}});
  }
}
        | FileLine InlineFinal Id HandlerList Id RegistrationUid DynamicRegistration '=' <commit> FileLine Id ';' 
{ 
  $return = Mace::Compiler::ServiceVar->new(name => $item[5], serviceclass => $item[3], dynamicCastSource => $item[11], defineLine => $item[10]->[0], defineFile => $item[10]->[1], line => $item[1]->[0], filename => $item[1]->[1], intermediate => ($item{InlineFinal}==2?1:0), final => (($item{InlineFinal}%2)==1?1:0), raw => ($item{InlineFinal} == 4 || $item{InlineFinal} == 3)?1:0, registrationUid => $item{RegistrationUid}, doDynamicCast => 1, registration => $item{DynamicRegistration});
  if(scalar($item{HandlerList}) == -1) {
    $return->allHandlers(1);
  } else {
    $return->allHandlers(0);
    $return->handlerList(@{$item{HandlerList}});
  }
}
        | FileLine InlineFinal Id HandlerList Id RegistrationUid DynamicRegistration ';' <commit> 
{ 
  $return = Mace::Compiler::ServiceVar->new(name => $item[5], serviceclass => $item[3], line => $item[1]->[0], filename => $item[1]->[1], intermediate => ($item{InlineFinal}==2?1:0), final => (($item{InlineFinal}%2)==1?1:0), raw => ($item{InlineFinal} == 4 || $item{InlineFinal} == 3)?1:0, registrationUid => $item{RegistrationUid}, registration => $item{DynamicRegistration});
  if(scalar($item{HandlerList}) == -1) {
    $return->allHandlers(1);
  } else {
    $return->allHandlers(0);
    $return->handlerList(@{$item{HandlerList}});
  }
}
        | <error>

typedefs : TypeDef(s?) ...'}'
        | <error>

TypeDef : /typedef\s/ Type FileLine Id ';'
{
    $thisparser->{'local'}{'service'}->push_typedefs(Mace::Compiler::TypeDef->new(name=>$item{Id}, type=>$item{Type}, line => $item{FileLine}->[0], filename => $item{FileLine}->[1]));
}
        | <error>

#XXX: Add in framework for checking specific options

contexts : ContextDeclaration[isGlobal=>$arg{isGlobal}]
{
    if( defined $item{ContextDowngrade} ){
        for ($item{ContextDowngrade}) {
            $item{ContextDeclaration}->push_downgradeto( $_ );
        }
    }
    #if( $arg{ isGlobal } == 1 ){
    #    $thisparser->{'local'}{'service'}->push_contexts($item{ContextDeclaration});
    #} else {
        $return = {type => 2, object => $item{ContextDeclaration} }; 
    #}
}
| ContextRelation
{

}
| <error>   

ContextRelation : ContextCondition '(' ContextConditionDef ')'
{

}
ContextCondition: /[_a-zA-Z][a-zA-Z0-9_]*/
{

}
ContextConditionDef:/[_a-zA-Z][a-zA-Z0-9_]*/
{

}

state_variables : Variable[isGlobal => 1, ctxKeys=>() ](s?) ...'}' 
{
    my $context = Mace::Compiler::Context->new(name => "globalContext",className =>"global_Context", isArray => 0);
    my $contextParamType = Mace::Compiler::ContextParam->new(className => "" );
    $context->paramType( $contextParamType );
    foreach ( @{$item{"Variable(s?)"} } ) {
        if ( $_->{type} == 1 ) {
            #push( @timers, $_->{object} );
        } elsif ( $_->{type} == 2 ) {
            $context->push_subcontexts( $_->{object}  );
        } elsif ( $_->{type} == 3 ) {
            $context->push_ContextVariables( $_->{object}  );
        } else {
            # complain
        }
    }
    my $commContext = Mace::Compiler::Context->new(name => "externalCommContext",className =>"externalComm_Context", isArray => 0);
    my $commContextParamType = Mace::Compiler::ContextParam->new(className => "" );
    $commContext->paramType( $commContextParamType );

    $context->push_subcontexts($commContext);
    $thisparser->{'local'}{'service'}->push_contexts($context);
}

ContextDowngrade: 'downgradeto' ContextName
{
    $return = $item{ContextName};
}

Variable : .../timer\b/ <commit> Timer[isGlobal=>$arg{isGlobal}, ctxKeys=> (defined $arg{ctxKeys})?  $arg{ctxKeys }:()]
{

    $return = {type => 1, object => $item{Timer} };
}| .../\bcontext\b/ <commit>  ContextDeclaration[isGlobal=>$arg{isGlobal}, ctxKeys=> (defined $arg{ctxKeys})? $arg{ctxKeys }:()  ] ContextDowngrade(?)
{
    if( defined $item{ContextDowngrade} ){
        for ($item{ContextDowngrade}) {
            $item{ContextDeclaration}->push_downgradeto( $_ );
        }
    }
    #if( $arg{ isGlobal } == 1 ){
        #$thisparser->{'local'}{'service'}->push_contexts($item{ContextDeclaration});
        #$thisparser->{'local'}{'service'}->global_contextpush_contexts($item{ContextDeclaration});
    #} else {
        $return = {type => 2, object => $item{ContextDeclaration} }; 
    #}
    

}| StateVar[isGlobal=>$arg{isGlobal}]
{
    $return = {type => 3, object => $item{StateVar} };
}| <error>

Timer : 'timer' TimerTypes Id TypeOptions[typeopt => 1] ';'
{

  my $timer = Mace::Compiler::Timer->new(name => $item{Id});
  $timer->typeOptions(@{$item{TypeOptions}});
  my @timerTypes = @{$item{TimerTypes}};
  foreach( @{$arg{ctxKeys}} ){
    push @timerTypes,$_->type;
  }
  $timer->types( @timerTypes );

  # specify contextTimer
  if( defined $arg{ctxKeys} ) {
      $timer->isContextTimer(1);
  } else {
      $timer->isContextTimer(0);
  }
  
  # add the timer into global context no matter what.
  $thisparser->{'local'}{'service'}->push_timers($timer);

  $return = $timer;
}
TimerTypes : '<' <commit> Type(s /,/) '>' { $return = $item[3]; } | { $return = []; }

StateVar : Parameter[typeopt => 1, arrayok => 1, semi => 1]
{
  if( $arg{ isGlobal } == 1 ){
      #$thisparser->{'local'}{'service'}->push_global_context($item{Parameter});
      #$thisparser->{'local'}{'service'}->push_state_variables($item{Parameter});
  }
  $return = $item{Parameter};
}

ContextDeclaration : 'context' <commit> ContextName ...'{' ContextBlock[ ctxKeys=> (defined $arg{ctxKeys})?[  @{ $item{ContextName}->{keyType} } ,  @{ $arg{ctxKeys} }  ]:  $item{ContextName}->{keyType}    ]
{
    my $name = $item{ContextName}->{name};
    my $isArray = $item{ContextName}->{isArray};
    my $context = Mace::Compiler::Context->new(name => $name,className =>"__${name}__Context", isArray => $isArray);
    my $contextParamType = Mace::Compiler::ContextParam->new(className => "" );

    if( $isArray ){
        if( scalar ( @{ $item{ContextName}->{keyType} } ) == 1 ){
            # if only one key, use the name of the key as the className.
            $contextParamType->className(  ${ $item{ContextName}->{keyType} }[0]->type->type()  );
            $contextParamType->key( @{ $item{ContextName}->{keyType} } );
        }else{
            $contextParamType->key( @{ $item{ContextName}->{keyType} } );
            $contextParamType->className("__${name}__Context__param"  );
        }
    }

    $context->paramType( $contextParamType );
    if( @{ $item{ContextBlock}->{timers} } > 0 ) {
        #$context->push_ContextTimers( @{$item{ContextBlock}->{timers} } );
    }
    if( @{ $item{ContextBlock}->{variables} } > 0 ) {
        $context->push_ContextVariables( @{$item{ContextBlock}->{variables} } );
    }
    if( @{ $item{ContextBlock}->{subcontexts} } > 0 ) {
        $context->push_subcontexts( @{ $item{ContextBlock}->{subcontexts} } );
    }

    $return = $context;
}

Parameters : '<' Parameter[typeopt => 1, arrayok => 0, semi => 0] ( ',' Parameter[typeopt => 1, arrayok => 0, semi => 0] )(s?) '>'
{
    my @contextParams = ($item[2], @{ $item[3]} );

    $return = \@contextParams;
}

# support more than multidimensional contexts
ContextName : /[_a-zA-Z][a-zA-Z0-9_]*/  Parameters(?)
{
    my %contextData = ();

    if( scalar @{ $item{q{Parameters(?)}} } ){
        $contextData{ isArray  }= 1;

        $contextData{ keyType  } = ${ $item{q{Parameters(?)} }  }[0];
    } else {
        $contextData{ isArray  }= 0;
        $contextData{ keyType  } = []; # empty array
    }
    $contextData{ name  }= $item[1];

    $return = \%contextData;
}

# chuangw: define a Context object

ContextBlock : '{' ContextVariables[ctxKeys=>  $arg{ctxKeys}  ] '}'
{
#    use Data::Dumper;
    #print "ContextBlock" . $arg{ctxKeys}->name . "\n";
    #if( defined $arg{ctxKeys} ){
    #    foreach( @{ $arg{ctxKeys} } ){
    #        print "ContextBlock:" . $_->type->type . ":" . $_->name . "\n";
    #    }
    #}else{
    #    print "Contextblock: (global)\n";
    #}
    #print "in ContextBlock:" . Dumper( $arg{ctxKeys} ) . "\n";
    #print "in ContextBlock:" .  $arg{ctxKeys}  . "\n";
    $return = $item{ContextVariables};
}| <error>


ContextVariables : Variable[isGlobal => 0, ctxKeys=>  $arg{ctxKeys}   ](s?) ...'}' 
{
    my %vars = ();
    my @timers = ();
    my @variables = ();
    my @subcontexts = ();
    foreach ( @{$item[1]} ) {
        if ( $_->{type} == 1 ) {
            push( @timers, $_->{object} );
        } elsif ( $_->{type} == 2 ) {
            push( @subcontexts, $_->{object} );
        } elsif ( $_->{type} == 3 ) {
            push( @variables, $_->{object} );
        } else {
            # complain
        }
    }
    $vars{ timers } = \@timers;
    $vars{ variables } = \@variables;
    $vars{ subcontexts } = \@subcontexts;

    $return = \%vars;
}

auto_types : AutoType(s?) ...'}' | <error>

messages : Message(s?) ...'}' | <error>
Message : FileLine Id TypeOptions[typeopt => 1] '{' Parameter[declareonly => 1, typeopt => 1, semi => 1](s?) '}' (';')(?) 
{ 
  my $at = Mace::Compiler::AutoType->new(name => $item{Id}, line => $item{FileLine}->[0], filename => $item{FileLine}->[1], method_type => Mace::Compiler::AutoType::FLAG_NONE);
  $at->typeOptions(@{$item{TypeOptions}});
  $at->fields(@{$item[5]});
  $thisparser->{'local'}{'service'}->push_messages($at);
}
| <error>

method_remappings : RemappingsSection(s?) ...'}' | <error>
RemappingsSection : RemappingsSectionNames Block[rule=>$item{RemappingsSectionNames}] | <error>
RemappingsSectionNames : 'uses' | 'implements' | <error>

ParseMethodRemapping : '(' Parameter[%arg](s? /,/) ')' ';' | <error>

uses : UsesMethod(s?) ...'}'

UsesMethod : UsesToken Method[noReturn => 1, noIdOk => 1, mapOk => $item{UsesToken}, usesOrImplements => "uses", locking => ($thisparser->{'local'}{'service'}->locking())]
{
    #print "from UsesMethod: parsing " . $item{Method}->toString(noline => 1) . "\n";
    if($item{UsesToken} eq "up") {
	if (grep { $item{Method}->eq($_, 1) } $thisparser->{'local'}{'service'}->usesUpcalls()) {
	    unless ($thisparser->{local}{update}) {
		die "duplicate remapping";
	    }
	}
	else {
            if($thisparser->{'local'}{'update'}) {
              $item{Method}->options('mhdefaults',1);
              my $arr = $thisparser->{'local'}{'service'}->usesUpcalls();
              my $i = scalar(grep($_->options('mhdefaults'), @$arr));
              splice(@$arr, $i, 0, $item{Method});
              1;
            } else {
              $thisparser->{'local'}{'service'}->push_usesUpcalls($item{Method});
            }
	}
    } else {
	if (grep { $item{Method}->eq($_, 1) } $thisparser->{'local'}{'service'}->usesDowncalls()) {
	    unless ($thisparser->{local}{update}) {
		die "duplicate remapping";
	    }
	}
	else {
            if($thisparser->{'local'}{'update'}) {
              $item{Method}->options('mhdefaults',1);
              my $arr = $thisparser->{'local'}{'service'}->usesDowncalls();
              my $i = scalar(grep($_->options('mhdefaults'), @$arr));
              splice(@$arr, $i, 0, $item{Method});
              1;
            } else {
              $thisparser->{'local'}{'service'}->push_usesDowncalls($item{Method});
            }
	}
    }
}
           | <error>
UsesToken : 'upcall_' { $return = "up" } | 'downcall_' { $return = "down" }

implements : ImplementsSection(s?) ...'}' | <error>
ImplementsSection : ImplementsBlockNames Block[rule=>$item{ImplementsBlockNames}] | <error>
ImplementsBlockNames : "upcalls" | "downcalls" | <error>

upcalls : Upcall(s?) ...'}' | <error>
Upcall : Method[noReturn => 1, noIdOk => 1, mapOk => "up", usesOrImplements => "implements", locking => ($thisparser->{'local'}{'service'}->locking())] 
{
    if (grep { $item{Method}->eq($_, 1) } $thisparser->{'local'}{'service'}->implementsUpcalls()) {
	unless ($thisparser->{local}{update}) {
	    die "duplicate remapping";
	}
    }
    else {
        if($thisparser->{'local'}{'update'}) {
          $item{Method}->options('mhdefaults',1);
          my $arr = $thisparser->{'local'}{'service'}->implementsUpcalls();
          my $i = scalar(grep($_->options('mhdefaults'), @$arr));
          splice(@$arr, $i, 0, $item{Method});
          1;
        } else {
          $thisparser->{'local'}{'service'}->push_implementsUpcalls($item{Method});
        }
    }
}
| <error>
downcalls : Downcall(s?) ...'}' | <error>
Downcall : Method[noReturn => 1, noIdOk => 1, mapOk => "down", usesOrImplements => "implements", locking => ($thisparser->{'local'}{'service'}->locking())] 
{
    if (grep { $item{Method}->eq($_, 1) } $thisparser->{'local'}{'service'}->implementsDowncalls()) {
	unless ($thisparser->{local}{update}) {
	    die "duplicate remapping";
	}
    }
    else {
        if($thisparser->{'local'}{'update'}) {
          $item{Method}->options('mhdefaults',1);
          my $arr = $thisparser->{'local'}{'service'}->implementsDowncalls();
          my $i = scalar(grep($_->options('mhdefaults'), @$arr));
          splice(@$arr, $i, 0, $item{Method});
          1;
        } else {
          $thisparser->{'local'}{'service'}->push_implementsDowncalls($item{Method});
        }
    }
}
| <error>

routines : ( 
              ContextScopeDesignation Method[staticOk => 1, locking => ($thisparser->{'local'}{'service'}->locking()),  context => ( keys( %{$item{ContextScopeDesignation}}) == 0)?"":$item{ContextScopeDesignation}->{context}, snapshot => ( keys( %{$item{ContextScopeDesignation}}) == 0)?():$item{ContextScopeDesignation}->{snapshot}, arrayok => 1] { $thisparser->{'local'}{'service'}->push_routines($item{Method}); }
            | RoutineObject 
           )(s?) ...'}' | <error>
RoutineObject : ObjectType Id MethodTermFoo ';' 
{
    #RoutineObject : <defer: Mace::Compiler::Globals::warning('deprecated', $thisparser->{local}{filemap}->[$thisline], $thisparser->{local}{linemap}->[$thisline], "Objects in routines blocks deprecated.  Use the new (not-yet-implemented) types block")> ObjectType Id MethodTerm ';' 
  #XXX -- MethodTerm instead of braceblock since it returns the string.
  my $o = Mace::Compiler::RoutineObject->new(name => $item{Id}, type => $item{ObjectType}, braceblock => $item{MethodTermFoo});
  $thisparser->{'local'}{'service'}->push_routineObjects($o);
}
| <error>
ObjectType : /class\b/ { $return = 'class'; } | /struct\b/ { $return = 'struct'; } | <error>

transitions : '}' <commit> <reject> 
| ( 
    ...TransitionType <commit> Transition 
  | ...'(' <commit> GuardBlock 
  | <error> 
  )(s) ...'}' FileLine { $thisparser->{'local'}{'service'}->transitionEnd($item{FileLine}->[0]); $thisparser->{'local'}{'service'}->transitionEndFile($item{FileLine}->[1]); }
| <error?:Services must contain at least one transition!> <error>

GuardBlock : <commit> 
             #<defer: Mace::Compiler::Globals::warning('deprecated', $thisline, "Bare block state expressions are deprecated!  Use as-yet-unimplemented 'guard' blocks instead!")> 
             '(' FileLine Expression ')' <uncommit> { $thisparser->{'local'}{'service'}->push_guards(Mace::Compiler::Guard->new(
                                                                                                                       'type' => "expr",
                                                                                                                       'expr' => $item{Expression},
                                                                                                                       'guardStr' => $item{Expression}->toString(),
                                                                                                                       'line' => $item{FileLine}->[0],
                                                                                                                       'file' => $item{FileLine}->[1],
                                                                                                                       )) }
             '{' transitions '}' 
                 {
                    $thisparser->{'local'}{'service'}->pop_guards($item{Expression}->toString()); 
                 }
#             '{' transitions '}' { $thisparser->{'local'}{'service'}->pop_guards($item{Expression}) }
           | <error?> { $thisparser->{'local'}{'service'}->pop_guards(); } <error>
StartCol : // { $return = $thiscolumn; }

ContextAlias: 'as' /[_a-zA-Z][a-zA-Z0-9_]*/ 
{
    #print "ContextAlias=" . $item[2]. "\n";
    $return = $item[2];
}

SnapshotContext: ',' ContextScope ContextAlias(?)
{
    my %snapshot = ();
    #if( @{ $item{ContextAlias} } == 1 ){
    if( @{  $item[3] }==1  ){
        #$snapshot{ $item{ContextScope} } = $item{ContextAlias}[0];
        $snapshot{ $item{ContextScope} } = $item[3][0];
    }else{
        $snapshot{ $item{ContextScope} } = $item{ContextScope};
    }
    #print "SnapshotContext:" . Dumper(%snapshot) . "\n";
    #$return = $item{ContextScope};
    $return = \%snapshot;
}

#ContextScopeDesignation : '[' <commit> ContextScope (',' ContextScope )(s?)  ']'
ContextScopeDesignation : '[' <commit> ContextScope SnapshotContext(s?)  ']'
{
    my %methodContext = ();
    my %snapshotContext = ();
    #foreach( @{ $item[4] } ){
    #print "item{SnapshotContext} = " . Dumper ( @{ $item{SnapshotContext} } ) . "\n";
    #foreach( @{ $item{SnapshotContext} } ){
    foreach( @{ $item[4] } ){
        #push @snapshotContext, $_;
        while( my ($k,$v) = each %{ $_ } ){
            #print "ContextScopeDesignation: k=$k, v=$v\n";
            $snapshotContext{ $k } = $v;
        }
    }
    #$methodContext{ context  } = $item[3];
    $methodContext{ context  } = $item{ContextScope};
    #$methodContext{ snapshot } = \@snapshotContext;
    $methodContext{ snapshot } = \%snapshotContext;
    #print Dumper( $methodContext{snapshot} );
    #print Dumper( %methodContext ) . "\n";
    
    $return = \%methodContext;
}|
{
    my %methodContext = ();
    $return = \%methodContext;
}

ContextScope : ContextScopeName ('::' ContextScopeName)(s?)
{
    $return = $item[1];
    foreach( @{ $item[2] } ){
        $return .= "::" . $_;
    }
    1;
}

ContextScopeName : /[_a-zA-Z][a-zA-Z0-9_]*/ ContextCellName(?)
{
    #print  $item[1]  . "\n";
    #print "ContextScopeName: " . $item[1] . ${$item[2]}[0] . "\n";
    if( @{$item[2]} == 0 ) { # in case the context is not defined as an array
        $return = $item[1];
    }else{
        $return = $item[1] . ${$item[2]}[0];
    }
}

#ContextCellName  : '<' /[_a-zA-Z][a-zA-Z0-9_]*/ '>'
ContextCellName  : '<' ContextCellParam (',' ContextCellParam )(s?) '>'
{
    #print "-->" . $item[2] . "\n";
    if( scalar( @{ $item[3] }  ) == 0 ){
        $return = $item[1] . $item[2] . $item[4];
    }else{
        $return = $item[1] . $item[2] . "," . join(",", @{ $item[3] } ) . $item[4];
    }
    1;
}

ContextCellParam : /[_a-zA-Z][a-zA-Z0-9_.\(\)]*/

#Transition : StartPos StartCol TransitionType FileLine ContextScopeDesignation StateExpression Method[noReturn => 1, typeOptional => 1, locking => ($thisparser->{'local'}{'service'}->locking())] 
Transition : StartPos StartCol TransitionType FileLine ContextScopeDesignation StateExpression Method[noReturn => 1, typeOptional => 1, locking => ($thisparser->{'local'}{'service'}->locking()), context => ( keys( %{$item{ContextScopeDesignation}}) == 0)?"":$item{ContextScopeDesignation}->{context}, snapshot => ( keys( %{$item{ContextScopeDesignation}}) == 0)?():$item{ContextScopeDesignation}->{snapshot}] 
{ 
  my $transitionType = $item{TransitionType};
  my $transitionContext="";
  my $transitionSnapshotContext="";
  if( scalar %{$item{ContextScopeDesignation} } ne 0 ){ # if the returned hash is not empty
        $transitionContext = $item{ContextScopeDesignation}->{context};
        $transitionSnapshotContext = $item{ContextScopeDesignation}->{snapshot};
        #print $item{Method}->name() . " context= " . $transitionContext . "\n";
        #print $item{Method}->name() . " snapshotcontext= " . $transitionSnapshotContext . "\n";
  }


#print "MaceGrammar.pm: transitionContext = $transitionContext, StartPos=" . $item{StartPos}. "\n";

  if(ref ($item{TransitionType})) {
    $transitionType = "aspect";
    #TODO: Resolve how to handle aspects with same name but different parameters, and same parameters but different name.
  }
  #my $t = Mace::Compiler::Transition->new(name => $item{Method}->name(), startFilePos => ($thisparser->{local}{update} ? -1 : $item{StartPos}), columnStart => $item{StartCol}, type => $transitionType, method => $item{Method}, context => $transitionContext, snapshotContext => $transitionSnapshotContext );
  my $t = Mace::Compiler::Transition->new(name => $item{Method}->name(), startFilePos => ($thisparser->{local}{update} ? -1 : $item{StartPos}), columnStart => $item{StartCol}, type => $transitionType, method => $item{Method} );
  $t->guards(@{$thisparser->{'local'}{'service'}->guards()});
  $t->push_guards(Mace::Compiler::Guard->new(
                                             'type' => "state_expr",
                                             'state_expr' => $item{StateExpression},
                                             'guardStr' => $item{StateExpression}->toString(),
                                             'line' => $item{FileLine}->[0],
                                             'file' => $item{FileLine}->[1],)); #Deprecated
  if(ref ($item{TransitionType})) {
    $t->options('monitor',$item{TransitionType});
  }
  if ($thisparser->{'local'}{'indetect'}) {
      push(@{$thisparser->{'local'}{'detect'}}, $t);
  }
  else {
      $thisparser->{'local'}{'service'}->push_transitions($t);
      #print Dumper ($t);
  }
} 
| <error>
TransitionType : /downcall\b/ | /upcall\b/ | /raw_upcall\b/ | /scheduler\b/ | /async\b/ | /broadcast\b/ | /aspect\b/ <commit> '<' Id(s /,/) '>' { $return = $item[4] } | <error>
StateExpression : #<defer: Mace::Compiler::Globals::warning('deprecated', $thisline, "Inline state expressions are deprecated!  Use as-yet-unimplemented 'guard' blocks instead!")> 
    '(' <commit> Expression ')' 
    {
        $return = Mace::Compiler::ParseTreeObject::StateExpression->new(type=>"expr", expr=>$item{Expression});
    } 
    # { $return = $item{Expression} } 
    | <error?> <reject>
#    | { $return = "true"; }
    |
    {
        $return = Mace::Compiler::ParseTreeObject::StateExpression->new(type=>"null");
#        $return = "true";
    }


Update : { $thisparser->{local}{update} = 1; } MaceBlock[%arg](s)
UpdateWithErrors : <skip: qr{\s* ((/[*] .*? [*]/|(//[^\n]*\n)) \s*)*}sx> { $thisparser->{local}{update} = $arg[1]; } ServiceBlock(s?) EOFile { 1 }
| {
    my $lastLine = "";
    my $lastMsg = "";
    for my $i (@{$thisparser->{errors}}) {
	if ($i->[1] ne $lastLine || $i->[0] ne $lastMsg) {
	    Mace::Compiler::Globals::error('invalid syntax', $thisparser->{local}{filemap}->[$i->[1]], $thisparser->{local}{linemap}->[$i->[1]], $i->[0]);
	    $lastLine = $i->[1];
	    $lastMsg = $i->[0];
	}
    }
    $thisparser->{errors} = undef;
}

MaceBlock : /mace\b/ "$arg{type}" '{' MaceBlockBody '}' | /mace\b/ ('service'|'provides') BraceBlockFoo

MaceBlockBody : ServiceBlock(s) ...'}'

structured_logging : Method[noReturn => 1, locking => ($thisparser->{'local'}{'service'}->locking())](s?) {
    $thisparser->{'local'}{'service'}->push_structuredLogs(@{$item[1]});
}

queryblock: FileLineEnd realquery {
    $return = Mace::Compiler::Query->new(text => $item[2],
					 filename => $item[1]->[1],
					 line => $item[1]->[0],
					 );
}

realquery: /[^{}]+{/ realquery(s?) '}' {
    $return = $item[1];
    for my $v (@{$item[2]}) {
	$return .= $v;
    }
    $return .= $item[3];
}
| /[^{}]+;/ {
    $return = $item[1]; 
}

queries: queryblock(s?) ... '}' {
    my $arg;
    
    for my $v (@{$item[1]}) {
	$arg .= $v->text() . "\n";
    }
    $thisparser->{'local'}{'service'}->queryArg($arg);
    $thisparser->{'local'}{'service'}->queryLine($item[1][0]->line());
    $thisparser->{'local'}{'service'}->queryFile($item[1][0]->filename());
}

################################################################################
# Detect
################################################################################

detect : ...'}'
| Id '{' <commit> DetectBody[ name=>$item{Id} ] '}' detect { $thisparser->{'local'}{'indetect'} = 0; }
| <error>

DetectBody : { $thisparser->{'local'}{'indetect'} = 1; $thisparser->{'local'}{'detect'} = (); 1; } FileLine DWho DTimerPeriod DWait DInterval DTimeout DWTrigger DITrigger DTTrigger SuppressionTransitions ...'}' 
{
    my $d = Mace::Compiler::Detect->new(name => $arg{name}, 
                                        line => $item{FileLine}->[0],
                                        filename => $item{FileLine}->[1],
                                        timerperiod => $item{DTimerPeriod}, 
                                        who => $item{DWho},
                                        'wait' => $item{DWait}, 
                                        interval => $item{DInterval}, 
                                        timeout => $item{DTimeout}, 
#                                        waittrigger => $item{DWTrigger}, 
#                                        intervaltrigger => $item{DITrigger}, 
#                                        timeouttrigger => $item{DTTrigger}, 
#                                        suppressions => $thisparser->{'local'}{'detect'}
                                        );
    if (ref($item{DWTrigger})) {
        $d->waittrigger($item{DWTrigger});
    }
    if (ref($item{DITrigger})) {
        $d->intervaltrigger($item{DITrigger});
    }
    if (ref($item{DTTrigger})) {
        $d->timeouttrigger($item{DTTrigger});
    }
    if (scalar($thisparser->{'local'}{'detect'})) {
        $d->push_suppressions(@{$thisparser->{'local'}{'detect'}});
    }
    $thisparser->{'local'}{'service'}->push_detects($d);
}
| <error>

DWho : ('nodes' | 'node') <commit> '=' Id ';' { $return = $item{Id}; } | <error>
DTimerPeriod : 'timer_period' <commit> '=' Expression ';' { $return = $item{Expression}->toString(); } | <error?> <reject> | { $return = ""; }
#DTimerPeriod : 'timer_period' <commit> '=' Expression ';' { $return = $item{Expression}; } | <error?> <reject> | { $return = ""; }
DWait : 'wait' <commit> '=' Expression ';' { $return = $item{Expression}->toString(); } | <error?> <reject> | { $return = ""; }
#DWait : 'wait' <commit> '=' Expression ';' { $return = $item{Expression} } | <error?> <reject> | { $return = ""; }
DInterval : 'interval' <commit> '=' Expression ';' { $return = $item{Expression}->toString(); } | <error?> <reject> | { $return = ""; }
#DInterval : 'interval' <commit> '=' Expression ';' { $return = $item{Expression} } | <error?> <reject> | { $return = ""; }
DTimeout : 'timeout' <commit> '=' Expression ';' { $return = $item{Expression}->toString(); } | <error?> <reject> | { $return = ""; }
#DTimeout : 'timeout' <commit> '=' Expression ';' { $return = $item{Expression} } | <error?> <reject> | { $return = ""; }
DWTrigger : ...'wait_trigger' <commit> Method[noReturn => 1, typeOptional => 1] | <error?> <reject> | 
DITrigger : ...'interval_trigger' <commit> Method[noReturn => 1, typeOptional => 1] | <error?> <reject> | 
DTTrigger : ...'timeout_trigger' <commit> Method[noReturn => 1, typeOptional => 1] | <error?> <reject> | 
SuppressionTransitions : 'suppression_transitions' <commit> '{' transitions '}' | <error?> <reject> |



################################################################################
# Properties
################################################################################

properties : PropertiesSection(s?) ...'}' | <error>
PropertiesSection : PropertiesSectionNames Block[rule=>$item{PropertiesSectionNames}] | <error>
PropertiesSectionNames : 'safety' | 'liveness' | <error>

safety : Property(s?) ...'}' {
  $thisparser->{'local'}{'service'}->push_safetyProperties(@{$item[1]});
  $return = 1;
} | <error>
liveness : Property(s?) ...'}' {
  $thisparser->{'local'}{'service'}->push_livenessProperties(@{$item[1]});
  $return = 1;
} | <error>

Property : <rulevar: $propname = "">
Property : Id ':' <commit>
{ 
  $propname = $item{Id};
#  print "Begun property $item{Id}\n";
  $thisparser->{local}{'propertyName'} = $item{Id}; 
  $thisparser->{local}{'count'} = 0; 
} 
GrandBExpression[lookahead=>';'] ';'
{
#  print "Finished property $item{Id}\n";
  $return = Mace::Compiler::Properties::Property->new(name=>$item{Id}, property=>$item{GrandBExpression});
}
| <error?:Invalid syntax in property $propname, see detailed error> <error>

GrandBExpression : <leftop: BExpression Join BExpression> ..."$arg{lookahead}"
  {
    my $lhs = shift(@{$item[1]});
    while(@{$item[1]}) {
      my $op = shift(@{$item[1]});
      my $rhs = shift(@{$item[1]});
      $lhs = Mace::Compiler::Properties::JoinExpression->new(lhs=>$lhs, rhs=>$rhs, type=>$op, methodName=>"$thisparser->{local}{'propertyName'}_JoinExpression_$thisparser->{local}{'count'}");
      $thisparser->{local}{'count'}++;
    }
    $return = $lhs;
  }
Join : '\or' {$return=0} 
  | '\and' {$return=1}
  | '\xor' {$return=2} 
  | '\implies' {$return=3}
  | '\iff' {$return=4}
  | <error>
BExpression : ...EquationLookahead <commit> Equation 
| ...BinaryOpLookahead <commit> BinaryBExpression 
| ...BBlockDelim <commit> BBlock 
| ...Quantifier <commit> Quantification 
| '\not' <commit> GrandBExpression[lookahead=>''] 
  {
    $return = Mace::Compiler::Properties::Equation->new(lhs=>Mace::Compiler::Properties::PropertyItem->new(null=>1), type=>"!", rhs=>$item{GrandBExpression}, methodName=>"$thisparser->{local}{'propertyName'}_Equation_$thisparser->{local}{'count'}");
    $thisparser->{local}{'count'}++;
  }
| SetVariable[boolean=>1, varType=>2] 
#| <error?> <error>

EquationLookahead : NonBExpression[varType=>2] EquationBinOp 
BinaryOpLookahead : SetVariable[varType=>2] (SetSetBinOp | ElementSetBinOp )
BinaryBExpression : ElementSetExpression | SetSetExpression

Equation : NonBExpression[varType=>2] Equality <commit> NonBExpression[varType=>2]
  {
    $return = Mace::Compiler::Properties::Equation->new(lhs=>$item[1], type=>$item[2], rhs=>$item[4], methodName=>"$thisparser->{local}{'propertyName'}_Equation_$thisparser->{local}{'count'}");
    $thisparser->{local}{'count'}++;
  }
| <error?> <reject> 

EquationBinOp : '==' | '>=' | '<=' | '!=' | '=' | '\neq' | '\leq' | '\geq' | '<' | '>' | <error>
SetSetBinOp : '\subset' | '\propersubset' | '\eq' | <error>
ElementSetBinOp : '\in' | '\notin' | <error>

NEquality : '==' <commit> <error: $item[1] not allowed, use = for equality>
| '>=' <commit> <error: $item[1] not allowed, use \\\geq> 
| '<=' <commit> <error: $item[1] not allowed, use \\\leq>
| '!=' <commit> <error: $item[1] not allowed, use \\\neq>
| '=' {$return=2}
| '\neq' {$return=5}
| '\leq' {$return=3}
| '\geq' {$return=1} 
| '<' {$return=4}   
| '>' {$return=0}

Equality : '==' <commit> <error: $item[1] not allowed, use = for equality>
| '>=' <commit> <error: $item[1] not allowed, use \\\geq>
| '<=' <commit> <error: $item[1] not allowed, use \\\leq>
| '!=' <commit> <error: $item[1] not allowed, use \\\neq>
| '=' {$return="=="}
| '\neq' {$return="!="}
| '\leq' {$return="<="}
| '\geq' {$return=">="} 
| '<'    
| '>'   

ElementSetExpression : SetVariable[noSet=>1, varType=>1] SetOp <commit> SetVariable[varType=>0]
{
  $return = Mace::Compiler::Properties::ElementSetExpression->new(lhs=>$item[1], type=>$item[2], rhs=>$item[4], methodName=>"$thisparser->{local}{'propertyName'}_ElementSetExpression_$thisparser->{local}{'count'}");
  $thisparser->{local}{'count'}++;
}
| <error?><error>
SetOp : '\in' {$return=1}
| '\notin' {$return=0}
| <error>
SetSetExpression : SetVariable[varType=>0] SetComparisons <commit> SetVariable[varType=>0]
{
  $return = Mace::Compiler::Properties::SetSetExpression->new(lhs=>$item[1], type=>$item[2], rhs=>$item[4], methodName=>"$thisparser->{local}{'propertyName'}_SetSetExpression_$thisparser->{local}{'count'}");
  $thisparser->{local}{'count'}++;
}
| <error?><error>
SetComparisons : '\subset' {$return=1}
| '\propersubset' {$return=2}
| '\eq' {$return=0}
| <error>

BBlock : BBlockDelim GrandBExpression[lookahead=>$item{BBlockDelim}] <commit> "$item{BBlockDelim}" { $return = $item{GrandBExpression} } | <error?> <reject>
BBlockDelim : '(' { $return = ')' } | '{' {$return = '}'}
Quantification : <commit> Quantifier Id '\in' SetVariable[varType=>0] ':' <uncommit> GrandBExpression[lookahead=>""] 
    {
      $return = $item{Quantifier};
      $return->varname($item{Id});
      $return->set($item{SetVariable});
      $return->expression($item{GrandBExpression});
    }
  | <error?> <reject>

Quantifier : '\forall' 
    {
      $return = Mace::Compiler::Properties::Quantification->new(type=>2,quantity=>'all', methodName=>"$thisparser->{local}{'propertyName'}_Quantification_$thisparser->{local}{'count'}");
      $thisparser->{local}{'count'}++;
    }
  | '\exists' 
    {
      $return = Mace::Compiler::Properties::Quantification->new(type=>1,quantity=>'exists', methodName=>"$thisparser->{local}{'propertyName'}_Quantification_$thisparser->{local}{'count'}");
      $thisparser->{local}{'count'}++;
    }
  | '\for' <commit> '{' NEquality '}' '{' Number '}' 
    {
      $return = Mace::Compiler::Properties::Quantification->new(type=>$item{NEquality},quantity=>$item{Number}, methodName=>"$thisparser->{local}{'propertyName'}_Quantification_$thisparser->{local}{'count'}");
      $thisparser->{local}{'count'}++;
    }
  | <error>

NonBExpressionOp : '+' | '-' | <error>

NonBExpression : <leftop: ParenNonBExpression[%arg] NonBExpressionOp ParenNonBExpression[%arg]> 
  {
    my $lhs = shift(@{$item[1]});
    while(@{$item[1]}) {
      my $op = shift(@{$item[1]});
      my $rhs = shift(@{$item[1]});
      $lhs = Mace::Compiler::Properties::NonBExpression->new(lhs=>$lhs, rhs=>$rhs, op=>$op);
      $thisparser->{local}{'count'}++;
    }
    $return = $lhs;
  }
| <error>

ParenNonBExpression : '(' <commit> NonBExpression[%arg] ')' { $return = $item{NonBExpression}; } | SetVariable[%arg] | <error>

SetVariable : '\nodes' <commit> <reject:$arg{varType}==1 or $arg{boolean}>
    {
      $return = Mace::Compiler::Properties::SetVariable->new(variable=>'\nodes', varType=>0);
    }
  | <error?> <reject>
#  | <reject:$arg{varType}==1 or $arg{boolean}> Var '*'
#    {
#      $return = Mace::Compiler::Properties::SetVariable->new(variable=>$item{Var}, varType=>0, closure=>1, methodName=>"$thisparser->{local}{'propertyName'}_Closure_$thisparser->{local}{'count'}");
#      $thisparser->{local}{'count'}++;
#    } 
  | Var[%arg] 
    {
      $return = $item{Var};
      #Mace::Compiler::Properties::SetVariable->new(variable=>$item{Var}, varType=>$arg{varType});
    } 
  | <reject:$arg{varType}==0 or $arg{boolean}> '|' <commit> SetVariable[varType=>0] '|'
    { #note varType doesn't totally capture it -- we in fact know it is neither a set nor a set element.
#      $return = Mace::Compiler::Properties::SetVariable->new(variable=>$item{Var}, varType=>2, cardinality=>1);
      $return = $item{SetVariable};
      $return->cardinality(1); #varType=>2
    } 
  | <error?> <reject>
  | '\null' <commit> <reject:$arg{varType}==0 or $arg{boolean}>
    {
      $return = Mace::Compiler::Properties::SetVariable->new(variable=>'\null', varType=>1);
    } 
  | <error>

MethodParams : '(' Var[varType=>2](s? /,/) ')' { for my $v (@{$item[2]}) { $v->inMethodParameter(1); }; $return = $item[2]; } | { $return = "NOT_METHOD"; }
SepTok : '.' | '->' | <error>
SubVar : SepTok '(' <commit> Var[varType=>1] ')' '*' 
  {
    $return = $item{Var};
    $return->closure(1);
    $return->prefix($item{SepTok});
    $return->methodName("$thisparser->{local}{'propertyName'}_Closure_$thisparser->{local}{'count'}");
    $thisparser->{local}{'count'}++;
  }
| SepTok Var[varType=>2] 
  {
    $return = $item{Var};
    $return->prefix($item{SepTok});
  }
| { $return = "NOT_SUBVAR" }
Var : FileLine Column ScopedId MethodParams SubVar
{
  my $r = Mace::Compiler::Properties::SetVariable->new(variable=>$item{ScopedId}->toString(), varType=>$arg{varType});
  if($item{SubVar} ne "NOT_SUBVAR") {
    $r->subvar($item{SubVar});
  }
  if($item{MethodParams} ne "NOT_METHOD") {
    $r->isMethodCall(1);
    $r->push_params(@{$item{MethodParams}});
  }
#  print "Line: $item{Line} Column: $item{Column} SetVariable: $item{ScopedId} varType: $arg{varType}\n";
  $return = $r;
}
| Number <reject:$arg{boolean}>
    {
      $return = Mace::Compiler::Properties::SetVariable->new(variable=>$item{Number}, varType=>$arg{varType});
    } 
| <error>

]; # MACE grammar

1;
