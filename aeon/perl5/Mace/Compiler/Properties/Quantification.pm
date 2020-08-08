# 
# Quantification.pm : part of the Mace toolkit for building distributed systems
# 
# Copyright (c) 2011, Charles Killian, Karthik Nagaraj
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
package Mace::Compiler::Properties::Quantification;

use strict;
use Class::MakeMethods::Utility::Ref qw( ref_clone );
use base qw{Mace::Compiler::Properties::MethodPropertyItem};

use Class::MakeMethods::Template::Hash
    (
     'new' => 'new',
     'number' => "type", # 0 - more than, 1 - at least, 2 - exactly, 3 - at most, 4 - less than, 5 - specifically not
     'scalar' => "quantity",
     'scalar' => "varname",
     'object' => ["set" => { class => "Mace::Compiler::Properties::PropertyItem"}],
     'object' => ["expression" => { class => "Mace::Compiler::Properties::PropertyItem"}],
     'scalar' => "methodName",
     );

sub toString {
    my $this = shift;
    my %args = @_;

    my $typeStr = "";
    $typeStr = "more than" if ($this->type == 0);
    $typeStr = "at least" if ($this->type == 1);
    $typeStr = "exactly" if ($this->type == 2);
    $typeStr = "at most" if ($this->type == 3);
    $typeStr = "less than" if ($this->type == 4);
    $typeStr = "specifically not" if ($this->type == 5);

    # Perfecting the Math grammar a little
    my $prefix = "for $typeStr";
    $prefix = "for" if ($this->quantity eq "all");
    $prefix = "there" if ($this->quantity eq "exists");

    my $r = qq/( $prefix ${\$this->quantity} elements ${\$this->varname} in ${\$this->set->toString(%args)} it is true that ${\$this->expression->toString(%args)} )/;
}

sub toMethod {
    my $this = shift;

    my $r = $this->SUPER::toMethod(@_);
    $r .= $this->set->toMethod(@_);
    $r .= $this->expression->toMethod(@_);
    
    return $r;
} # toMethod

sub toMethodDeclare {
    my $this = shift;

    my $r = $this->SUPER::toMethodDeclare(@_);
    $r .= $this->set->toMethodDeclare(@_);
    $r .= $this->expression->toMethodDeclare(@_);
    
    return $r;
} # toMethod

sub getOp {
  my $this = shift;
  if($this->type == 0) {
    return ">";
  }
  elsif($this->type == 1) {
    return ">=";
  }
  elsif($this->type == 2) {
    return "==";
  }
  elsif($this->type == 3) {
    return "<=";
  }
  elsif($this->type == 4) {
    return "<";
  }
  elsif($this->type == 5) {
    return "!=";
  }
  else {
    die("Invalid type!");
  }
}

sub validate {
  my $this = shift;
  my $sv = shift;
  my @p = @_;

#  print "validating Quantification ${\$this->methodName}\n";

  $this->set->validate($sv, @p);

  my $t = Mace::Compiler::Type->new(type=>"${\$this->set()->overallType()->type()}::const_iterator", isConst=>1, isRef=>1);
  my $p = Mace::Compiler::Param->new(type=>$t, name=>$this->varname);
  if($t->type eq $p[0]->type->type."::const_iterator" or $t->type eq "mace::map<int, ".$p[0]->type->type."::const_iterator, mace::SoftState>::const_iterator") {
    $p->flags('varType', 1);
  } else {
    $p->flags('varType', 2);
  }

  $this->expression->validate($sv, @p, $p);

#  if($this->quantity eq "all") {
#    $this->quantity($this->set()->toMethodCall().".size()");
#  }

  my $m = Mace::Compiler::Method->new(name=>$this->methodName, isStatic=>1, returnType=>Mace::Compiler::Type->new(type=>"bool"), body=>$this->getBody($sv, @p));
  $m->push_params(@p);
  $this->method($m);
}

sub getBody {
  my $this = shift;
  my $sv = shift;
  my @p = @_;
  my $setVar = $this->set->toMethodCall();
  my $closureSetVar = "";
  if($this->set->overallClosure()) {
    $closureSetVar = "${\$this->set->overallType->type} _closure_ = $setVar;";
    $setVar = "_closure_";
  }
#  print "getBody: 
  if(! defined($this->set()->overallType()) ) {
    Mace::Compiler::Globals::error("XXX", -1, "Guessed type for ".$this->set->toString()." undefined!\n");
    return "{ return false; }
    ";
  }
  my $sel = "";
  my $log = "";
  if($sv->traceLevel() > 1) {
    $sel = qq/ADD_SELECTORS("${\$sv->name()}::${\$this->methodName}");/;
    if ($this->quantity eq "all" or $this->quantity eq "exists") {
      $log = qq/maceout << "quantity: " << (quantity?"true":"false") << " size: " << $setVar.size() <<  " ${\$this->toString()}" << Log::endl;/;
    } else {
      $log = qq/maceout << "quantity: " << quantity << " compute: " << ${\$this->quantity} << " size: " << $setVar.size() << " ${\$this->toString()}" << Log::endl;/;
    }
  }

  my $body = qq/{
      $sel
      /;
  if ($this->quantity eq "all") {
    $body .= qq/bool quantity = true;/;
  } elsif ($this->quantity eq "exists") {
    $body .= qq/bool quantity = false;/;
  } else {
    $body .= qq/size_t quantity = 0;/;
  }
  $body .= qq/
    $closureSetVar
    for (${\$this->set()->overallType()->type()}::const_iterator ${\$this->varname} = $setVar.begin(); ${\$this->varname} != $setVar.end(); ${\$this->varname}++) {
    /;

  if ($this->quantity eq "all") {
    $body .= qq/if (!${\$this->expression->toMethodCall()}) {
                  quantity = false; break;
                }
                }
                $log
                return quantity;
                /;
  } elsif ($this->quantity eq "exists") {
    $body .= qq/if (${\$this->expression->toMethodCall()}) {
                  quantity = true; break;
                }
                }
                $log
                return quantity;
                /;
  } else {
    $body .= qq/if (${\$this->expression->toMethodCall()}) {
                  quantity++;
                }
                }
                $log
                return quantity ${\$this->getOp()} ${\$this->quantity};
                /;
  }
  
  $body .=qq/}
             /;

  return $body;
}

1;
