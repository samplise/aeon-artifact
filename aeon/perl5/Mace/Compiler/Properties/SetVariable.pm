# 
# SetVariable.pm : part of the Mace toolkit for building distributed systems
# 
# Copyright (c) 2011, Charles Killian
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
package Mace::Compiler::Properties::SetVariable;

use strict;
use Class::MakeMethods::Utility::Ref qw( ref_clone );
use base qw{Mace::Compiler::Properties::PropertyItem};

use Class::MakeMethods::Template::Hash
    (
     'new' => 'new',
     'scalar' => "variable",

     'scalar' => "actualVariable",
     'scalar' => "prefix", #on sub variables only!
     'object' => ["allnodes" => {class => "Mace::Compiler::Param"}],
     'object' => ["keymap" => {class => "Mace::Compiler::Param"}],

     'object' => ["guessedType" => { class => "Mace::Compiler::Type"}],

     'object' => [subvar => { class => "Mace::Compiler::Properties::SetVariable"}],

     'boolean' => 'inMethodParameter',
     'boolean' => 'isMethodCall',
     'array_of_objects' => [params => { class => "Mace::Compiler::Properties::SetVariable"}],

     'boolean' => "closure",
     'boolean' => "cardinality",
     'number' => "varType", # 0 - set, 1 - set element, 2 - indeterminant/other
     boolean => 'castPrior', #if true, cast the prior stem as a node

     'object' => [closureMethod => { class => "Mace::Compiler::Method" }],
     'scalar' => "methodName",
    );

sub overallClosure {
  my $this = shift;
  if($this->closure and not $this->cardinality) {
    return 1;
  }
  elsif(defined($this->subvar)) {
    return $this->subvar->overallClosure();
  }
  return 0;
}

sub actualPrefix {
  my $this = shift;
  if($this->prefix eq "." and $this->castPrior) {
    if( $this->isMethodCall ){
        return "->second->";
    }elsif( $this->actualVariable eq "state" ){
        return "->second->";
    }else{
        return "->second->globalContext->";
    }
  }
  return $this->prefix;
}

sub toString {
    my $this = shift;
    my %args = @_;

    my $rhs = $this->variable();
    if($this->isMethodCall) {
      $rhs .= '('.join(',', map{$_->toString(@_)} $this->params).')';
    }
    if(defined($this->subvar)) {
      $rhs .= $this->subvar->toString(@_);
    }
    if($this->closure()) {
      $rhs = "($rhs)*";
    }
    if($this->cardinality()) {
      $rhs = "|$rhs|";
    }
    $rhs = $this->prefix.$rhs;

    return $rhs;
} # toString

sub toMethod {
  my $this = shift;
  my %args = @_;
  my $s = "";

  if($this->closure()) {
    $s .= $this->closureMethod->toString(body=>1, %args);
  }
  if(defined($this->subvar)) {
    $s .= $this->subvar->toMethod(%args);
  }
  for my $p ($this->params()) {
    $s .= $p->toMethod(%args);
  }
  return $s;
}

sub toMethodCall {
    my $this = shift;
    my $lhs = "";
    if($this->prefix ne "") {
      $lhs = shift;
    }
    my %args = @_;

    if($this->castPrior and not $args{specialClosure}) {
      $lhs = "castNode($lhs, ${\$this->allnodes->name}, ${\$this->keymap->name})";
    }

    if($this->closure() and not $args{specialClosure}) {
      $lhs = $this->closureMethod->name."(".join(",", (map{$_->toString(notype=>1)} grep($_->name ne '__init', $this->closureMethod->params())), $lhs).")";
    }
    else {
      $lhs .= $this->actualPrefix.$this->actualVariable;
      if($this->isMethodCall) {
        $lhs .= '('.join(',', map{$_->toMethodCall(@_)} $this->params()).')';
      }
    }

    my $ret = $lhs;
    if(defined($this->subvar) and (not $this->closure() or $args{specialClosure})) {
      $ret = $this->subvar->toMethodCall($lhs, @_);
    }
    if($this->cardinality()) {
      $ret .= ".size()";
    }

#    print "toMethodCall: ".$this->variable." allnodes ".$this->allnodes->toString()."\n";
    if($this->inMethodParameter()) {
      if(defined($this->overallType())) {
        my $IterType = ref_clone($this->allnodes->type());
        $IterType->type($IterType->type."::const_iterator");
#        print "toMethodCall: ".$this->variable." overallType ".$this->overallType()->toString()." itertype ".$IterType->toString()."\n";
        if($this->overallType()->eq($IterType)) {
#          print "Success\n";
          $ret = "castNode($ret, ${\$this->allnodes->name}, ${\$this->keymap->name})->second->localAddress()";
        } else {
#          print "Failure\n";
        }
      }
    }
    elsif($this->prefix eq "" and $this->varType == 1) {
      $ret = "castNode($ret, ${\$this->allnodes->name}, ${\$this->keymap->name})";
    }

    return $ret;
}

sub subVal{
  my $this = shift;
  my $sv = shift;
  my @p = @_;

#  print "Validating variable piece ".$this->variable."\n";

  $this->allnodes($p[0]);
  $this->keymap($p[1]);
  $this->actualVariable($this->variable());
  foreach my $p ($this->params) {
    $p->validate($sv, @p);
  }
  if(defined($this->subvar())) {
    $this->subvar->validateSubVar($sv, @p);
  }
}

sub validate {
    my $this = shift;
    my $sv = shift;
    my @p = @_;

    die("Cannot have prefix '${\$this->prefix}' on variable ".$this->variable) if(defined($this->prefix) and $this->prefix ne "");
    $this->prefix("");
    
    my $IterType = Mace::Compiler::Type->new(type=>$p[0]->type->type."::const_iterator");
    if($this->variable eq '\nodes') {
      $this->actualVariable($p[0]->name);
      $this->guessedType($p[0]->type);
      $this->allnodes($p[0]);
      $this->keymap($p[1]);
      die if($this->varType() != 0);
      die if(defined($this->subvar));
    } elsif($this->variable eq '\null') {
      $this->actualVariable($p[0]->name.".end()");
      $this->guessedType($IterType);
      $this->allnodes($p[0]);
      $this->keymap($p[1]);
      die("Variable was \\null but varType is ${\$this->varType}") if($this->varType() != 1);
      die if(defined($this->subvar));
    } 
    else {
      if(my @vp = grep($this->variable eq $_->name, @p)) {
        my $vp = shift @vp;
        $this->guessedType($vp->type);
        if($vp->flags('varType') == 1 and not defined($this->subvar)) {
          if($this->varType() == 0) {
            die;
          }
          $this->varType(1);
        }
      }
      
      #How does one validate a variable?
      #What about guessed type?
      $this->subVal($sv, @p);
    }
}

sub overallType {
  my $this = shift;
  if($this->cardinality()) {
    return Mace::Compiler::Type->new(type=>'size_t');
  }
  if($this->closure()) {
    return $this->closureMethod->returnType;
  }
  if(defined($this->subvar)) {
#    print "overallType called on variable ".$this->variable.", calling subvar overallType\n";
    return $this->subvar->overallType();
  }
#  print "overallType called on variable ".$this->variable.", returning ".(defined($this->guessedType)?$this->guessedType->type:"undef")."\n";
  return $this->guessedType();
}

sub validateSubVar {
    my $this = shift;
    my $sv = shift;
    my @params = @_;

    $this->subVal($sv, @params);
    if($this->closure) {
      $this->castPrior(1);
      $this->setMethod(@params);
    }

    die("Need prefix separator on sub variable") unless(defined($this->prefix));

    if($this->prefix() eq ".") {
      my $StateVariable = Mace::Compiler::Param->new(name=>'state',type=>Mace::Compiler::Type->new(type=>'State'));
      #my @p = grep($this->variable eq $_->name, $sv->state_variables(), $StateVariable, $sv->timers());
      my @p = grep($this->variable eq $_->name, ${ $sv->contexts() }[0]->ContextVariables() , $StateVariable, $sv->timers());
      my @m = grep($this->variable eq $_->name, $sv->routines());
      push(@m, grep($this->variable eq "downcall_".$_->name, grep($_->isConst(), $sv->usesClassMethods())));
      push(@m, grep($this->variable eq "upcall_".$_->name, grep($_->isConst(), $sv->providedHandlerMethods())));
      push(@m, grep($this->variable eq $_->name, grep($_->isConst(), $sv->providedMethods())));
      #map { push(@p, grep($this->variable eq $_->name, $_->fields())) } $sv->auto_types();
      if(@p) {
        my $p = shift @p;
        $this->castPrior(1);
        $this->guessedType($p->type);
      } elsif(@m) {
        my $m = shift @m;
        $this->castPrior(1);
        $this->guessedType($m->returnType);
      } else {
          push(@m, grep($this->variable eq "downcall_".$_->name, $sv->usesClassMethods()));
          push(@m, grep($this->variable eq "upcall_".$_->name, $sv->providedHandlerMethods()));
          push(@m, grep($this->variable eq $_->name, $sv->providedMethods()));
          if (@m) {
              Mace::Compiler::Globals::warning("suspicious_property", -1, "It looks like you may have incorrectly tried to use a non-const method '".$this->variable."' within a property.  g++ will fail to compile if so, complaining that an iterator won't have the given method (if g++ doesn't complain, you may safely disregard this message).");
          }
        $this->castPrior(0);
      }
    } else {
      #For now, if it's not . as a separator, we need not cast as a node.
      $this->castPrior(0);
      #We also don't know what type to guess
    }
}

sub setMethod {
  my $this = shift;
  my @p = @_;

  my $setType = $p[0]->type->type;
  my $setIterType = "${setType}::const_iterator";
  my $returnType = Mace::Compiler::Type->new(type=>"mace::map<int, ${\$p[0]->type->type}::const_iterator, mace::SoftState>");
  my $closureSetType = $returnType->type;
  my $allnodes = $p[0]->name;
  my $keymapn = $p[1]->name;
  my $pump = $this->toMethodCall("iter", specialClosure=>1);

  # only used for closures
  my $s =  qq/{
                $closureSetType closure;
                $setIterType iter = __init;
                while ((iter != $allnodes.end()) && closure.find(iter->first) == closure.end()) {
                  closure[iter->first] = iter;
                  iter = castNode($pump, $allnodes, $keymapn);
                }
                return closure;
              }
            /;
     
  my $m = Mace::Compiler::Method->new(name=>$this->methodName, isStatic=>1, returnType=>$returnType, body=>$s);
  $m->push_params(@p);
  $m->push_params(Mace::Compiler::Param->new(name=>'__init', type=>Mace::Compiler::Type->new(type=>$setIterType)));
  $this->closureMethod($m);
}

sub toMethodDeclare {
  my $this = shift;
  my %args = @_;

  my $s = "";

  if($this->closure()) {
    $s .= $this->closureMethod->toString(%args).";\n";
  }
  if(defined($this->subvar)) {
    $s .= $this->subvar->toMethodDeclare(%args);
  }
  for my $p ($this->params()) {
    $s .= $p->toMethodDeclare(%args);
  }
  return $s;
}

1;

