# 
# Equation.pm : part of the Mace toolkit for building distributed systems
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
package Mace::Compiler::Properties::Equation;

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
    return $this->SUPER::toString($this->type, @_);
} # toString

sub validate {
  my $this = shift;
  my $sv = shift;
  my @p = @_;

  $this->SUPER::validate($sv, @p);

  unless($this->lhs->null()) {
    if($this->lhs->varType == 1 and $this->rhs->varType == 0) {
      Mace::Compiler::Globals::error("bad_property", -1, "lhs is of type 1 and rhs is of type 0");
    }
    elsif($this->lhs->varType == 0 and $this->rhs->varType == 1) {
      Mace::Compiler::Globals::error("bad_property", -1, "lhs is of type 0 and rhs is of type 1");
    }

    if($this->lhs->varType() != 2) {
      $this->rhs->varType($this->lhs->varType());
      $this->SUPER::validate($sv, @p);
    }
    elsif($this->rhs->varType() != 2) {
      $this->lhs->varType($this->rhs->varType());
      $this->SUPER::validate($sv, @p);
    }
  }


  my $m = Mace::Compiler::Method->new(name=>$this->methodName, isStatic=>1, returnType=>Mace::Compiler::Type->new(type=>"bool"), body=>$this->getBody());
  $m->push_params(@p);
  $this->method($m);
}

sub getBody {
    my $this = shift;
    my $m = qq/{/;

    if(! $this->lhs->null() ) {
      $m .= qq/ return ( ${\$this->lhs->toMethodCall()} ${\$this->type} ${\$this->rhs->toMethodCall()} ); /;
    } else {
      $m .= qq/ return ( ${\$this->type} ${\$this->rhs->toMethodCall()} ); /;
    }

    $m .= qq/}
    /;
    return $m;
}

1;
