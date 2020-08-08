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
package Mace::Dependency::MHGrammar;

use strict;

use constant MH => q[

NonServiceClassLine : ...!'serviceclass' /.*?\n/ 

NonHandlerLine : ...!'handlers' /.*?\n/ 

Id : /[_a-zA-Z][a-zA-Z0-9_]*/

IdCommaList : Id(s /,/)

ServiceClassExtend : ':' <commit> IdCommaList
{
    $return = $item{IdCommaList};
}
| { $return = []; }

Handlers : 'handlers' IdCommaList ';' 
{
    $return = $item{IdCommaList};
}
| { $return = []; }

ServiceClassBody : 'serviceclass' Id ServiceClassExtend '{' NonHandlerLine(s?) Handlers
{
    if (scalar(@{$item{ServiceClassExtend}}) or scalar(@{$item{Handlers}})) {
        $arg{vars}->{$item{Id}."ServiceClass_dep"} = "SET(".$item{Id}."ServiceClass_dep \${".$item{Id}."ServiceClass_h} \${ServiceClass_h}" .
             join('', map {" \${${_}ServiceClass_dep}"} @{$item{ServiceClassExtend}}) .
             join('', map {" \${${_}Handler_dep}"} @{$item{Handlers}}) .
             ")\n";
    }
}

ServiceClass : <skip: qr{\s* ((/[*] .*? [*]/|(//[^\n]*\n)|([#]line \s* \d+ \s* ["][^"\r\n]+["])) \s*)*}sx> NonServiceClassLine(s?) ServiceClassBody[%arg](?)

];

1;
