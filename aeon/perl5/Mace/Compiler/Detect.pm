# 
# Detect.pm : part of the Mace toolkit for building distributed systems
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
package Mace::Compiler::Detect;

use strict;
use Class::MakeMethods::Utility::Ref qw( ref_clone );

use Mace::Compiler::Method;
use Mace::Compiler::Param;
use Mace::Compiler::Type;
use Mace::Util qw(:all);

use Class::MakeMethods::Template::Hash 
    (
     'new' => 'new',
     'scalar' => "line",
     'string' => "filename",
     'string' => "name",
     'scalar' => "timerperiod",
     'scalar' => "who",
     'scalar' => "wait",
     'scalar' => "interval",
     'scalar' => "timeout",
     'object' => ["waittrigger" => { class => "Mace::Compiler::Method"}],
     'object' => ["intervaltrigger" => { class => "Mace::Compiler::Method"}],
     'object' => ["timeouttrigger" => { class => "Mace::Compiler::Method"}],
     'array_of_objects' => ["suppressions" => { class => "Mace::Compiler::Transition" }],

     'object' => ["statevar" => {class => "Mace::Compiler::Param"}],
     );

sub validate {
    my $this = shift;
    my $sv = shift;
    my $name = $this->name();

    my @svars = grep($_->name() eq $this->who(), $sv->state_variables());
    if (scalar(@svars != 1)) {
        Mace::Compiler::Globals::error("bad_detect", $this->filename(), $this->line(), "node(s) ".$this->who()." matched ".scalar(@svars)." state variables.  Must match exactly 1");
    }

    $this->statevar($svars[0]);

    if ($this->statevar()->type()->type() eq "MaceKey") {
        $sv->push_state_variables(Mace::Compiler::Param->new(name => "__${name}_peer", type => $this->statevar()->type()));
        my $u64t = Mace::Compiler::Type->new(type => "uint64_t");
        $sv->push_state_variables(Mace::Compiler::Param->new(name => "__${name}_time", type => $u64t));
        $sv->push_state_variables(Mace::Compiler::Param->new(name => "__${name}_probe", type => $u64t));
    }

    $this->timerperiod("1*1000*1000") unless ($this->timerperiod());
    $this->wait("0") unless ($this->wait());
    $this->interval($this->timerperiod()) unless ($this->interval());
    
    my $timer = Mace::Compiler::Timer->new(name => "__detect_$name");
    my $recuropt = Mace::Compiler::TypeOption->new(name => "recur");
    $recuropt->options($this->timerperiod() => $this->timerperiod());
    $timer->push_typeOptions($recuropt);
    $sv->push_timers($timer);

    my $voidtype = Mace::Compiler::Type->new(type => "void");
    my $macekeytype = Mace::Compiler::Param->new(name => "noname1", type => Mace::Compiler::Type->new(type => "MaceKey", isConst => 1, isRef => 1));

    my $m = Mace::Compiler::Method->new(name => "wait_trigger", returnType => $voidtype, isVirtual => 0, isStatic => 0, isConst => 0);
    $m->push_params($macekeytype);
    fixDetectMethod($m, $this->waittrigger());
    $this->waittrigger()->name("_${name}_wait_trigger");

    $m->name("interval_trigger");
    fixDetectMethod($m, $this->intervaltrigger());
    $this->intervaltrigger()->name("_${name}_interval_trigger");

    $m->name("timeout_trigger");
    fixDetectMethod($m, $this->timeouttrigger());
    $this->timeouttrigger()->name("_${name}_timeout_trigger");

    $sv->push_transitions($this->getTimerTransition());
    $sv->push_transitions($this->getMaceInitTransition());
    $sv->push_routines($this->getResetRoutine());
    $sv->push_routines($this->waittrigger());
    $sv->push_routines($this->intervaltrigger());
    $sv->push_routines($this->timeouttrigger());

    for my $t ($this->suppressions()) {
        $t->method()->options("merge", "pre");
        $sv->push_transitions($t);
    }
}

sub getTimerTransition {
    my $this = shift;
    my $name = $this->name();
    my $varname = $this->who();
    my $svtype = $this->statevar->type()->type();

    my $timeoutcall = "";
    my $timeoutcheck = "";
    if (defined($this->timeouttrigger())) {
        $timeoutcall = "_${name}_timeout_trigger(__d_peer);"
    }
    if ($this->timeout()) {
        $timeoutcheck = " 
            if (__d_time + ${\$this->timeout()} < curtime) {
                __d_time = curtime;
                $timeoutcall
            } else 
        ";
    }
    my $wait = $this->wait();
    my $interval = $this->interval();
    my $waitcall = "";
    if (defined($this->waittrigger())) {
        $waitcall = "if (__d_probe < __d_time) { _${name}_wait_trigger(__d_peer); }"
    }
    my $intervalif = "__d_probe = curtime";
    if (defined($this->intervaltrigger())) {
        $intervalif = " if (__d_probe < __d_time || __d_probe + $interval < curtime) {
                            _${name}_interval_trigger(__d_peer);
                            __d_probe = curtime;
                        } ";
    }

    my $loopbeg = "
        ${svtype}::map_iterator mi = ${varname}.mapIterator();
        while(mi.hasNext()) {
            MaceKey __d_peer;
            ${svtype}::mapped_type __d_obj = mi.next(__d_peer);
            uint64_t& __d_probe = __d_obj.__d_probe;
            uint64_t& __d_time  = __d_obj.__d_time;
        ";
    my $loopend = "}";
    if ($svtype eq "MaceKey") {
        $loopbeg = "
            MaceKey& __d_peer = __${name}_peer;
            uint64_t& __d_probe = __${name}_probe;
            uint64_t& __d_time = __${name}_time;
            if (${varname}.isNullAddress()) {
                __d_peer = ${varname};
                return;
            }
            if (${varname} != __d_peer) {
                __d_time = curtime;
                __d_peer = ${varname};
                return;
            }
        ";
        $loopend = "
        ";
    }

    my $methodbody = " {
        $loopbeg
            $timeoutcheck
            if (__d_time + $wait < curtime) {
                $waitcall
                $intervalif
            }
        $loopend
    }
    ";

    my $m = Mace::Compiler::Method->new(name => "__detect_$name", returnType => Mace::Compiler::Type->new(), isConst => 0, isStatic => 0, body => $methodbody);
    my $t = Mace::Compiler::Transition->new(name => "__detect_$name", type => "scheduler", method => $m);
    $t->push_guards(Mace::Compiler::Guard->new(is_object => 0, guardStr => "(true)", line => -1, file => "nofile"));
    return $t;
}

sub getMaceInitTransition {
    my $this = shift;
    my $name = $this->name();

    my $methodbody = " {
        __detect_$name.schedule(${\$this->timerperiod()});
    }
    ";

    my $m = Mace::Compiler::Method->new(name => "maceInit", returnType => Mace::Compiler::Type->new(), isConst => 0, isStatic => 0, body => $methodbody);
    $m->options("merge", "post");
    my $t = Mace::Compiler::Transition->new(name => "maceInit", type => "downcall", method => $m);
    $t->push_guards(Mace::Compiler::Guard->new(is_object => 0, guardStr => "(true)", line => -1, file => "nofile"));
    return $t;
}

sub getResetRoutine {
    my $this = shift;
    my $name = $this->name();
    my $varname = $this->who();

    my $methodbody = "
        if (${varname}.containsKey(__peer)) {
            ${varname}.get(__peer).__d_time = curtime;
        }
    ";
    if ($this->statevar()->type()->type() eq "MaceKey") {
        $methodbody = "
            if (${varname} == __peer) {
                __${name}_peer = __peer;
                __${name}_time = curtime;
            }
        "
    }
    my $m = Mace::Compiler::Method->new(name => "reset_$name", returnType => Mace::Compiler::Type->new(type => "void"), isConst => 0, isStatic => 0, body => $methodbody);
    $m->push_params(Mace::Compiler::Param->new(name => "__peer", type => Mace::Compiler::Type->new(type => "MaceKey", isConst => 1, isRef => 1)));
    return $m;
}

sub fixDetectMethod {
    my $matchMethod = shift;
    my $parsedMethod = shift;

    my @methods;
    push(@methods, $matchMethod);

    my $origmethod;

    if (defined $parsedMethod) {
        if(ref ($origmethod = Mace::Compiler::Method::containsTransition($parsedMethod, @methods))) {
            $parsedMethod->returnType($origmethod->returnType());
            my $i = 0;
            for my $p ($parsedMethod->params()) {
                unless($p->type) {
                    $p->type($origmethod->params()->[$i]->type());
                }
                $i++;
            }
        } 
        else {
            Mace::Compiler::Globals::error("bad_detect", $parsedMethod->filename(), $parsedMethod->line(), $origmethod);
        }
    }
}

1;
