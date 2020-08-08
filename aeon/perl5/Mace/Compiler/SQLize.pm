# 
# SQLize.pm : part of the Mace toolkit for building distributed systems
# 
# Copyright (c) 2011, Ryan Braud, Charles Killian
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
package Mace::Compiler::SQLize;

use strict;

sub generateBody {
  my $params = shift;
  my $printState = shift;
  my $testDump = shift;
  my $vars;
  my $ret = "int _index = __node->nextIndex();\n";
  
  $ret .= "if (_index == 0) {\nstd::ostringstream __out;\n__out << \"CREATE TABLE \" << __node->logName << \" (_id INT PRIMARY KEY";
  my $insertValues = "std::ostringstream __out;\n__out << __node->logId << \"\\t\" << _index";
  if ($printState == 1) {
    $insertValues .= " << \"\\t\" << getStateName(state);\n";
    $ret .= ", state TEXT";
  }
  else {
      $insertValues .= ";\n";
  }
  my $childNodes = "";
  my $sqlizeStr;
  my $cntr = 0;
  
  foreach my $var (@$params) {
    # this used to be &&'ed with $var->flags('dump'): $var->flags('serialize')
    if ($var->shouldLog() && ($testDump == 0 || ($var->flags('dump')))) {
      my $type = $var->type()->type();
      my $varName = $var->name();
      if ($type eq 'long' || $type eq 'unsigned long' || 
	  $type eq 'int64_t' || $type eq 'uint32_t' || $type eq 'unsigned int' ||
	  $type eq 'unsigned' || $type eq 'long long' || $type eq 'size_t') {
	$ret .= ", $varName INT8";
	$insertValues .= "__out << \"\\t\" << $varName;\n";
      }
      elsif ($type eq 'uint64_t' || $type eq 'unsigned long long') {
	$ret .= ", $varName NUMERIC(20, 0)";
	$insertValues .= "__out << \"\\t\" << $varName;\n";
      }
      elsif ($type eq 'float') {
	$ret .= ", $varName REAL";
	$insertValues .= "__out << \"\\t\" << $varName;\n";
      }
      elsif ($type eq 'double') {
	$ret .= ", $varName DOUBLE PRECISION";
	$insertValues .= "__out << \"\\t\" << $varName;\n";
      }
      elsif ($type eq 'string' || $type eq 'std::string' || 
	     $type eq 'mace::string') {
	$ret .= ", $varName TEXT";
	$insertValues .= "__out << \"\\t\";\n";
	$insertValues .= "if (Base64::isPrintable($varName)) {\n";
	$insertValues .= "__out << $varName;\n}\n";
	$insertValues .= "else {\n";
	$insertValues .= "__out << HashString::hash($varName);\n}\n";
      }
      else {
	$ret .= ", $varName INT";
	if ($type eq 'int' || $type eq 'short' || $type eq 'char' || 
	    $type eq 'int32_t' || $type eq 'int16_t' || $type eq 'int8_t' ||
	    $type eq 'uint16_t' || $type eq 'uint8_t' ||
	    $type eq 'unsigned short' || $type eq 'unsigned char' || 
	    $type eq 'bool') {
	  $insertValues .= "__out << \"\\t\" << $varName;\n";
	}
	else {
	  $childNodes .= "mace::LogNode* __child$cntr = new mace::LogNode(__node->logName + \"_${varName}\", Log::sqlEventsLog);\n";
	  $childNodes .= "__node->children[$cntr] = __child$cntr;\n";
	  $vars .= "int _${varName}Index = __node->children[$cntr]->nextId;\n";
	  $insertValues .= "__out << \"\\t\" << _${varName}Index;\n";
	  $sqlizeStr .= "mace::sqlize(&$varName, __node->children[$cntr]);\n";
	  $cntr++;
        }
      }
    }
  }
  $ret .= ");\" << std::endl;\n";
  if ($cntr == 0) {
      $ret .= "__node->children = NULL;";
  }
  else {
    $ret .= "__node->children = new mace::LogNode*[$cntr];";
  }
  $ret .= "\n${childNodes}Log::logSqlCreate(__out.str(), __node);\n}\n";
  $insertValues .= "__out << std::endl;\nASSERT(fwrite(__out.str().c_str(), __out.str().size(), 1, __node->file) > 0);\n";
  if (defined($vars)) {
    $ret .= $vars;
  }
  $ret .= $insertValues;
  if (defined($sqlizeStr)) {
    $ret .= $sqlizeStr;
  }
  return $ret;
}

1;
