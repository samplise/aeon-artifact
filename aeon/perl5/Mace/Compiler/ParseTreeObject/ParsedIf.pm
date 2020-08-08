# 
# ParsedIf.pm : part of the Mace toolkit for building distributed systems
# 
# Copyright (c) 2010, Sunghwan Yoo, Charles Killian
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in
#      the documentation and/or other materials provided with the
#      distribution.
#    * Neither the names of Duke University nor The University of
#      California, San Diego, nor the names of the authors or contributors
#      may be used to endorse or promote products derived from
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
package Mace::Compiler::ParseTreeObject::ParsedIf;

use strict;
use v5.10.1;
use feature 'switch';

use Class::MakeMethods::Template::Hash
    (
     'new' => 'new',
     'scalar' => 'type',
     'object' => ["parsed_expr" => { class => "Mace::Compiler::ParseTreeObject::ParsedExpression" }],
     'object' => ["expr_or_assign" => { class => "Mace::Compiler::ParseTreeObject::ExpressionOrAssignLValue" }],
     'object' => ["stmt_or_block" => { class => "Mace::Compiler::ParseTreeObject::StatementOrBraceBlock" }],
     'object' => ["parsed_else_ifs" => { class => "Mace::Compiler::ParseTreeObject::ParsedElseIfs" }],
     'object' => ["parsed_else" => { class => "Mace::Compiler::ParseTreeObject::ParsedElse" }],
    );

sub toString {
    my $this = shift;
    my $s;

    given ($this->type()) {
        when ("parsed_expression")
            {
                $s = "if ( ".$this->parsed_expr()->toString()." ) { ".$this->stmt_or_block()->toString()." }"; 
                if ($this->parsed_else_ifs()->toString() ne "") {
                    $s .= " ".$this->parsed_else_ifs()->toString();
                } 
                if ($this->parsed_else()->toString() ne "") {
                    $s .= " ".$this->parsed_else()->toString();
                }

            }
        when ("expression_or_assign_lvalue")
            {
                $s = "if ( ".$this->expr_or_assign()->toString()." ) { ".$this->stmt_or_block()->toString()." }";
                if ($this->parsed_else_ifs()->toString() ne "") {
                    $s .= " ".$this->parsed_else_ifs()->toString();
                } 
                if ($this->parsed_else()->toString() ne "") {
                    $s .= " ".$this->parsed_else()->toString();
                }
            }
        default { return "ParsedIf:NOT-PARSED"; }
    }

    return $s;
}

sub usedVar {
    my $this = shift;
    my @array = ();

    my $type = $this->type();

    given ($type) {
        when ("parsed_expression") { @array = ($this->parsed_expr()->usedVar(),$this->stmt_or_block()->usedVar(),$this->parsed_else_ifs()->usedVar(),$this->parsed_else()->usedVar()); }
        when ("expression_or_assign_lvalue") { @array = ($this->expr_or_assign()->usedVar(),$this->stmt_or_block()->usedVar(),$this->parsed_else_ifs()->usedVar(),$this->parsed_else()->usedVar()); }
        default { return @array; }
    }

    return @array;
}


1;
