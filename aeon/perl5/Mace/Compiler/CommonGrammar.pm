#
# CommonGrammar.pm : part of the Mace toolkit for building distributed systems
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
package Mace::Compiler::CommonGrammar;

use strict;
use Mace::Compiler::Grammar;

use constant COMMON => q{

Word : /\S*/
LookaheadWord : ...!<matchrule: $arg{rule}>[%arg] Word
LookaheadString : LookaheadWord[%arg](s?)
CopyLookaheadString : StartPos LookaheadString[%arg] EndPos
{
  $return = substr($Mace::Compiler::Grammar::text, $item{StartPos},
      1 + $item{EndPos} - $item{StartPos});
}
ToEnd : Word(s?)

StartPos : // { $thisoffset }
| <error>

EndPos : { $prevoffset }
Line : '' { $thisline }
Column : { $thiscolumn }
FileLine : '' {
    my $line = 0;
    while (defined $thisparser->{local}{offsetmap}->[$line] and $thisparser->{local}{offsetmap}->[$line] <= $thisoffset) {
        $line++;
    }
    $line--;
    if ($line <= 0) {
        confess("Line $line < 0 : thisline $thisline thisoffset $thisoffset");
    }
    # if (not defined $thisparser->{local}{linemap}->[$thisline]) {
    #     print "WARNING: offsetline $line thisline $thisline\n";
    # }
    #if (not defined $thisparser->{local}{linemap}->[$line]) {
    #    # $return = [ 0, "error", $thisline ];
    #    open(OUT, ">", "/tmp/foo");
    #    print OUT "Thisline: $thisline prevline $prevline\n";
    #    my $i = 0;
    #    for my $l (@{$thisparser->{local}{linemap}}) {
    #        print OUT "$i - $l - ".$thisparser->{local}{filemap}->[$i]."\n";
    #        $i++;
    #    }
    #    print OUT "File::\n";
    #    print OUT $Mace::Compiler::Grammar::text;
    #    print OUT "Remaining::\n$text\n";
    #    close(OUT);
    #    confess("Invalid line $thisline");
    #} else {
        $return = [ $thisparser->{local}{linemap}->[$line], $thisparser->{local}{filemap}->[$line], $line ];
    #}
    }
FileLineEnd : {
    my $line = 0;
    while (defined $thisparser->{local}{offsetmap}->[$line] and $thisparser->{local}{offsetmap}->[$line] < $prevoffset) {
        $line++;
    }
    $line--;
    # if (not defined $thisparser->{local}{linemap}->[$prevline]) {
    #     print "WARNING: offsetline $line prevline $prevline\n";
    # }
    # if (not defined $thisparser->{local}{linemap}->[$line]) {
    #     # $return = [ 0, "error", $prevline ];
    #     open(OUT, ">", "/tmp/foo");
    #     print OUT "prevline: $prevline\n";
    #     my $i = 0;
    #     for my $l (@{$thisparser->{local}{linemap}}) {
    #         print OUT "$i - $l - ".$thisparser->{local}{filemap}->[$i]."\n";
    #         $i++;
    #     }
    #     print OUT "File::\n";
    #     print OUT $Mace::Compiler::Grammar::text;
    #     print OUT "Remaining::\n$text\n";
    #     close(OUT);
    #     confess("Invalid line $prevline");
    # } else {
        $return = [ $thisparser->{local}{linemap}->[$line], $thisparser->{local}{filemap}->[$line], $line ];
    # }
    }

Id : /[_a-zA-Z][a-zA-Z0-9_]*/

SemiStatementToken : m|[^;{}][^;{}/]*|

SemiStatementBegin : SemiStatementToken(s)

BraceBlockFoo : '{' SemiStatementFoo(s?) '}' { $return = $item[2]; }

BraceBlock : '{' SemiStatement(s?) '}'
{
    my $node = Mace::Compiler::ParseTreeObject::BraceBlock->new();
    $node->not_null(scalar(@{$item[2]}));

    if (scalar(@{$item[2]})) {
        $node->semi_statements(@{$item[2]});
    }
    $return = $node;
    #$return = $item[2];    # original
}


Enum : /enum\s/ Id '{' SemiStatementBegin '}'

ParsedExpression : Expression
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedExpression->new(expr=>$item{Expression});
    }

ParsedReturn : /return\b/ ';'
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedReturn->new(type=>0);
    }
| /return\b/ ParsedExpression ';'
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedReturn->new(type=>1, parsed_expr=>$item{ParsedExpression});
    }

ParsedVar : StaticToken(?) Parameter[%arg, initializerOk => 1]
{
  if (scalar(@{$item[1]})) {
    $return = Mace::Compiler::ParseTreeObject::ParsedVar->new(is_static=>1, is_semi=>$arg{semi}, parameter=>$item{Parameter}->toString(noline => 1));
  } else {
    $return = Mace::Compiler::ParseTreeObject::ParsedVar->new(is_static=>0, is_semi=>$arg{semi}, parameter=>$item{Parameter}->toString(noline => 1));
  }
}
| <error>

ParsedElse: /else\b/ <commit> StatementOrBraceBlock
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedElse->new(null=>0, stmt_or_block=>$item{StatementOrBraceBlock});
    }
| <error?> <reject>
|   {
      $return = Mace::Compiler::ParseTreeObject::ParsedElse->new(null=>1);
    }

ElseAndIf: /else\b/ /if\b/

ParsedElseIfs: ...ElseAndIf <commit> ParsedElseIf ParsedElseIfs
    {
      $return = Mace::Compiler::ParseTreeObject::ParsedElseIfs->new(null=>0, parsed_else_if=>$item{ParsedElseIf}, parsed_else_ifs=>$item{ParsedElseIfs});
    }
| <error?> <reject>
|   {
      $return = Mace::Compiler::ParseTreeObject::ParsedElseIfs->new(null=>1);
    }

ParsedElseIf: ElseAndIf <commit> '(' ParsedExpression ')' StatementOrBraceBlock
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedElseIf->new(type=>"parsed_expression", parsed_expr=>$item{ParsedExpression}, stmt_or_block=>$item{StatementOrBraceBlock});
    }
| ElseAndIf <commit> '(' ExpressionOrAssignLValue ')' StatementOrBraceBlock
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedElseIf->new(type=>"expression_or_assign_lvalue", expr_or_assign=>$item{ExpressionOrAssignLValue}, stmt_or_block=>$item{StatementOrBraceBlock});
    }
| <error>

ParsedIf : /if\b/ '(' ParsedExpression ')' StatementOrBraceBlock ParsedElseIfs ParsedElse
    {
      $return = Mace::Compiler::ParseTreeObject::ParsedIf->new(type=>"parsed_expression", parsed_expr=>$item{ParsedExpression}, stmt_or_block=>$item{StatementOrBraceBlock}, parsed_else_ifs=>$item{ParsedElseIfs}, parsed_else=>$item{ParsedElse});
    }
| /if\b/ '(' ExpressionOrAssignLValue ')' StatementOrBraceBlock ParsedElseIfs ParsedElse
    {
      $return = Mace::Compiler::ParseTreeObject::ParsedIf->new(type=>"expression_or_assign_lvalue", expr_or_assign=>$item{ExpressionOrAssignLValue}, stmt_or_block=>$item{StatementOrBraceBlock}, parsed_else_ifs=>$item{ParsedElseIfs}, parsed_else=>$item{ParsedElse});
    }
| <error>

ParsedDoWhile : /do\b/ <commit> StatementOrBraceBlock /while\b/ '(' ParsedExpression ')' (';')(?)
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedDoWhile->new(stmt_or_block=>$item{StatementOrBraceBlock}, parsed_expr=>$item{ParsedExpression});
    }

ParsedWhile : /while\b/ <commit> '(' ParsedExpression ')' StatementOrBraceBlock
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedWhile->new(parsed_expr=>$item{ParsedExpression}, stmt_or_block=>$item{StatementOrBraceBlock});
    }

ParsedAbort : 'ABORT' '(' QuotedString ')' ';'
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedAbort->new(quoted_string=>$item{QuotedString});
    }
| <error>

ParsedAssertMsg : 'ASSERTMSG' '(' Expression ',' QuotedString ')' ';'
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedAssertMsg->new(expr=>$item{Expression}, quoted_string=>$item{QuotedString});
    }
| <error>

ParsedAssert : 'ASSERT' '(' Expression ')' ';'
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedAssert->new(expr=>$item{Expression});
    }
| <error>

ParsedFCall : ExpressionLValue[parseFunctionCall => 1]
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedFCall->new(expr_lvalue=>$item{ExpressionLValue});
    }
| <error>


ParsedLValue : ParsedPlusPlus
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedLValue->new(type=>"parsed_plus_plus", parsed_plus_plus=>$item{ParsedPlusPlus});
    }
| ParsedBinaryAssignOp
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedLValue->new(type=>"parsed_binary_assign_op", parsed_binary_assign_op=>$item{ParsedBinaryAssignOp});
    }
| ExpressionLValue
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedLValue->new(type=>"expression_lvalue", parsed_expr_lvalue=>$item{ExpressionLValue});
    }
#ParsedLValue : ParsedPlusPlus | ParsedBinaryAssignOp | ExpressionLValue
| <error>

# ??
#| ScopedId '[' Expression ']' { $return = $item{ScopedId} . '[' . $item{Expression} . ']'; }

ParsedBinaryAssignOp : ExpressionLValue AssignBinaryOp Expression CheckSemi[%arg]
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedBinaryAssignOp->new(type=>"expression", expr_lvalue=>$item{ExpressionLValue}, assign_binary_op=>$item{AssignBinaryOp}, expr=>$item{Expression}, is_semi=>$arg{is_semi});
    }
| ExpressionLValue AssignBinaryOp <commit> ParsedLValue CheckSemi[%arg]
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedBinaryAssignOp->new(type=>"parsed_lvalue", expr_lvalue=>$item{ExpressionLValue}, assign_binary_op=>$item{AssignBinaryOp}, parsed_lvalue=>$item{ParsedLValue}, is_semi=>$item{is_semi});
    }
| <uncommit> <defer: print "ParsedBinaryAssignOp failed.";> <error?> <error>

ParsedPlusPlus : ExpressionLValue '++'
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedPlusPlus->new(type=>"post++", expr_lvalue=>$item{ExpressionLValue});
    }
| '++' ExpressionLValue
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedPlusPlus->new(type=>"pre++", expr_lvalue=>$item{ExpressionLValue});
    }
| ExpressionLValue '--'
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedPlusPlus->new(type=>"post--", expr_lvalue=>$item{ExpressionLValue});
    }
| '--' ExpressionLValue
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedPlusPlus->new(type=>"pre--", expr_lvalue=>$item{ExpressionLValue});
    }


ParsedForVar : ParsedVar
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedForVar->new(type=>"parsed_var", parsed_var=>$item{ParsedVar});
    }
| ParsedBinaryAssignOp
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedForVar->new(type=>"parsed_binary_assign_op", parsed_binary_assign_op=>$item{ParsedBinaryAssignOp});
    }
|
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedForVar->new(type=>"null");
    }

ParsedForUpdate : ParsedPlusPlus
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedForUpdate->new(type=>"parsed_plus_plus", parsed_plus_plus=>$item{ParsedPlusPlus});
    }
| ParsedBinaryAssignOp
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedForUpdate->new(type=>"parsed_binary_assign_op", parsed_binary_assign_op=>$item{ParsedBinaryAssignOp});
    }
|
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedForUpdate->new(type=>"null");
    }

ParsedForLoop : /for\b/ '(' ParsedForVar ';' Expression ';' ParsedForUpdate ')' StatementOrBraceBlock
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedForLoop->new(parsed_for_var=>$item{ParsedForVar}, expr=>$item{Expression}, parsed_for_update=>$item{ParsedForUpdate}, stmt_or_block=>$item{StatementOrBraceBlock});
    }

OutputStream1 : 'maceout' | 'maceerr' | 'macewarn' | 'macedbg' '(' Number ')' {$return = "macedbg(".$item{Number}.")";} | 'cout' | 'cerr' | 'std::cout' | 'std::cerr' | 'kenout' | <error>

OutputStream : StartPos OutputStream1 EndPos
{
    $return = substr($Mace::Compiler::Grammar::text, $item{StartPos},
             1 + $item{EndPos} - $item{StartPos});
}

OutputOperator1 : '<<' | <error>

OutputOperator : StartPos OutputOperator1 EndPos
{
    $return = substr($Mace::Compiler::Grammar::text, $item{StartPos},
             1 + $item{EndPos} - $item{StartPos});
}

ParsedLogging : OutputStream <commit> OutputOperator Expression ';'
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedLogging->new(output_stream=>$item{OutputStream}, output_operator=>$item{OutputOperator}, expr=>$item{Expression});
    }

ParsedOutput : ExpressionLValue OutputOperator <commit> Expression
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedOutput->new(expr_lvalue=>$item{ExpressionLValue}, output_operator=>$item{OutputOperator}, expr=>$item{Expression});
    }

ParsedDefaultCase : 'default' <commit> ':' SemiStatement(s?)
    {
        my $node = Mace::Compiler::ParseTreeObject::ParsedDefaultCase->new();

        $node->type("default");
        $node->not_null(scalar(@{$item[-1]}));
        if (scalar(@{$item[-1]})) {
            $node->semi_statements(@{$item[-1]});
        }
        $return = $node;
    }
| <error?> <reject>
|
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedDefaultCase->new(type=>"null");
    }

ParsedSwitchConstant : Number
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedSwitchConstant->new(type=>"number", val=>$item{Number});
    }
| Character
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedSwitchConstant->new(type=>"character", val=>$item{Character});
    }
| ScopedId
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedSwitchConstant->new(type=>"scoped_id", scoped_id=>$item{ScopedId});
    }

ParsedSwitchCase : 'case' ParsedSwitchConstant ':' SemiStatement(s?)
    {
        my $node = Mace::Compiler::ParseTreeObject::ParsedSwitchCase->new();

        $node->parsed_switch_constant($item{ParsedSwitchConstant});
        $node->not_null(scalar(@{$item[-1]}));
        if (scalar(@{$item[-1]})) {
            $node->semi_statements(@{$item[-1]});
        }

        $return = $node;
    }

ParsedSwitchCases : ...'case' <commit> ParsedSwitchCase ParsedSwitchCases
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedSwitchCases->new(type=>"case", parsed_switch_case=>$item{ParsedSwitchCase}, parsed_switch_cases=>$item{ParsedSwitchCases});
    }
| <error?><reject>
|
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedSwitchCases->new(type=>"null");
    }

ParsedSwitch : 'switch' '(' Expression ')' '{' ParsedSwitchCases ParsedDefaultCase '}' (';')(?)
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedSwitch->new(expr=>$item{Expression}, parsed_switch_cases=>$item{ParsedSwitchCases}, parsed_default_case=>$item{ParsedDefaultCase});
    }

ParsedMacro : '#' <commit> /[^\n]+/
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedMacro->new(item=>$item[3]);
    }
| <error?> <error>

ParsedControlFlow : 'break' | 'continue'

ParsedCaseOrDefault : 'case' ParsedSwitchConstant ':'
    {
        my $node = Mace::Compiler::ParseTreeObject::ParsedCaseOrDefault->new();
        $node->type("case");
        $node->parsed_switch_constant($item{ParsedSwitchConstant});

        $return = $node;
    }
| 'default' ':'
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedCaseOrDefault->new(type=>"default");
    }

ParsedExpectStatement : 'EXPECT' '(' Expression ')' '{' StatementBlock '}'
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedExpectStatement->new(type=>"stmt_block", expr=>$item{Expression}, stmt_block=>$item{StatementBlock});
    }
| 'EXPECT' '(' Expression ')' SemiStatement <error: You need a semi-colon after an EXPECT condition, or an opening brace to start a success block.>
| 'EXPECT' <commit> '(' Expression ')' ';'
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedExpectStatement->new(type=>"expr", expr=>$item{Expression});
    }
| <error>

ParsedCatch : 'catch' '(' ParsedVar <commit> ')' '{' StatementBlock '}'
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedCatch->new(type=>"parsed_var", parsed_var=>$item{ParsedVar}, stmt_block=>$item{StatementBlock});
    }
| 'catch' <commit> '(' '...' ')' '{' StatementBlock '}'
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedCatch->new(type=>"...", stmt_block=>$item{StatementBlock});
    }
| <error?> <error>

ParsedCatches : .../catch\b/ <commit> ParsedCatch ParsedCatches
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedCatches->new(type=>"catch", parsed_catch=>$item{ParsedCatch}, parsed_catches=>$item{ParsedCatches});
    }
| <error?> <reject>
|
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedCatches->new(type=>"null");
    }

ParsedTryCatch : 'try' <commit> '{' StatementBlock '}' .../catch\b/ ParsedCatches
    {
        $return = Mace::Compiler::ParseTreeObject::ParsedTryCatch->new(stmt_block=>$item{StatementBlock}, parsed_catches=>$item{ParsedCatches});
    }
| <error>

StatementOrBraceBlock : '{' <commit> StatementBlock '}'
    {
        $return = Mace::Compiler::ParseTreeObject::StatementOrBraceBlock->new(type=>"statement_block", stmt_block=>$item{StatementBlock});
    }
| SemiStatement
    {
        $return = Mace::Compiler::ParseTreeObject::StatementOrBraceBlock->new(type=>"semi_statement", semi_stmt=>$item{SemiStatement});
    }
| <error?> <error>

StatementBlock : SemiStatement(s?) .../\}/
    {
        my $node = Mace::Compiler::ParseTreeObject::StatementBlock->new();
        $node->not_null(scalar(@{$item[1]}));

        if (scalar(@{$item[1]})) {
            $node->semi_statements(@{$item[1]});
        }

        $return = $node;
    }

SemiStatement : Enum ';'
    {
        $return = Mace::Compiler::ParseTreeObject::SemiStatement->new(type=>"enum", enum=>$item{Enum});
        #print "SemiStatement[Enum]: ".$return->toString()."\n";
    }
| .../return\b/ <commit> ParsedReturn
    {
        $return = Mace::Compiler::ParseTreeObject::SemiStatement->new(type=>"parsed_return", parsed_return=>$item{ParsedReturn});
        #print "SemiStatement[ParsedReturn]: ".$return->toString()."\n";
    }
| .../if\b/ <commit> ParsedIf
    {
        $return = Mace::Compiler::ParseTreeObject::SemiStatement->new(type=>"parsed_if", parsed_if=>$item{ParsedIf});
        #print "SemiStatement[ParsedIf]: ".$return->toString()."\n";
    }
| .../for\b/ <commit> ParsedForLoop
    {
        $return = Mace::Compiler::ParseTreeObject::SemiStatement->new(type=>"parsed_for_loop", parsed_for_loop=>$item{ParsedForLoop});
        #print "SemiStatement[ParsedForLoop]: ".$return->toString()."\n";
    }
| .../do\b/ <commit> ParsedDoWhile
    {
        $return = Mace::Compiler::ParseTreeObject::SemiStatement->new(type=>"parsed_do_while", parsed_do_while=>$item{ParsedDoWhile});
        #print "SemiStatement[ParsedDoWhile]: ".$return->toString()."\n";
    }
| .../while\b/ <commit> ParsedWhile
    {
        $return = Mace::Compiler::ParseTreeObject::SemiStatement->new(type=>"parsed_while", parsed_while=>$item{ParsedWhile});
        #print "SemiStatement[ParsedWhile]: ".$return->toString()."\n";
    }
| ...OutputStream <commit> ParsedLogging
    {
        $return = Mace::Compiler::ParseTreeObject::SemiStatement->new(type=>"parsed_logging", parsed_logging=>$item{ParsedLogging});
        #print "SemiStatement[ParsedLogging]: ".$return->toString()."\n";
    }
| .../switch\b/ <commit> ParsedSwitch
    {
        $return = Mace::Compiler::ParseTreeObject::SemiStatement->new(type=>"parsed_switch", parsed_switch=>$item{ParsedSwitch});
        #print "SemiStatement[ParsedSwitch]: ".$return->toString()."\n";
    }
| .../try\b/ <commit> ParsedTryCatch
    {
        $return = Mace::Compiler::ParseTreeObject::SemiStatement->new(type=>"parsed_try_catch", parsed_try_catch=>$item{ParsedTryCatch});
        #print "SemiStatement[ParsedTryCatch]: ".$return->toString()."\n";
    }
| .../#/ <commit> ParsedMacro
    {
        $return = Mace::Compiler::ParseTreeObject::SemiStatement->new(type=>"parsed_macro", parsed_macro=>$item{ParsedMacro});
        #print "SemiStatement[ParsedMacro]: ".$return->toString()."\n";
    }
| .../EXPECT\b/ <commit> ParsedExpectStatement
    {
        $return = Mace::Compiler::ParseTreeObject::SemiStatement->new(type=>"parsed_expect_stmt", parsed_expect_stmt=>$item{ParsedExpectStatement});
        #print "SemiStatement[ParsedExpectStatement]: ".$return->toString()."\n";
    }
| .../ASSERTMSG\b/ <commit> ParsedAssertMsg
    {
        $return = Mace::Compiler::ParseTreeObject::SemiStatement->new(type=>"parsed_assert_msg", parsed_assert_msg=>$item{ParsedAssertMsg});
        #print "SemiStatement[ParsedAssertMsg]: ".$return->toString()."\n";
    }
| .../ASSERT\b/ <commit> ParsedAssert
    {
        $return = Mace::Compiler::ParseTreeObject::SemiStatement->new(type=>"parsed_assert", parsed_assert=>$item{ParsedAssert});
        #print "SemiStatement[ParsedAssert]: ".$return->toString()."\n";
    }
| .../ABORT\b/ <commit> ParsedAbort
    {
        $return = Mace::Compiler::ParseTreeObject::SemiStatement->new(type=>"parsed_abort", parsed_abort=>$item{ParsedAbort});
        #print "SemiStatement[ParsedAbort]: ".$return->toString()."\n";
    }
| /assert\b/ <commit> <error?: Please use ASSERT rather than assert>
| /abort\b/ <commit> <error?: Please use ABORT rather than abort>
| /drand48\b/ <commit> <error?: Please use RandomUtil rather than drand48>
| /random\b/ <commit> <error?: Please use RandomUtil rather than random>
| <error?> <reject>
| ParsedFCall ';' #{ $return = $item[1]; }
    {
        $return = Mace::Compiler::ParseTreeObject::SemiStatement->new(type=>"parsed_fcall", parsed_fcall=>$item{ParsedFCall});
        #print "SemiStatement[ParsedFCall]: ".$return->toString()."\n";
    }
| ParsedBinaryAssignOp[semi=>1] #{ $return = $item[1]; }
    {
        $return = Mace::Compiler::ParseTreeObject::SemiStatement->new(type=>"parsed_binary_assign_op", parsed_binary_assign_op=>$item{ParsedBinaryAssignOp});
        #print "SemiStatement[ParsedBinaryAssignOp]: ".$return->toString()."\n";
    }
| ParsedPlusPlus ';' #{ $return = $item[1]; }
    {
        $return = Mace::Compiler::ParseTreeObject::SemiStatement->new(type=>"parsed_plus_plus", parsed_plus_plus=>$item{ParsedPlusPlus});
        #print "SemiStatement[ParsedPlusPlus]: ".$return->toString()."\n";
    }
| ParsedControlFlow ';' #{ $return = $item[1]; }
    {
        $return = Mace::Compiler::ParseTreeObject::SemiStatement->new(type=>"parsed_control_flow", parsed_control_flow=>$item{ParsedControlFlow});
        #print "SemiStatement[ParsedControlFlow]: ".$return->toString()."\n";
    }
| ParsedCaseOrDefault <commit>
    {
        $return = Mace::Compiler::ParseTreeObject::SemiStatement->new(type=>"parsed_case_or_default", parsed_case_or_default=>$item{ParsedCaseOrDefault});
        #print "SemiStatement[ParsedCaseOrDefault]: ".$return->toString()."\n";
    }
| ParsedVar[semi=>1, arrayok=>1]
    {
        $return = Mace::Compiler::ParseTreeObject::SemiStatement->new(type=>"parsed_var", parsed_var=>$item{ParsedVar});
        #print "SemiStatement[ParsedVar]: ".$return->toString()."\n";
    }
| ParsedOutput ';'
    {
        $return = Mace::Compiler::ParseTreeObject::SemiStatement->new(type=>"parsed_output", parsed_output=>$item{ParsedOutput});
        #print "SemiStatement[ParsedOutput]: ".$return->toString()."\n";
    }
| StartPos SemiStatementBegin BraceBlock(?) (';')(?) EndPos { print "WARN (line $thisline): GENERIC SEMI-STATEMENT: ".substr($Mace::Compiler::Grammar::text, $item{StartPos}, 1+$item{EndPos}-$item{StartPos}).". Default parser will be used instead.\n"; } <error: Generic Semi-Statement on $thisline.>
#| <defer: Mace::Compiler::Globals::warning('unusual', $thisparser->{local}{filemap}->[$thisline], $thisparser->{local}{linemap}->[$thisline], "Bare Brace Block Found")> BraceBlock (';')(?) #{ $return = "UNUSUAL BARE BRACEBLOCK"; }
#| <error>

SemiStatementFoo : Enum ';'
| SemiStatementBegin BraceBlockFoo(?) (';')(?)
| <defer: Mace::Compiler::Globals::warning('unusual', $thisparser->{local}{filemap}->[$thisline], $thisparser->{local}{linemap}->[$thisline], "Bare Brace Block Found")> BraceBlockFoo (';')(?)
| <error>

MethodDecl : VirtualMethod | Method

VirtualMethod : 'virtual' Method
{
    $item{Method}->isVirtual(1);
    $return = $item{Method};
}

MethodTermFoo : StartPos FileLineEnd BraceBlockFoo EndPos
{
    my $startline = "";
    my $endline = "";
    #if(defined($Mace::Compiler::Globals::filename) and $Mace::Compiler::Globals::filename ne '') {
      $startline = "\n#line ".$item{FileLineEnd}->[0]." \"".$item{FileLineEnd}->[1]."\"\n";
      $endline = "\n// __INSERT_LINE_HERE__\n";
    #}

#    for my $statement (@{$item{BraceBlockFoo}}) {
#        print "PARSED STATEMENT: $statement\n";
#    }

    $return = $startline.substr($Mace::Compiler::Grammar::text, $item{StartPos},
                     1 + $item{EndPos} - $item{StartPos}).$endline;
}
| '=' '0' ';' { $return = "0"; }
| ';'  { $return = ""; }
| <reject:!$arg{forceColon}> ':'  { $return = ""; }



MethodTerm : StartPos FileLineEnd BraceBlock EndPos
{
    my $startline = "";
    my $endline = "";
    #if(defined($Mace::Compiler::Globals::filename) and $Mace::Compiler::Globals::filename ne '') {
      $startline = "\n#line ".$item{FileLineEnd}->[0]." \"".$item{FileLineEnd}->[1]."\"\n";
      $endline = "\n// __INSERT_LINE_HERE__\n";
    #}

#    if(defined($arg{methodName}))
#    {
#        print "| ".$arg{methodName}." {";
#        print $item{BraceBlock}->toString()."\n";
#        print "| }\n";
#        print "|\n";
#    } else {
#        print "| Undefined {\n";
#        print $item{BraceBlock}->toString()."\n";
#        print "| }\n";
#        print "|\n";
#    }

#    $return = $startline.substr($Mace::Compiler::Grammar::text, $item{StartPos},
#                     1 + $item{EndPos} - $item{StartPos}).$endline;

#    $return = $item{BraceBlock}->toString();
    $return = Mace::Compiler::ParseTreeObject::MethodTerm->new(type=>"block", block=>$item{BraceBlock});

}
| '=' '0' ';'
    {
        $return = Mace::Compiler::ParseTreeObject::MethodTerm->new(type=>"zero");
    }
| ';'
    {
        $return = Mace::Compiler::ParseTreeObject::MethodTerm->new(type=>"null");
    }
| <reject:!$arg{forceColon}> ':'
    {
        $return = Mace::Compiler::ParseTreeObject::MethodTerm->new(type=>"null");
    }

Expression : Expression1
#Expression : StartPos Expression1 EndPos
    {
        $return = Mace::Compiler::ParseTreeObject::Expression->new(expr1=>$item{Expression1});
#    $return = substr($Mace::Compiler::Grammar::text, $item{StartPos},
#                     1 + $item{EndPos} - $item{StartPos});
    }

AssignBinaryOp1 : '/=' | '*=' | '+=' | '-=' | '<<=' | '>>=' | '|=' | '&=' | '^=' |'=' ...!'=' | '%=' | <error>

AssignBinaryOp : StartPos AssignBinaryOp1 EndPos
{
    $return = substr($Mace::Compiler::Grammar::text, $item{StartPos},
             1 + $item{EndPos} - $item{StartPos});
}

PrePostAssignOp : '++' | '--' | <error>

PrePostAssignOp1 : StartPos PrePostAssignOp1 EndPos
{
    $return = substr($Mace::Compiler::Grammar::text, $item{StartPos},
             1 + $item{EndPos} - $item{StartPos});
}

BinaryOp1 : '*' | '/' | '+' ...!/[+=]/ | '<<' ...!'=' | '>>' ...!'=' | '!=' | '==' | '<=' | '>=' | '<' | '>' | '||' | '|' | '&&' | '&' | '^' | '.' | '->' | '-' ...!/[-=]/ | '%' ...!'=' | <error>

BinaryOp : StartPos BinaryOp1 EndPos
{
    $return = substr($Mace::Compiler::Grammar::text, $item{StartPos},
             1 + $item{EndPos} - $item{StartPos});
}

UnaryOp1 : '!' | '~' | '*' | '&' | /new\b/ | /delete\b/ | 'delete' '[' ']' | <error>

UnaryOp : StartPos UnaryOp1 EndPos
{
    $return = substr($Mace::Compiler::Grammar::text, $item{StartPos},
             1 + $item{EndPos} - $item{StartPos});
}


# Assume - operators have usual conventions on r/w (+, -, ++, +=, ...)

ExpressionOrAssignLValue : StartPos ExpressionOrAssignLValue1 EndPos
    {
        $return = Mace::Compiler::ParseTreeObject::ExpressionOrAssignLValue->new(expr_or_assign_lvalue1=>$item{ExpressionOrAssignLValue1});
    }


ExpressionOrAssignLValue1 : ExpressionLValue1 PrePostAssignOp
    {
        $return = Mace::Compiler::ParseTreeObject::ExpressionOrAssignLValue1->new(type=>"post_op", expr_lvalue1=>$item{ExpressionLValue1}, prepost_assign_op=>$item{PrePostAssignOp});
    }
| PrePostAssignOp ExpressionLValue1
    {
        $return = Mace::Compiler::ParseTreeObject::ExpressionOrAssignLValue1->new(type=>"pre_op", prepost_assign_op=>$item{PrePostAssignOp}, expr_lvalue1=>$item{ExpressionLValue1});
    }
| ExpressionLValue1 AssignBinaryOp Expression1
    {
        $return = Mace::Compiler::ParseTreeObject::ExpressionOrAssignLValue1->new(type=>"assign_op", expr_lvalue1=>$item{ExpressionLValue1}, assign_binary_op=>$item{AssignBinaryOp}, expr1=>$item{Expression1});
    }
| ExpressionLValue1
    {
        $return = Mace::Compiler::ParseTreeObject::ExpressionOrAssignLValue1->new(type=>"expr_lvalue1", expr_lvalue1=>$item{ExpressionLValue1});
    }
| <error>

ExpressionLValue :
StartPos ExpressionLValue1 EndPos <commit> <reject: $arg{parseFunctionCall} and not ($item{ExpressionLValue1}->getRef() eq "FUNCTION_CALL")>
    {
        #print "ExpressionLValue1->getRef() : ".$item{ExpressionLValue1}->getRef()."\n";
        $return = Mace::Compiler::ParseTreeObject::ExpressionLValue->new(expr_lvalue1=>$item{ExpressionLValue1});
    }
| <error?: Parsed Expression LValue, but Required Function Call> <error>

ExpressionLValue1 : ExpressionLValue2 '.' <commit> ExpressionLValue1
    {
        #print "ExpressionLValue1: ".$item{ExpressionLValue2}->toString()." . ".$item[-1]->toString()."\n";
        $return = Mace::Compiler::ParseTreeObject::ExpressionLValue1->new(type=>"dot", expr_lvalue2=>$item{ExpressionLValue2}, expr_lvalue1=>$item[-1]);
    }
| '*' <commit> ExpressionLValue1
    {
        $return = Mace::Compiler::ParseTreeObject::ExpressionLValue1->new(type=>"star", expr_lvalue1=>$item[-1]);
    }
| ExpressionLValue2 '->' <commit> ExpressionLValue1
    {
        $return = Mace::Compiler::ParseTreeObject::ExpressionLValue1->new(type=>"arrow", expr_lvalue2=>$item{ExpressionLValue2}, expr_lvalue1=>$item[-1]);
    }
| ExpressionLValue2 '?' <commit> ExpressionLValue1 ':' ExpressionLValue1
    {
        $return = Mace::Compiler::ParseTreeObject::ExpressionLValue1->new(type=>"question", expr_lvalue2=>$item{ExpressionLValue2}, expr_lvalue1a=>$item[-3], expr_lvalue1b=>$item[-1] );
    }
| ExpressionLValue2
    {
        $return = Mace::Compiler::ParseTreeObject::ExpressionLValue1->new(type=>"expr_lvalue2", expr_lvalue2=>$item{ExpressionLValue2});
    }
| <error>

ArrayIndex : '[' <commit> Expression1 ']'
    {
        $return = Mace::Compiler::ParseTreeObject::ArrayIndex->new(expr1=>$item{Expression1});
    }
| <error>

ExpressionLValue2: <reject>
| ScopedId ...'[' ArrayIndex(s)
    {
        my $node = Mace::Compiler::ParseTreeObject::ExpressionLValue2->new(type=>"array", scoped_id=>$item{ScopedId});
        $node->array_index(@{$item[3]});
#        print "ExpressionLValue2[ARRAY]: ".$node->toString()."\n";
        $return = $node;
    }
| ScopedId '(' Expression1(s? /,/) ')'# { $return = "FUNCTION_CALL"; }
    {
        my $node = Mace::Compiler::ParseTreeObject::ExpressionLValue2->new(type=>"fcall", scoped_id=>$item{ScopedId});

        $node->not_null_expr1(scalar(@{$item[3]}));

        if (scalar(@{$item[3]})) {
            $node->expr1(@{$item[3]});
        }
#        print "ExpressionLValue2[F-CALL]: ".$node->toString()."\n";
        $return = $node;
    }
| ScopedId '(' <commit> ExpressionOrAssignLValue1(s /,/) ')'# { $return = "FUNCTION_CALL"; }
    {
        my $node = Mace::Compiler::ParseTreeObject::ExpressionLValue2->new(type=>"fcall_assign", scoped_id=>$item{ScopedId});
        $node->expr_or_assign_lvalue1(@{$item[-2]});
#        print "ExpressionLValue2[F-CALL]: ".$node->toString()."\n";
        $return = $node;
    }
| ScopedId
    {
        $return = Mace::Compiler::ParseTreeObject::ExpressionLValue2->new(type=>"scoped_id", scoped_id=>$item{ScopedId});
#        print "ExpressionLValue2[ScopedId]: ".$return->toString()."\n";
    }
| <error>


#Changed to Expression1 to allow unary operators before things like my.foo
Expression1 : UnaryOp <commit> Expression1
    {
        $return = Mace::Compiler::ParseTreeObject::Expression1->new(type=>"unary_op", unary_op=>$item{UnaryOp}, expr1=>$item{Expression1} );
#        print "Expression1[unary_op] : ".$return->toString()."\n";
    }
| Expression2 BinaryOp <commit> Expression1
    {
        $return = Mace::Compiler::ParseTreeObject::Expression1->new(type=>"binary_op", expr2=>$item{Expression2}, binary_op=>$item{BinaryOp}, expr1=>$item{Expression1} );
#        print "Expression1[binary_op] : ".$return->toString()."  op : ".$item{BinaryOp}."\n";
    }
| Expression2 '?' <commit> Expression1 ':' Expression1
    {
        $return = Mace::Compiler::ParseTreeObject::Expression1->new(type=>"question", expr2=>$item{Expression2}, expr1=>$item[-3], expr1a=>$item[-1] );
#        print "Expression1[question] : ".$return->toString()."\n";
    }
| Expression2
    {
        $return = Mace::Compiler::ParseTreeObject::Expression1->new(type=>"expr2", expr2=>$item{Expression2} );
#        print "Expression1[expr2] : ".$return->toString()."\n";
    }
| <error>

QuotedString : <skip:'\s*'>
               /"           #Opening Quote
                [^"\\\\]*     #Any number of characters not a quote or slash
                (           #Group: 1
                  \\\\        #Followed by a slash
                  .         #Any character
                  [^"\\\\]*   #Any number of characters not a quote or slash
                )*          #1: Repeated any number of times
                "           #Closing quote
               /sx

Number : /0x[a-fA-F0-9]+(LL)?/ | /-?\d+LL/ | /-?\d+(\.\d+)?/ | <error>

Character : /'\\?.'/ | <error>

ParenOrBrace : '(' | '[' | <error>

ArrayIndOrFunction : '(' Expression1(s? /,/) ')'
    {
        my $node = Mace::Compiler::ParseTreeObject::ArrayIndOrFunction->new(type=>"func");

        $node->not_null_expr1_list(scalar(@{$item[2]}));

        if (scalar(@{$item[2]})) {
            $node->expr1_list(@{$item[2]});
        }
#        print "ArrayIndOrFunction[FUNC] : ".$node->toString()."\n";

        $return = $node;
    }
| '[' Expression1 ']'
    {
        $return = Mace::Compiler::ParseTreeObject::ArrayIndOrFunction->new(type=>"array", expr1=>$item{Expression1});
#        print "ArrayIndOrFunction[ARRAY] : ".$return->toString()."\n";
    }
| <error>

ArrayIndOrFunctionParts : ...ParenOrBrace <commit> ArrayIndOrFunction ArrayIndOrFunctionParts
    {
        $return = Mace::Compiler::ParseTreeObject::ArrayIndOrFunctionParts->new(not_null=>1, array_ind_or_function=>$item{ArrayIndOrFunction}, array_ind_or_function_parts=>$item{ArrayIndOrFunctionParts});
#        print "ArrayIndOrFunctionParts[ARRAY-FUNC] : ".$return->toString()."\n";
    }
| <error?> <reject>
|
    {
        $return = Mace::Compiler::ParseTreeObject::ArrayIndOrFunctionParts->new(not_null=>0);
#        print "ArrayIndOrFunctionParts[NULL]\n";
    }

Expression2 : Number
    {
        $return = Mace::Compiler::ParseTreeObject::Expression2->new(type=>"number", number=>$item{Number} );
    }
| ScopedId ...ParenOrBrace <commit> ArrayIndOrFunctionParts
    {
        $return = Mace::Compiler::ParseTreeObject::Expression2->new(type=>"array_func", scoped_id=>$item{ScopedId}, array_ind_or_function_parts=>$item{ArrayIndOrFunctionParts} );
#        print "Expression2[ARRAY_OR_FUNC] : ".$return->toString()."\n";
    }
| ..."'" <commit> "'" /[^\']*/ "'"
    {
        $return = Mace::Compiler::ParseTreeObject::Expression2->new(type=>"string", string=>$item[-2] );
    }
| ...'"' <commit> QuotedString
    {
        $return = Mace::Compiler::ParseTreeObject::Expression2->new(type=>"quoted_string", quoted_string=>$item[-1] );
    }
| '(' Type ')' Expression1
    {
        $return = Mace::Compiler::ParseTreeObject::Expression2->new(type=>"typecast", typecast=>$item{Type}, expr1=>$item{Expression1} );
    }
| '(' <commit> Expression1 ')'
    {
        $return = Mace::Compiler::ParseTreeObject::Expression2->new(type=>"expr1", expr1=>$item{Expression1} );
    }
| ScopedId
    {
        $return = Mace::Compiler::ParseTreeObject::Expression2->new(type=>"scoped_id", scoped_id=>$item{ScopedId} );
    }
| <error>

TypeOptions : <reject: !$arg{typeopt}> '__attribute((' <commit> TypeOption(s /,/) '))'
{
  $return = $item[4];
} | { $return = []; }
TypeOption : <rulevar: %opt>
TypeOption : FileLine Id '(' TypeOptionParamList['options'=>\%opt] ')'
{
  $return = Mace::Compiler::TypeOption->new(name => $item{Id}, line => $item{FileLine}->[0], file => $item{FileLine}->[1]);
  $return->options(%opt);
} | <error>
TypeOptionParamList :
(
  Id '=' <commit> Expression
  { $arg{options}->{$item{Id}} = $item{Expression}->toString() }
#  { $arg{options}->{$item{Id}} = $item{Expression} }
| Expression
  { $arg{options}->{$item{Expression}->toString()} = $item{Expression}->toString() }
#  { $arg{options}->{$item{Expression}} = $item{Expression} }
)(s? /;/) (';')(?) ...')' | <error>

ArraySizes : <reject: !$arg{arrayok}> ArraySize(s) | { $return = []; }
ArraySize : '[' Expression ']' { $return = $item{Expression}->toString(); } | <error>
#ArraySize : '[' Expression ']' { $return = $item{Expression}; } | <error>

CheckSemi : <reject: !$arg{semi}> ';' <commit> | <reject: $arg{semi}> | <error>

Parameter : ...Type ParameterType[%arg]
{
#    print "in Parameter:" . $item{ParameterType}->{type} .":" . $item{ParameterType}->{name} . "\n";
    $return = $item{ParameterType};
}
| <reject:!defined($arg{typeOptional})> ParameterId[%arg]
| <error>

# FIXME : should be able to process "int x=3, y=2;"
ParameterType : <reject: $arg{declareonly}> Type FileLineEnd Id ArraySizes[%arg] TypeOptions[%arg] '=' Expression CheckSemi[%arg]
{
#    print "ParameterType[AssignExp] : ".$item{Type}->type()." ".$item{Id}." := ".$item{Expression}->toString()."\n";
    #use Mace::Compiler::Context;
    my $p = Mace::Compiler::Param->new(name => $item{Id},
                                       type => $item{Type},
                                       hasDefault => 1,
                                       filename => $item{FileLineEnd}->[1],
                                       line => $item{FileLineEnd}->[0],
                                       default => $item{Expression}->toString());
#                                       default => $item{Expression});
    $p->typeOptions(@{$item{TypeOptions}});
    $p->arraySizes(@{$item{ArraySizes}});
    $return = $p;
}
| <reject: $arg{declareonly}> Type FileLineEnd Id ArraySizes[%arg] TypeOptions[%arg] '=' <commit> ExpressionOrAssignLValue CheckSemi[%arg]
{
#    print "ParameterType[AssignExprOrAssign] : ".$item{Type}->type()." ".$item{Id}." := ".$item{ExpressionOrAssignLValue}->toString()."\n";
    my $p = Mace::Compiler::Param->new(name => $item{Id},
                                       type => $item{Type},
                                       hasDefault => 1,
                                       filename => $item{FileLineEnd}->[1],
                                       line => $item{FileLineEnd}->[0],
                                       default => $item{ExpressionOrAssignLValue}->toString());
#                                       default => $item{ExpressionOrAssignLValue});
    $p->typeOptions(@{$item{TypeOptions}});
    $p->arraySizes(@{$item{ArraySizes}});
    $return = $p;
}
| <reject: !$arg{mustinit}> <commit> <error>
| <reject: !defined($arg{initializerOk})> Type FileLineEnd Id ArraySizes[%arg] '(' Expression(s? /,/) ')' CheckSemi[%arg]
{
    my $p = Mace::Compiler::Param->new(name => $item{Id},
                                       type => $item{Type},
#                                       hasDefault => 1,
#                                       hasExpression => 1,
                                       filename => $item{FileLineEnd}->[1],
                                       line => $item{FileLineEnd}->[0],
                                       expression => "(".join(", ",map { $_->toString() } @{$item[-3]}).")",
                                       default => $item{Type}->type()."(".join(", ",map { $_->toString() } @{$item[-3]}).")");
#                                       default => $item{Type}->type()."(".join(", ", @{$item[-3]}).")");
    if( scalar(@{$item[-5]}) ) {
        $p->hasDefault(1);
#        print "ParameterType[ArrayExprDefault] : ".$item{Type}->type()." ".$item{Id}."(".join(", ",map { $_->toString() } @{$item[-3]}).")"."\n";
    } else {
        $p->hasExpression(1);
#        print "ParameterType[ArrayExprExpr] : ".$item{Type}->type()." ".$item{Id}."(".join(", ",map { $_->toString() } @{$item[-3]}).")"."\n";
    }
    $p->arraySizes(@{$item{ArraySizes}});
    $return = $p;
}
| <reject: !defined($arg{initializerOk})> Type FileLineEnd Id ArraySizes[%arg] '(' <commit> ExpressionOrAssignLValue(s? /,/) ')' CheckSemi[%arg]
{
    my $p = Mace::Compiler::Param->new(name => $item{Id},
                                       type => $item{Type},
#                                       hasDefault => 1,
#                                       hasExpression => 1,
                                       filename => $item{FileLineEnd}->[1],
                                       line => $item{FileLineEnd}->[0],
                                       expression => "(".join(", ",map { $_->toString() } @{$item[-3]}).")",
                                       default => $item{Type}->type()."(".join(", ", map { $_->toString() } @{$item[-3]}).")");
#                                       default => $item{Type}->type()."(".join(", ", @{$item[-3]}).")");
    if( scalar(@{$item[-6]}) ) {
#        print "ParameterType[ArrayExprLValueDefault] : ".$item{Type}->type()." ".$item{Id}."(".join(", ", map { $_->toString() } @{$item[-3]}).")"."\n";
        $p->hasDefault(1);
    } else {
#        print "ParameterType[ArrayExprLValueExpr] : ".$item{Type}->type()." ".$item{Id}."(".join(", ", map { $_->toString() } @{$item[-3]}).")"."\n";
        $p->hasExpression(1);
    }
    $p->arraySizes(@{$item{ArraySizes}});
    $return = $p;
}
| Type FileLineEnd Id <commit> ArraySizes[%arg] TypeOptions[%arg] CheckSemi
{
    #print "Param1 type ".$item{Type}->toString()."\n";
#    print "ParameterType[Var] : ".$item{Type}->type()." ".$item{Id}."\n";
    my $p = Mace::Compiler::Param->new(name => $item{Id},
                                       type => $item{Type},
                                       filename => $item{FileLineEnd}->[1],
                                       line => $item{FileLineEnd}->[0],
                                       hasDefault => 0);
    $p->typeOptions(@{$item{TypeOptions}});
    $p->arraySizes(@{$item{ArraySizes}});

    $return = $p;
}
| <reject: !$arg{declareonly}> <commit> <error>
| <reject:!defined($arg{mapOk})> Type FileLineEnd DirArrow[direction => $arg{usesOrImplements}] Type
{
    #print "Param2 type ".$item{Type}->toString()."\n";
#    print "ParameterType[Noname] : ".$item[5]->type()."\n";
    my $p = Mace::Compiler::Param->new(name => "noname_".$thisrule->{'local'}{'paramnum'}++,
                                       type => $item[5],
                                       typeSerial => $item[2],
                                       filename => $item{FileLineEnd}->[1],
                                       line => $item{FileLineEnd}->[0],
                                       hasDefault => 0);

    $return = $p;
}
| Type FileLineEnd ('=' Expression)(?) <reject:!defined($arg{noIdOk})>
{
    #print "Param2 type ".$item{Type}->toString()."\n";
#    print "ParameterType[NonameExpr] : ".$item{Type}->type()."\n";
    my $p = Mace::Compiler::Param->new(name => "noname_".$thisrule->{'local'}{'paramnum'}++,
                                       type => $item{Type},
                                       filename => $item{FileLineEnd}->[1],
                                       line => $item{FileLineEnd}->[0],
                                       hasDefault => scalar(@{$item[3]}));

    if ($p->hasDefault()) {
        $p->default(${$item[3]}[0]);
    }
    $return = $p;
}
| StartPos SemiStatementBegin EndPos {
    #print "Note (line $thisline): NOT PARAMETER-TYPE: ".substr($Mace::Compiler::Grammar::text, $item{StartPos}, 1+$item{EndPos}-$item{StartPos})."\n";
    } <reject>
| <error?> <error>

ParameterId : Id FileLineEnd <reject:!defined($arg{typeOptional})>
{
#    print "ParameterId : ".$item{Id}."\n";
    #print "Param2 type ".$item{Type}->toString()."\n";
    my $p = Mace::Compiler::Param->new(name => $item{Id},
                                       #type => $item{Type},
                                       filename => $item{FileLineEnd}->[1],
                                       line => $item{FileLineEnd}->[0],
                                       hasDefault => 0);

    $return = $p;
}
| <error>

ATTypeDef : /typedef\s/ FileLine Type Id ';'
{
    $return = Mace::Compiler::TypeDef->new(name=>$item{Id}, type=>$item{Type}, line => $item{FileLine}->[0], filename => $item{FileLine}->[1]);
}
        | <error>

AutoType : Id FileLine TypeOptions[typeopt => 1] '{' ATTypeDef(s?) Parameter[typeopt => 1, semi => 1](s?) Constructor[className => $item{Id}](s?) Method[staticOk=>1](s?) '}' (';')(?)
{
  my $at = Mace::Compiler::AutoType->new(name => $item{Id},
                                         line => $item{FileLine}->[0],
                                         filename => $item{FileLine}->[1],
                                         );
  $at->typeOptions(@{$item{TypeOptions}});
  $at->typedefs(@{$item[5]});
  $at->fields(@{$item[6]});
  $at->constructors(@{$item[7]});
  $at->methods(@{$item[8]});
  for my $m (@{$item[7]}) {
    if($m->name ne $item{Id}) {
      Mace::Compiler::Globals::error("bad_auto_type",  $item{FileLine}->[1], $item{FileLine}->[0], "Constructor name does not match auto_type name");
    }
  }
  my $key = "service";
  if (defined($arg{key})) {
      $key = $arg{key};
  }
  $thisparser->{'local'}{$key}->push_auto_types($at);
}
| <error>

#XXX: reject
DirArrow : '<-' <commit> <reject: $arg{direction} eq "uses"> | '->' <commit> <reject: $arg{direction} eq "implements">

KeyEqVal : Id '=' Id { $return = [ $item[1], $item[-1] ] }

MethodOptions : '[' KeyEqVal(s /;/) (';')(?) ']' { $return = $item[2];}

InitializerItem : ScopedId '(' Expression(s? /,/) ')'
InitializerList : StartPos FileLineEnd ':' InitializerItem(s /,/) EndPos
{
    my $startline = "";
    my $endline = "";
    #if(defined($Mace::Compiler::Globals::filename) and $Mace::Compiler::Globals::filename ne '') {
      $startline = "\n#line ".$item{FileLineEnd}->[0]." \"".$item{FileLineEnd}->[1]."\"\n";
      $endline = "\n// __INSERT_LINE_HERE__\n";
    #}
    $return = $startline.substr($Mace::Compiler::Grammar::text, $item{StartPos},
                     1 + $item{EndPos} - $item{StartPos}).$endline;
}
| {$return = ""}

Constructor : <reject:defined($arg{className})> <commit> /\b$thisparser->{'local'}{'classname'}\b/ '(' Parameter(s? /,/) ')' InitializerList MethodTerm
{
    my $t = Mace::Compiler::Type->new(type => "");
    my $m = Mace::Compiler::Method->new(name => $thisparser->{'local'}{'classname'},
                                        returnType => $t,
                                        isUsedVariablesParsed => 1,
                                        body => $item{InitializerList}.$item{MethodTerm}->toString());

    if (scalar(@{$item[5]})) {
        $m->params(@{$item[5]});
    }

    $return = $m;
}
| <reject:!defined($arg{className})> <commit> /\b$arg{className}\b/ '(' Parameter(s? /,/) ')' InitializerList MethodTerm
{
    my $t = Mace::Compiler::Type->new(type => "");
    my $m = Mace::Compiler::Method->new(name => $arg{className},
                                        returnType => $t,
                                        isUsedVariablesParsed => 1,
                                        body => $item{InitializerList}.$item{MethodTerm}->toString());

    if (scalar(@{$item[5]})) {
        $m->params(@{$item[5]});
    }

    $return = $m;
}
| <error>


Destructor : ('virtual')(?) '~' /\b$thisparser->{'local'}{'classname'}\b/ '(' ')' MethodTerm
{
    my $t = Mace::Compiler::Type->new(type => "");
    my $m = Mace::Compiler::Method->new(name => '~' . $thisparser->{'local'}{'classname'},
                                        isUsedVariablesParsed => 1,
                                        returnType => $t,
                                        body => $item{MethodTerm}->toString());
    if (defined($item[1])) {
        $m->isVirtual(1);
    }
    $return = $m;
}

ThrowType : Type { $return = $item{Type}->toString() } | "..."
Throws : 'throw' '(' ThrowType ')'
{
  $return = 'throw('.$item{ThrowType}.')';
}

#MethodReturnType : <reject:$arg{noReturn}> Type | <reject:!$arg{noReturn}> { $return = Mace::Compiler::Type->new(); } | <error>
MethodReturnType : Type ...!'(' { $return = $item{Type} } | <reject:!$arg{noReturn}> { $return = Mace::Compiler::Type->new(); } | <error>

MethodOperator : '==' | '<=' | '>=' | '<' | '>' | '=' | '!=' | '+' | '*' | '/' | '->' | '-'
MethodName : /operator\b/ <commit> MethodOperator { $return = "operator".$item{MethodOperator}; } | Id | <error>
StaticToken : /static\b/

#Method : StaticToken(?) <reject:!defined($arg{context}) or (defined($arg{context}) and !$arg{context}) or (!$arg{staticOk} and scalar(@{$item[1]}))> MethodReturnType[%arg] MethodName FileLineEnd '(' Parameter[%arg](s? /,/) ')' ConstToken(?) Throws(?) MethodOptions(?) MethodTerm[forceColon => $arg{forceColon}, methodName => $item{MethodName}]
Method : StaticToken(?) <reject:!( $Mace::Compiler::Globals::useSnapshot and $Mace::Compiler::Globals::useParseVariables) or (!$arg{staticOk} and scalar(@{$item[1]}))> MethodReturnType[%arg] MethodName FileLineEnd '(' Parameter[%arg](s? /,/) ')' ConstToken(?) Throws(?) MethodOptions(?) MethodTerm[forceColon => $arg{forceColon}, methodName => $item{MethodName}]
{
    # context = 1

    #print STDERR "useSnapshot : ".$Mace::Compiler::Globals::useSnapshot."\n";

#    if( defined($item{MethodName}) ) {
#      print STDERR "Method ".$item{MethodName}." uses INCONTEXT parser.\n";
#    } else {
#      print STDERR "Method [unnamed] uses INCONTEXT parser.\n";
#    }

    # print "DEBUG:  ".$item{FileLine}->[2]."\n";
    # print "DEBUG1: ".$item{FileLine}->[0]."\n";
    # print "DEBUG2: ".$item{FileLine}->[1]."\n";
#    my $mt = $item{MethodTerm};

    my $m = Mace::Compiler::Method->new(name => $item{MethodName},
                                        returnType => $item{MethodReturnType},
                                        isConst => scalar(@{$item[-4]}),
                                        isStatic => scalar(@{$item[1]}),
                                        isUsedVariablesParsed => 1,
                                        line => $item{FileLineEnd}->[0],
                                        filename => $item{FileLineEnd}->[1],
                                        body => $item{MethodTerm}->toString(),
                                        );
    if( defined $arg{context} ){
        $m->targetContextObject( $arg{context} );
    }
    if( defined $arg{snapshot} ){
        $m->snapshotContextObjects( $arg{snapshot} );
    }

    $m->usedStateVariables(@{$item{MethodTerm}->usedVar()});

    if (scalar($item[-3])) {
        $m->throw(@{$item[-3]}[0]);
    }

#    print STDERR "MethodName : ".$item{MethodName}."\n";

    if (scalar(@{$item[7]})) {
        $m->params(@{$item[7]});
#        for my $el (@{$item[7]})
#        {
#           print STDERR "   Param: ".$el->name()."\n";
#        }
    }

    if (scalar(@{$item[-2]})) {
        my $ref = ${$item[-2]}[0];
        for my $el (@$ref) {
            $m->options(@$el);
#        print STDERR "MethodOptions DEBUG:  ".$el->[0]."=".$el->[1]."\n";
        }
    }

    $return = $m;
}
#| StaticToken(?) <reject:( defined($arg{context}) and $arg{context} ) or (!$arg{staticOk} and scalar(@{$item[1]}))> MethodReturnType[%arg] MethodName FileLineEnd '(' Parameter[%arg](s? /,/) ')' ConstToken(?) Throws(?) MethodOptions(?) MethodTermFoo[forceColon => $arg{forceColon}, methodName => $item{MethodName}]
| StaticToken(?) <reject:(!$arg{staticOk} and scalar(@{$item[1]}))> MethodReturnType[%arg] MethodName FileLineEnd '(' Parameter[%arg](s? /,/) ')' ConstToken(?) Throws(?) MethodOptions(?) MethodTermFoo[forceColon => $arg{forceColon}, methodName => $item{MethodName}]
{


  # context = 0

  # print STDERR "useSnapshot : ".$Mace::Compiler::Globals::useSnapshot."\n";
#    if( defined($item{MethodName}) ) {
#      print STDERR "Method ".$item{MethodName}." uses DEFAULT parser.\n";
#    } else {
#      print STDERR "Method [unnamed] uses DEFAULT parser.\n";
#    }

    # print $item{MethodName}."\n";
    # print "DEBUG:  ".$item{FileLine}->[2]."\n";
    # print "DEBUG1: ".$item{FileLine}->[0]."\n";
    # print "DEBUG2: ".$item{FileLine}->[1]."\n";
#    my $mt = $item{MethodTerm};

    my $m = Mace::Compiler::Method->new(name => $item{MethodName},
                                        returnType => $item{MethodReturnType},
                                        isConst => scalar(@{$item[-4]}),
                                        isStatic => scalar(@{$item[1]}),
                                        isUsedVariablesParsed => 0,
                                        line => $item{FileLineEnd}->[0],
                                        filename => $item{FileLineEnd}->[1],
                                        body => $item{MethodTermFoo},
                                        );
    if( defined $arg{context} ){
        $m->targetContextObject( $arg{context} );
    }
    if( defined $arg{snapshot} ){
        $m->snapshotContextObjects( $arg{snapshot} );
    }

#    $m->usedStateVariables(@{$item{MethodTerm}->usedVar()});

    if (scalar($item[-3])) {
        $m->throw(@{$item[-3]}[0]);
    }
    if (scalar(@{$item[7]})) {
        $m->params(@{$item[7]});
    }

    if (scalar(@{$item[-2]})) {
        my $ref = ${$item[-2]}[0];
        for my $el (@$ref) {
            $m->options(@$el);
#        print STDERR "MethodOptions DEBUG:  ".$el->[0]."=".$el->[1]."\n";
        }
    }

    $return = $m;
}
| <error>

PointerType : NonPointerType ConstToken ('*')(s) | NonPointerType ('*')(s) ConstToken ('*')(s) | NonPointerType ('*')(s?) | <error>

NonPointerType : BasicType | StructType | ScopedType | <error>

#XXX-CK: Shouldn't this really be a recursion on Type, not Id?  After all, you can have std::map<> or map<>::iterator . . .
ScopedId : StartPos TemplateTypeId ('::' TemplateTypeId)(s?) EndPos
    {
        my $node = Mace::Compiler::ParseTreeObject::ScopedId->new(val=>substr($Mace::Compiler::Grammar::text, $item{StartPos}, 1 + $item{EndPos} - $item{StartPos}));
        $return = $node;
    }
#{ $return = substr($Mace::Compiler::Grammar::text, $item{StartPos}, 1 + $item{EndPos} - $item{StartPos}); }
ScopedType : TemplateType ('::' TemplateType)(s?)

#NOTE: CK -- added ScopedType to Template type to allow ::
#      CK -- Added | Expression since template parameters could be constant expressions
#      CK -- BasicType added to parse primitive types and their modifiers
TemplateTypeId : Id '<' ( ConstToken(?) (PointerType | Number) )(s /,/) '>' | Id | <error>
TemplateType : Id '<' <commit> ( ConstToken(?) (PointerType | Number) )(s /,/) '>' | Id | <error>

ConstToken : /const\b/

RefToken : '&'

#NOTE: CK -- added \b to avoid problems parsing things like int8_t.  Also -- removed 'long long'
TypeModifier : /\blong\b/ | /\bsigned\b/ | /\bunsigned\b/ | /\bshort\b/ | <error>
PrimitiveType : /\bint\b/ | /\bdouble\b/ | /\bfloat\b/ | /\bchar\b/ | <error>
BasicType : TypeModifier(0..3) PrimitiveType | TypeModifier(1..3) | <error>

StructType : 'struct' Id

Type : ConstToken(?) StartPos PointerType EndPos ConstToken(?) RefToken(?)
{
    my $type = substr($Mace::Compiler::Grammar::text, $item{StartPos},
                      1 + $item{EndPos} - $item{StartPos});

    $return = Mace::Compiler::Type->new(type => Mace::Util::trim($type),
                                         isConst1 => scalar(@{$item[1]}),
                                         isConst2 => scalar(@{$item[-2]}),
                                         isConst => (scalar(@{$item[1]}) or scalar(@{$item[-2]})),
                                         isRef => scalar(@{$item[-1]}));
}
| <error>

ProtectionToken : /public\b/ | /private\b/ | /protected\b/

EOFile : /^\Z/

}; # COMMON grammar

1;
