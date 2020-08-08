# 
# SetSetExpression.pm : part of the Mace toolkit for building distributed systems
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
package Mace::Compiler::Properties::SetSetExpression;

use strict;
use base qw{Mace::Compiler::Properties::BinaryOpPropertyItem};

use Class::MakeMethods::Template::Hash
    (
     'new' => 'new',
     'number' => "type", # 0 - eq, 1 - subset, 2 - proper subset
     'scalar' => 'methodName',
    );

sub toString {
    my $this = shift;

    my $setop;
    if($this->type == 0) { $setop = "is equivalent to"; }
    elsif($this->type == 1) { $setop = "is a subset of"; }
    elsif($this->type == 2) { $setop = "is a proper subset of"; }
    return $this->SUPER::toString($setop, @_);
}

sub validate {
  my $this = shift;
  my $sv = shift;
  my @p = @_;

  $this->SUPER::validate($sv, @p);

  my $m = Mace::Compiler::Method->new(name=>$this->methodName, isStatic=>1, returnType=>Mace::Compiler::Type->new(type=>"bool"), body=>$this->getBody());
  $m->push_params(@p);
  $this->method($m);
}

sub getBody {
  my $this = shift;
  my $r = qq/{/;
  my $op;

  if($this->type == 0) {
    $op = "!=";
  }
  elsif($this->type == 1) {
    $op = ">";
  }
  elsif($this->type == 2) {
    $op = ">=";
  }
  else {
    die("Invalid Set Expression Type!");
  }

  my $setVar1 = $this->lhs->toMethodCall();
  my $closureSetVar1 = "";
  if($this->lhs->overallClosure()) {
    $closureSetVar1 = "${\$this->lhs->overallType->type} _closure1_ = $setVar1;";
    $setVar1 = "_closure1_";
  }

  my $setVar2 = $this->rhs->toMethodCall();
  my $closureSetVar2 = "";
  if($this->rhs->overallClosure()) {
    $closureSetVar2 = "${\$this->rhs->overallType->type} _closure2_ = $setVar2;";
    $setVar2 = "_closure2_";
  }

  $r .= qq/   bool bvar = true;
    $closureSetVar1
    $closureSetVar2
    if(${setVar1}.size() $op ${setVar2}.size()) { bvar = false; }
    else {
      for(${\$this->lhs->overallType->type}::const_iterator i = ${setVar1}.begin(); i != ${setVar1}.end(); i++) {
        if(!${setVar2}.containsKey(i->first)) {
          bvar = false;
          break;
        }
      }
    }
    return bvar;
  }/;
  return $r;
}

1;

