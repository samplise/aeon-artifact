# 
# VariantGrammar.pm : part of the Mace toolkit for building distributed systems
# 
# Copyright (c) 2011, Charles Killian, James W. Anderson
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
package Mace::Compiler::VariantGrammar;

use strict;
use Mace::Compiler::Grammar;
use Mace::Compiler::CommonGrammar;

use constant VARIANT => Mace::Compiler::CommonGrammar::COMMON . q[

VariantsKeyword : /\bvariants\b/ 

Variants : /\bvariants\b/ Id(s /,/) ';' { push(@{$thisparser->{local}{variants}}, @{$item[2]}); }

CopyLookaheadStringLine : FileLine CopyLookaheadString[rule=>$arg{rule}]
{
    $return = "";
    if ($item{CopyLookaheadString} ne "") {
        $return .= "#line ".($item{FileLine}->[0])." \"".$item{FileLine}->[1]."\"\n";
        $return .= $item{CopyLookaheadString};
    }
}

Preamble : CopyLookaheadStringLine[rule=>'VariantsKeyword'] Variants 
{
    $thisparser->{local}{preamble} = $item{CopyLookaheadStringLine} . "\n";
    my $svname = $thisparser->{local}{servicename};
    for my $variant (@{$thisparser->{local}{variants}}) {

        my $preamble = $thisparser->{local}{preamble};
        $preamble =~ s/service\s+$svname\s*;/service $variant;/s;

        if (not defined $thisparser->{local}{linemap}->[$thisline]) {
            print "ERROR at line $thisline\n";
        }

        $thisparser->{local}{"variant:$variant"} = $preamble;
        $thisparser->{local}{"variant:$variant"} .= "#line ".($thisparser->{local}{linemap}->[$thisline])." \"".$thisparser->{local}{filemap}->[$thisline].qq/"\n/;
    }
}
| <error>

VariantFile : <skip: qr{(\xef\xbb\xbf)?\s* ((/[*] .*? [*]/|(//[^\n]*\n)|([#]line \s* \d+ \s* ["][^"\r\n]+["])) \s*)*}sx>
    {
        $thisparser->{local}{servicename} = $arg[0];
        $thisparser->{local}{variants} = [ $arg[0] ];
    }
    Preamble Block(s) StartPos 
    {
        $return = $thisparser->{local}{variants};
    }
| 
    {
        my $lastLine = "";
        my $lastMsg = "";
        for my $i (@{$thisparser->{errors}}) {
            if ($i->[1] ne $lastLine || $i->[0] ne $lastMsg) {
                Mace::Compiler::Globals::error('invalid syntax', $thisparser->{local}{filemap}->[$i->[1]], $thisparser->{local}{linemap}->[$i->[1]], $i->[0]);
                $lastLine = $i->[1];
                $lastMsg = $i->[0];
            }
        }
        $thisparser->{errors} = undef;
    }

Block : VariantBlock | CopyBlock

CopyBlock : CopyLookaheadStringLine[rule=>'VariantList'] 
{
    for my $variant (@{$thisparser->{local}{variants}}) {
        $thisparser->{local}{"variant:$variant"} .= $item{CopyLookaheadStringLine} . "\n";
    }
}

VariantList : /\bvariant/ '<' Id(s /,/) '>' { $return = $item[3]; }

VariantBlock : VariantList '{' StartPos FileLine SemiStatementFoo(s?) EndPos '}' 
{
    my $subst = substr($Mace::Compiler::Grammar::text, $item{StartPos}, 1 + $item{EndPos} - $item{StartPos});
    for my $variant (@{$item{VariantList}}) {
        $thisparser->{local}{"variant:$variant"} .= "\n#line ".($item{FileLine}->[0])." \"".$item{FileLine}->[1]."\"\n";
        $thisparser->{local}{"variant:$variant"} .= $subst . "\n";
    }
}

GetVariant : {
    $return = $thisparser->{local}{"variant:".$arg[0]};
}

]; # VARIANT grammar

1;
