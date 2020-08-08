# 
# BinaryOpPropertyItem.pm : part of the Mace toolkit for building distributed systems
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
package Mace::Compiler::Properties::BinaryOpPropertyItem;

use strict;
use base qw{Mace::Compiler::Properties::MethodPropertyItem};

use Class::MakeMethods::Template::Hash
    (
     'object' => ["lhs" => { class => "Mace::Compiler::Properties::PropertyItem"}],
     'object' => ["rhs" => { class => "Mace::Compiler::Properties::PropertyItem"}],
    );

sub toString {
    my $this = shift;
    my $op = shift;
    my %args = @_;

    if(!$this->lhs->null()) {
      return qq/( ${\$this->lhs->toString(%args)} $op ${\$this->rhs->toString(%args)} )/;
    } else {
      return qq/( $op ${\$this->rhs->toString(%args)})/;
    }
}

sub toMethod {
  my $this = shift;
  my $s = $this->SUPER::toMethod(@_);
  if(!$this->lhs->null()) {
    $s .= $this->lhs->toMethod(@_);
  }
  $s .= $this->rhs->toMethod(@_);
  return $s;
}

sub toMethodDeclare {
  my $this = shift;
  my $s = $this->SUPER::toMethodDeclare(@_);
  if(!$this->lhs->null()) {
    $s .= $this->lhs->toMethodDeclare(@_);
  }
  $s .= $this->rhs->toMethodDeclare(@_);
  return $s;
}

sub validate {
  my $this = shift;
  $this->lhs->validate(@_);
  $this->rhs->validate(@_);
}

1;
