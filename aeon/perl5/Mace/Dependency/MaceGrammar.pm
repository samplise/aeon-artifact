# 
# MaceGrammar.pm : part of the Mace toolkit for building distributed systems
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
package Mace::Dependency::MaceGrammar;

use strict;

use constant MACE => q[

MaceService: <skip: qr{\s* ((/[*] .*? [*]/|(//[^\n]*\n)|([#]line \s* \d+ \s* ["][^"\r\n]+["])) \s*)*}sx> NonProvidesLine(s?) Provides NonServiceLine(s?) ServiceBlock
{
    my @vars;
    if (scalar(@{$item{Provides}}) or scalar(@{$item{ServiceBlock}})) {
        $arg{vars}->{$arg{name}."Service_dep"} = "SET(".$arg{name}."Service_dep \${ServiceClass_h}" .
               join('', map {" \${${_}ServiceClass_dep}"} @{$item{Provides}}) . 
               join('', map {" \${".@{${_}}[0]."ServiceClass_dep}"} @{$item{ServiceBlock}}) .
               ")\n";
        $arg{vars}->{$arg{name}."Service_tgt_dep"} = "SET(".$arg{name}."Service_tgt_dep ServiceClass_dep" .
               join('', map {" ${_}ServiceClass_dep"} @{$item{Provides}}) . 
               join('', map {" ".@{${_}}[0]."ServiceClass_dep"} @{$item{ServiceBlock}}) . 
               ")\n";
    }
    $arg{vars}->{$arg{name}."Service_sv_dep"} = "SET(".$arg{name}."Service_sv_dep ".$arg{name} .
           join('', map {" \${".@{${_}}[1]."Service_sv_dep}"} @{$item{ServiceBlock}}) .
           ")\n";
    $arg{vars}->{$arg{name}."Service_sv_list"} = "SET(".$arg{name}."Service_sv_list ".
           join('', map {" ".@{${_}}[1]} @{$item{ServiceBlock}}) .
           ")\n";
    $return = 1;
}

ProvidesOrService : 'provides' | 'services'
NonProvidesLine : ...!ProvidesOrService /.*?\n/ 

Provides : 'provides' IdCommaList ';'
{
    $return = $item{IdCommaList};
}
| { $return = []; }

NonServiceLine : ...!'services' /.*?\n/ 

ServiceBlock : 'services' '{' ServiceList 
{
    $return = $item{ServiceList}
}
| { $return = []; }


Id : /[_a-zA-Z][a-zA-Z0-9_]*/
IdCommaList : Id(s /,/)

InlineFinal : 'inline' | 'final' | 
ServiceDefault : Id '(' /[^;]*;/ 
{
    $return = $item{Id}
} 
| Id /[^;]*;/ { $return = ""; }
ServiceItem : InlineFinal Id /[^=;]*=/ ServiceDefault
{
    $return = [ $item{Id}, $item{ServiceDefault}];
}
| InlineFinal Id /[^;]*;/
{
    $return = [ $item{Id}, "" ];
}
ServiceList : ServiceItem(s?)

];

1;
