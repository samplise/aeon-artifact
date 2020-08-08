# 
# JoinExpression.pm : part of the Mace toolkit for building distributed systems
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
package Mace::Compiler::Properties::JoinExpression;

use strict;
use base qw{Mace::Compiler::Properties::BinaryOpPropertyItem};

use Class::MakeMethods::Template::Hash
    (
     'new' => 'new',
     'scalar' => "type", 
     'scalar' => "methodName",
    );

sub toString {
    my $this = shift;
    my $r;

    $r .= "or" if($this->type == 0);
    $r .= "and" if($this->type == 1);
    $r .= "xor" if($this->type == 2);
    $r .= "implies" if($this->type == 3);
    $r .= "iff" if($this->type == 4);

    return $this->SUPER::toString($r, @_);
} # toString

sub validate {
  my $this = shift;
  my $sv = shift;
  my @p = @_;

  $this->SUPER::validate($sv, @p);

  my $m = Mace::Compiler::Method->new(name=>$this->methodName, isStatic=>1, returnType=>Mace::Compiler::Type->new(type=>"bool"), body=>$this->getMethodBody());
  $m->push_params(@p);
  $this->method($m);
}

sub getMethodBody {
  my $this = shift;

    my $m = qq/{/;

    if($this->type == 0) {
      $m .= "return ${\$this->lhs->toMethodCall()} || ${\$this->rhs->toMethodCall()}; ";
    }
    elsif($this->type == 1) {
      $m .= "return ${\$this->lhs->toMethodCall()} && ${\$this->rhs->toMethodCall()}; ";
    }
    elsif($this->type == 2) {
      $m .= "return ${\$this->lhs->toMethodCall()} ^ ${\$this->rhs->toMethodCall()}; /*kinda sketchy -- using bitwise or, seems to work.*/  ";
    }
    elsif($this->type == 3) {
      $m .= "return (!${\$this->lhs->toMethodCall()}) || ${\$this->rhs->toMethodCall()}; ";
    }
    elsif($this->type == 4) {
      $m .= "return (!${\$this->lhs->toMethodCall()}) ^ ${\$this->rhs->toMethodCall()}; ";
    }

    $m .= qq/}\n/;

    return $m;
}

1;
