# 
# MHGrammar.pm : part of the Mace toolkit for building distributed systems
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
package Mace::Compiler::MHGrammar;

use strict;
use Mace::Compiler::Grammar;
use Mace::Compiler::ServiceClassCommonGrammar;

use constant MH => Mace::Compiler::ServiceClassCommonGrammar::SERVICE_CLASS_COMMON . q[

CopyBlockPre : StartPos LookaheadString[rule=>'ClassToken'] EndPos
    <defer: $thisparser->{'local'}{'serviceclass'}->pre(substr($Mace::Compiler::Grammar::text, $item{StartPos},
						    1 + $item{EndPos} - $item{StartPos})); >


CopyBlockPost : StartPos ToEnd EndPos
    <defer: $thisparser->{'local'}{'serviceclass'}->post(substr($Mace::Compiler::Grammar::text, $item{StartPos},
						    1 + $item{EndPos} - $item{StartPos})); >

Class : 'auto_type' <commit> AutoType[key=>"serviceclass"] { $thisparser->{'local'}{'serviceclass'}->name(@{$thisparser->{'local'}{'serviceclass'}->auto_types()}[0]->name()); $thisparser->{'local'}{'serviceclass'}->isAutoType(1); }
| 'namespace' Id '{' AutoTypeList '}' ';'
{ $thisparser->{'local'}{'serviceclass'}->name($item{Id}); $thisparser->{'local'}{'serviceclass'}->isNamespace(1); }
| ClassToken Id ClassExt(?) '{' ClassBody '}' ';'
{ $thisparser->{'local'}{'serviceclass'}->name($item{Id}) }
| <error>

File : <skip: qr{(\xef\xbb\xbf)?\s* ((/[*] .*? [*]/|(//[^\n]*\n)|([#]line \s* \d+ \s* ["][^"\r\n]+["])) \s*)*}sx> Reset CopyBlockPre Class CopyBlockPost(?) EOFile
{ $return = $thisparser->{'local'}{'serviceclass'} } | <error>

ClassToken : /serviceclass/ { $thisparser->{'local'}{'serviceclass'}->isServiceClass(1) }
| /handler/ { $thisparser->{'local'}{'serviceclass'}->isHandler(1) }
| /auto_type/ 
| /namespace/
| <error>

ClassExt : ':' Id(s /,/) { $thisparser->{'local'}{'serviceclass'}->superclasses(@{$item[2]}); }

ClassBody : ...!'}' <commit> (...'mace' <commit> MaceBlock | ...'auto_type' <commit> MHAutoType | Statement) ClassBody |

AutoTypeList : ...!'}' <commit> (...'auto_type' <commit> MHAutoType | Statement)  AutoTypeList |

MHAutoType : 'auto_type' <commit> AutoType[key=>"serviceclass"]

MaceBlock : StartPos 'mace' <commit> ('provides'|'services') BraceBlockFoo EndPos
{
    my $maceLit = "\n#ifdef MACE_VOODOO\n" .
	substr($Mace::Compiler::Grammar::text, $item{StartPos},
	       1 + $item{EndPos} - $item{StartPos}) .
	       "\n#endif // MACE_VOODOO\n";
    $thisparser->{local}{serviceclass}->maceLiteral($thisparser->{local}{serviceclass}->maceLiteral() . $maceLit);
}

Statement : Handlers | MethodDecl
{ $thisparser->{'local'}{'serviceclass'}->push_methods($item{MethodDecl}) }
| ProtectionToken <commit> <error: invalid token $item{ProtectionToken}>
| StartPos SemiStatementFoo EndPos
{
    $thisparser->{'local'}{'serviceclass'}->push_literals(substr($Mace::Compiler::Grammar::text,
						      $item{StartPos},
						      1 + $item{EndPos} - $item{StartPos}));
}

Handlers : 'handlers' Id(s /,/) { $thisparser->{'local'}{'serviceclass'}->handlers(@{$item[2]}); } ';'

]; # MH grammar

1;
