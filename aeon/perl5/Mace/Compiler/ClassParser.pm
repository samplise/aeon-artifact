# 
# ClassParser.pm : part of the Mace toolkit for building distributed systems
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
package Mace::Compiler::ClassParser;

use strict;

use Mace::Compiler::ServiceClass;
use Mace::Compiler::Method;
use Mace::Compiler::Param;
use Mace::Compiler::Grammar;
use Mace::Compiler::ClassParserRecDescent;

use Mace::Compiler::ParseTreeObject::BraceBlock;
use Mace::Compiler::ParseTreeObject::Expression;
use Mace::Compiler::ParseTreeObject::MethodTerm;
use Mace::Compiler::ParseTreeObject::ParsedAbort;
use Mace::Compiler::ParseTreeObject::ParsedAssertMsg;
use Mace::Compiler::ParseTreeObject::ParsedAssert;
use Mace::Compiler::ParseTreeObject::ParsedBinaryAssignOp;
use Mace::Compiler::ParseTreeObject::ParsedCatches;
use Mace::Compiler::ParseTreeObject::ParsedCatch;
use Mace::Compiler::ParseTreeObject::ParsedDefaultCase;
use Mace::Compiler::ParseTreeObject::ParsedDoWhile;
use Mace::Compiler::ParseTreeObject::ParsedElseIf;
use Mace::Compiler::ParseTreeObject::ParsedElseIfs;
use Mace::Compiler::ParseTreeObject::ParsedElse;
use Mace::Compiler::ParseTreeObject::ParsedExpectStatement;
use Mace::Compiler::ParseTreeObject::ParsedExpression;
use Mace::Compiler::ParseTreeObject::ParsedFCall;
use Mace::Compiler::ParseTreeObject::ParsedForLoop;
use Mace::Compiler::ParseTreeObject::ParsedForUpdate;
use Mace::Compiler::ParseTreeObject::ParsedForVar;
use Mace::Compiler::ParseTreeObject::ParsedIf;
use Mace::Compiler::ParseTreeObject::ParsedLogging;
use Mace::Compiler::ParseTreeObject::ParsedLValue;
use Mace::Compiler::ParseTreeObject::ParsedOutput;
use Mace::Compiler::ParseTreeObject::ParsedPlusPlus;
use Mace::Compiler::ParseTreeObject::ParsedCaseOrDefault;
use Mace::Compiler::ParseTreeObject::ParsedReturn;
use Mace::Compiler::ParseTreeObject::ParsedSwitchCase;
use Mace::Compiler::ParseTreeObject::ParsedSwitchCases;
use Mace::Compiler::ParseTreeObject::ParsedSwitchConstant;
use Mace::Compiler::ParseTreeObject::ParsedSwitch;
use Mace::Compiler::ParseTreeObject::ParsedMacro;
use Mace::Compiler::ParseTreeObject::ParsedTryCatch;
use Mace::Compiler::ParseTreeObject::ParsedVar;
use Mace::Compiler::ParseTreeObject::ParsedWhile;
use Mace::Compiler::ParseTreeObject::StatementBlock;
use Mace::Compiler::ParseTreeObject::StatementOrBraceBlock;
use Mace::Compiler::ParseTreeObject::SemiStatement;
use Mace::Compiler::ParseTreeObject::ScopedId;
use Mace::Compiler::ParseTreeObject::ArrayIndex;
use Mace::Compiler::ParseTreeObject::ArrayIndOrFunction;
use Mace::Compiler::ParseTreeObject::ArrayIndOrFunctionParts;
use Mace::Compiler::ParseTreeObject::Expression1;
use Mace::Compiler::ParseTreeObject::Expression2;
use Mace::Compiler::ParseTreeObject::ExpressionLValue;
use Mace::Compiler::ParseTreeObject::ExpressionLValue1;
use Mace::Compiler::ParseTreeObject::ExpressionLValue2;
use Mace::Compiler::ParseTreeObject::ExpressionOrAssignLValue;
use Mace::Compiler::ParseTreeObject::ExpressionOrAssignLValue1;
use Mace::Compiler::ParseTreeObject::StateExpression;

use Mace::Util qw(:all);


use Parse::RecDescent;

$::RD_ERRORS = 1; # Make sure the parser dies when it encounters an error
$::RD_WARN   = 1; # Enable warnings. This will warn on unused rules &c.
$::RD_HINT   = 1; # Give out hints to help fix problems.
#$::RD_TRACE  = 1;

use Class::MakeMethods::Template::Hash
    (
    'new --and_then_init' => 'new',
    'object' => ['parser' => { class => "Parse::RecDescent" }],
    'object' => ['class' => { class => "Mace::Compiler::ServiceClass" }],
    );

sub init {
    my $this = shift;
    my $p = Mace::Compiler::ClassParserRecDescent->new();
    $this->parser($p);
} # init

sub parse {
    my $this = shift;
    my $t = shift;
    my $classname = shift;
    my $linemap = shift or die("no linemap!");
    my $filemap = shift or die("no filemap!");
    my $offsetmap = shift or die("no offsetmap!");

    $this->parser()->{local}{linemap} = $linemap;
    $this->parser()->{local}{filemap} = $filemap;
    $this->parser()->{local}{offsetmap} = $offsetmap;

    $Mace::Compiler::Grammar::text = $t;
    my $sc = undef;
    if (defined($classname)) {
	$sc = $this->parser()->NamedFile($t, 0, $classname) || die "syntax error\n";
    }
    else {
	$sc = $this->parser()->File($t) || die "syntax error\n";
    }

    # open(OUT, ">>", "/tmp/foo");
    # print OUT $sc->toString(noline=>1);
    # close(OUT);

    $this->class($sc);
    return $sc;
} # parse

1;
