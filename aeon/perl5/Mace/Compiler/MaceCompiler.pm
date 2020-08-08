# 
# MaceCompiler.pm : part of the Mace toolkit for building distributed systems
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
package Mace::Compiler::MaceCompiler;

use strict;

use Mace::Compiler::Context;
use Mace::Compiler::Type;
use Mace::Compiler::TypeOption;
use Mace::Compiler::Param;
use Mace::Compiler::Method;
use Mace::Compiler::ServiceImpl;
use Mace::Compiler::ServiceVar;
use Mace::Compiler::AutoType;
use Mace::Compiler::RoutineObject;
use Mace::Compiler::Transition;
use Mace::Compiler::TypeDef;
use Mace::Compiler::GeneratedOn;
use Mace::Compiler::Grammar;
use Mace::Compiler::MaceRecDescent;
use Mace::Compiler::Guard;
use Mace::Compiler::Timer;
use Mace::Compiler::Detect;
use Mace::Compiler::Query;
use Mace::Compiler::Properties::SetVariable;
use Mace::Compiler::Properties::Quantification;
use Mace::Compiler::Properties::SetSetExpression;
use Mace::Compiler::Properties::ElementSetExpression;
use Mace::Compiler::Properties::Equation;
use Mace::Compiler::Properties::JoinExpression;
use Mace::Compiler::Properties::Property;
use Mace::Compiler::Properties::NonBExpression;

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

use Mace::Util qw(:all);

use Parse::RecDescent;

$::RD_ERRORS = 1; # Make sure the parser dies when it encounters an error
$::RD_WARN   = 1; # Enable warnings. This will warn on unused rules &c.
$::RD_HINT   = 1; # Give out hints to help fix problems.
# $::RD_TRACE  = 1;


use Class::MakeMethods::Template::Hash
    (
    'new --and_then_init' => 'new',
    'object' => ['parser' => { class => "Parse::RecDescent" }],
    'hash --get_set_items' => 'using',
#     'object' => ['class' => { class => "Mace::Compiler::ServiceImpl" }],
    );

sub init {
    my $this = shift;
    my $p = Mace::Compiler::MaceRecDescent->new();
    $this->parser($p);
} # init

sub parse {
    my $this = shift;
    my $in = shift;
    my $file = shift;

    #print "parse\n";

    $this->processUsing($in);

    my @linemap;
    my @filemap;
    my @offsetmap;

    Mace::Compiler::MInclude::getLines($file, $in, \@linemap, \@filemap, \@offsetmap);

    $this->parser()->{local}{linemap} = \@linemap;
    $this->parser()->{local}{filemap} = \@filemap;
    $this->parser()->{local}{offsetmap} = \@offsetmap;

    my $t = join("", @$in);

    $Mace::Compiler::Grammar::text = $t;
    # open(OUT, ">", "/tmp/foo");
    # print OUT $t;
    # close(OUT);
    my $svName = $file;
    $svName =~ s|^.*/||;
    $svName =~ s|[.].*||;
    $this->parser()->{local}{servicename} = $svName;
    my $sc = $this->parser()->PrintError($t, 1, $file) || die "syntax error\n";
    my @defers = ();
    $Mace::Compiler::Grammar::text = $t;
    $sc->parser($this->parser());
    $sc->origMacFile($t);
    push(@defers, $this->findDefers($t));
    $this->enableFailureRecovery($sc);

    $sc->validate(@defers);
    return $sc;
} # parse

sub enableFailureRecovery {
    my $this = shift;
    my $sc = shift;


    return if ( (not $Mace::Compiler::Globals::supportFailureRecovery)  or not($sc->count_contexts()>0) );


    return; # chuangw: This is obsoleted
=begin
    # add 'msgseqno' into state variable
    my $type = Mace::Compiler::Type->new( type => "mace::map< mace::string, uint32_t>", isConst1 => 0, isConst2 => 0, isConst => 0, isRef => 0);
    my $p = Mace::Compiler::Param->new(name => "__internal_msgseqno", type => $type, hasDefault => 0, filename => __FILE__, line => __LINE__, default => 0 );
    $sc->push_state_variables($p);
    # add 'lastAckedSeqno' into state variable
    my $lastAckedType = Mace::Compiler::Type->new( type => "mace::map<mace::string, uint32_t >", isConst1 => 0, isConst2 => 0, isConst => 0, isRef => 0);
    my $lastAckedSeqno = Mace::Compiler::Param->new(name => "__internal_lastAckedSeqno", type => $lastAckedType, hasDefault => 0, filename => __FILE__, line => __LINE__, default => 0 );
    $sc->push_state_variables($lastAckedSeqno);

    # add 'receivedSeqno' into state variable
    # chuangw: I wanted to use priority queue, but there's no such implementation in Mace that support serialization. Use mace::map instead.
    my $receivedSeqnoType = Mace::Compiler::Type->new( type => "mace::map<mace::string, mace::map<uint32_t,uint8_t>  >", isConst1 => 0, isConst2 => 0, isConst => 0, isRef => 0);
    my $receivedSeqno = Mace::Compiler::Param->new(name => "__internal_receivedSeqno", type => $receivedSeqnoType, hasDefault => 0, filename => __FILE__, line => __LINE__, default => 0 );
    $sc->push_state_variables($receivedSeqno);

    # add 'unAck' into state variable
    my $unAckType = Mace::Compiler::Type->new( type => "mace::map<mace::string, mace::map<uint32_t, mace::string> >", isConst1 => 0, isConst2 => 0, isConst => 0, isRef => 0);
    my $unAckParam = Mace::Compiler::Param->new(name => "__internal_unAck", type => $unAckType, filename => __FILE__, line => __LINE__);
    $sc->push_state_variables($unAckParam);

    # used by virtual head node.
    my $contextPrevEventType = Mace::Compiler::Type->new( type => "mace::map<mace::string, uint32_t>", isConst1 => 0, isConst2 => 0, isConst => 0, isRef => 0);
    my $contextPrevEvent = Mace::Compiler::Param->new(name => "__internal_contextPrevEvent", type => $contextPrevEventType, filename => __FILE__, line => __LINE__,);
    $sc->push_state_variables($contextPrevEvent);


    # add 'Ack' message type which has two parameter: seqno and ackno
    my $seqnoType = Mace::Compiler::Type->new(type=>"uint32_t",isConst=>0,isConst1=>0,isConst2=>0,isRef=>0);
    my $acknoField = Mace::Compiler::Param->new(name=>"ackno", type=>$seqnoType);

    my $ackContext = Mace::Compiler::Type->new(type=>"mace::string",isConst=>0,isConst1=>0,isConst2=>0,isRef=>0);
    my $ackctxField = Mace::Compiler::Param->new(name=>"ackctx", type=>$ackContext);
    # chuangw: XXX: Does acknowledge packet needs a sequence number??
    my $ackMessageType = Mace::Compiler::AutoType->new(name=>"__internal_Ack",filename => __FILE__, line => __LINE__, method_type=>1, method_type=>Mace::Compiler::AutoType::FLAG_CONTEXT);
    $ackMessageType->push_fields($acknoField);
    $ackMessageType->push_fields($ackctxField);
    $sc->push_messages( $ackMessageType );
    # TODO: only add Ack message handler if not previously defined
    my $ackDeliverHandlerBody = qq#
    {
        // TODO: need to lock on internal lock?
        ScopedLock sl(mace::ContextBaseClass::__internal_ContextMutex );
        if( __internal_unAck.find( msg.ackctx ) == __internal_unAck.end() ){
            macedbg(1)<<"Ack came from "<<src<<",context="<< msg.ackctx <<" whom I haven't received Ack before. Did something just recovered from failure?"<<Log::endl;
        }else{
            macedbg(1)<<"Ack "<< msg.ackno <<" received"<<Log::endl;
            // remove buffers that sequence number is <= msg.ackno
            mace::map<uint32_t, mace::string>::iterator bufferIt = __internal_unAck[msg.ackctx].begin();
            while( bufferIt != __internal_unAck[msg.ackctx].end() && bufferIt->first <= msg.ackno ){
                __internal_unAck[msg.ackctx].erase( bufferIt );
                macedbg(1)<<"Removing seqno "<< bufferIt->first <<" from __internal_unAck"<<Log::endl;
                bufferIt = __internal_unAck[msg.ackctx].begin();
            }

        }

    }
    #;

  my $ackHandlerReturnType = Mace::Compiler::Type->new();
  my $macekeyType = Mace::Compiler::Type->new(isConst=>1,isConst1=>1,isConst2=>0,type=>'MaceKey',isRef=>1);
  my $ackType = Mace::Compiler::Type->new(isConst=>1,isConst1=>1,isConst2=>0,type=>'__internal_Ack',isRef=>1);

  my $ackDeliverParam1 = Mace::Compiler::Param->new(filename=>__FILE__,hasDefault=>0,name=>'src',type=>$macekeyType,line=>__LINE__);
  my $ackDeliverParam2 = Mace::Compiler::Param->new(filename=>__FILE__,hasDefault=>0,name=>'dest',type=>$macekeyType,line=>__LINE__);
  my $ackDeliverParam3 = Mace::Compiler::Param->new(filename=>__FILE__,hasDefault=>0,name=>'msg',type=>$ackType,line=>__LINE__);
  my $ackDeliverHandlerGuard = Mace::Compiler::Guard->new( 
      file => __FILE__,
      guardStr => 'true',
      type => 'state_var',
      state_expr => Mace::Compiler::ParseTreeObject::StateExpression->new(type=>'null'),
      line => __LINE__

  );
  my $ackDeliverHandler = Mace::Compiler::Method->new(
      body => $ackDeliverHandlerBody, 
      throw => undef,
      filename => __FILE__,
      isConst => 0, 
      isUsedVariablesParsed => 0,
      isStatic => 0, 
      name => "deliver",
      returnType => $ackHandlerReturnType,
      line => __LINE__, 
      );
  $ackDeliverHandler->push_params( $ackDeliverParam1 );
  $ackDeliverHandler->push_params( $ackDeliverParam2 );
  $ackDeliverHandler->push_params( $ackDeliverParam3 );
  $ackDeliverHandler->targetContextObject("__internal"  );

  my $t = Mace::Compiler::Transition->new(name => $ackDeliverHandler->name(), 
  startFilePos => -1, 
  columnStart => -1,  
  type => "upcall", 
  method => $ackDeliverHandler,
    startFilePos => -1,
    columnStart => '-1',

  transitionNum => 100
  );
  $t->push_guards( $ackDeliverHandlerGuard );
  $sc->push_transitions( $t);
  my $timer = Mace::Compiler::Timer->new( name=> 'resender_timer', expireMethod=>'expire_resender_timer'
  );
  my %timerOptionHash = ( "500*1000" => "500*1000" );
  my $timerOption = Mace::Compiler::TypeOption->new( 
   name => 'recur',
   file => __FILE__,
   line => __LINE__,
  );
  $timerOption->options( %timerOptionHash );
  $timer->push_typeOptions( $timerOption );
  $sc->push_timers( $timer );
 
 # add timer event handler
  my $resenderTimerHandlerReturnType = Mace::Compiler::Type->new();

  my $resenderTimerHandlerBody = qq/
  {
      std::cout<<"resender timer"<<std::endl;
      return;
  }/;
  my $resenderTimerHandler = Mace::Compiler::Method->new(
      body => $resenderTimerHandlerBody, 
      throw => undef,
      filename => __FILE__,
      isConst => 0, 
      isUsedVariablesParsed => 0,
      isStatic => 0, 
      name => "resender_timer",
      returnType => $resenderTimerHandlerReturnType,
      line => __LINE__,
      targetContextObject => "__internal" , 
      );
  my $resenderTimerHandlerGuard = Mace::Compiler::Guard->new( 
      file => __FILE__,
      guardStr => 'true',
      type => 'state_var',
      state_expr => Mace::Compiler::ParseTreeObject::StateExpression->new(type=>'null'),
      line => __LINE__

  );
  my $resenderSchedulerTransition = Mace::Compiler::Transition->new(
      name => 'resender_timer',
      startFilePos => -1,
      columnStart => -1,
      type => "scheduler", 
      method => $resenderTimerHandler,
      startFilePos => -1,
      columnStart => '-1',
      transitionNum => 101
  );
  $resenderSchedulerTransition->push_guards( $resenderTimerHandlerGuard );
  $sc->push_transitions( $resenderSchedulerTransition);
=cut
}

sub processUsing {
  my $this = shift;
  my $t = shift;

  my $line = 1;
  for (@$t) {
    if ($_ =~ m|^\s*using\s+namespace|) {
        Mace::Compiler::Globals::error("bad_using", " ", $line, "'using namespace' declarations are not supported");
    }
    
    if ($_ =~ m|^\s*using\s+(.*)::(\w+);\s*|) {
	$this->using($2,$1);
    }
    $line++;
  }

  #my @l = split(/\n/, $$t);
  for my $l (@$t) {
      my %using = %{$this->using()};
      while (my ($k, $v) = each(%using)) {
	  if ($l =~ m|^\s*#|) {
	      next;
	  }
	  $l =~ s|^(\s*using\s+${v}::${k};)|/* $1 */|m;
	  $l =~ s|([^:])\b($k)\b|${1}${v}::${k}|g;
      }
  }
  #$$t = join("\n", @l) . "\n";
}

sub findDefers {
    my $this = shift;
    my $t = shift;

    my @r = ();

    open(IN, "<", \$t) or die;
    while(<IN>) {
        if ($_ =~ m|\sdefer_(.*?)\(|) {
            push(@r, $1);
        }
    }
    close(IN);
    return @r;
}

1;
