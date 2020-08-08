# 
# Property.pm : part of the Mace toolkit for building distributed systems
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
package Mace::Compiler::Properties::Property;

use strict;
use base qw{Mace::Compiler::Properties::MethodPropertyItem};

use Class::MakeMethods::Template::Hash
    (
     'new' => 'new',
     'scalar' => "name",
     'object' => ["property" => {class => "Mace::Compiler::Properties::PropertyItem"}],

    );

sub toString {
    my $this = shift;
    my %args = @_;

    return "Property ${\$this->name} : ${\$this->property->toString()}";
} # toString

sub toMethod {
  my $this = shift;
  my $r = $this->SUPER::toMethod(@_);
  $r .= $this->property->toMethod(@_);
}

sub toMethodDeclare {
  my $this = shift;
  my $r = $this->SUPER::toMethodDeclare(@_);
  $r .= $this->property->toMethodDeclare(@_);
}

sub validate {
  my $this = shift;
  my $sv = shift;
  my %args = @_;

#  print "validating property ${\$this->name}\n";

#  my $t = Mace::Compiler::Type->new(type=>"mace::map<MaceKey, ${\$sv->name}Service*>", isConst=>1, isRef=>1);
  my $t = Mace::Compiler::Type->new(type=>"_NodeMap_", isConst=>1, isRef=>1);
  my $p = Mace::Compiler::Param->new(type=>$t, name=>"_nodes_");
  my $t2 = Mace::Compiler::Type->new(type=>"_KeyMap_", isConst=>1, isRef=>1);
  my $p2 = Mace::Compiler::Param->new(type=>$t2, name=>"_keys_");

  $this->property->validate($sv, $p, $p2);

  my $m = Mace::Compiler::Method->new(name=>"modelProperty_${\$this->name}", isStatic=>1, returnType=>Mace::Compiler::Type->new(type=>"bool"), body=>qq/{ ADD_SELECTORS("${\$sv->name}::modelProperty_${\$this->name}");
    bool retval = ${\$this->property->toMethodCall()};
    maceout << "Property ${\$this->name}: " << retval << Log::endl;
    return retval; 
  }
  /);
  $m->push_params($p, $p2);
  $this->method($m);
}

1;
