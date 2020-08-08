# 
# Guard.pm : part of the Mace toolkit for building distributed systems
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
package Mace::Compiler::Guard;

use strict;
use v5.10.1;
use feature 'switch';

use Mace::Util qw(:all);

use Class::MakeMethods::Template::Hash 
    (
     'new' => 'new',
     'scalar' => 'type',      # if set "expr", return string from Expression. otherwise, return from guardStr.
     'object' => ["expr" => { class => "Mace::Compiler::ParseTreeObject::Expression" }],
     'object' => ["state_expr" => { class => "Mace::Compiler::ParseTreeObject::StateExpression" }],
     'string' => 'guardStr',
     'number' => 'line',
     'string' => 'file',
     );

sub toString {
    my $this = shift;
    my %opt = @_;

    my $s = "";

    my $type = $this->type();

    given ($type) {
        when ("expr") {
            my $string;

            if( defined $this->expr() )
            {
              $string = $this->expr()->toString();
            }
            else
            {
              $string = $this->guardStr();
            }

            if($opt{withline} and $this->line() >= 0) {
              return "\n#line ".$this->line().' "'.$this->file()."\"\n".$string."\n// __INSERT_LINE_HERE__\n";
            } elsif($opt{oneline}) {
              my $s = $string;   
              $s =~ s/\n//g;
              return $s;
            } else {
              return $string;
            }
        }
        when ("state_expr") {
            my $string;

            if( defined $this->state_expr() )
            {
              $string = $this->state_expr()->toString();
            }
            else
            {
              $string = $this->guardStr();
            }

            if($opt{withline} and $this->line() >= 0) {
              return "\n#line ".$this->line().' "'.$this->file()."\"\n".$string."\n// __INSERT_LINE_HERE__\n";
            } elsif($opt{oneline}) {
              my $s = $string;   
              $s =~ s/\n//g;
              return $s;
            } else {
              return $string;
            }
        }
        default {
            if($opt{withline} and $this->line() > 0) {
              return "\n#line ".$this->line().' "'.$this->file()."\"\n".$this->guardStr()."\n// __INSERT_LINE_HERE__\n";
            } elsif($opt{oneline}) {
              my $s = $this->guardStr();   
              $s =~ s/\n//g;
              return $s;
            } else {
              return $this->guardStr();
            }
        }
    }
} # toString

sub usedVar {
    my $this = shift;
    my @array = ();

    my $type = $this->type();

    given ($type) {
        when ("expr") {
            @array = $this->expr()->usedVar();
        }
        when ("state_expr") {
            @array = $this->state_expr()->usedVar();
        }
        default {
            @array = ();
        }
    }

    return \@array;

} #usedVar

sub getType {
    my $this = shift;

    return $this->type();

}

1;
