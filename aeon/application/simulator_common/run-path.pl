#!/usr/bin/perl -w
# 
# run-path.pl : part of the Mace toolkit for building distributed systems
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

use strict;

use FindBin;
use lib "$FindBin::Bin";
use SimulatorScriptsCommon;

my $replayFile = shift(@ARGV) || "";
unless (-e $replayFile) {
    print "usage: $0 replayFile\n";
    exit(1);
}

my $path = $FindBin::Bin;
my $mc = findSimulator();
my $gengraph = "$path/generateEventGraph.pl";
my $parsescoped = "$path/parse-scoped-serialize.pl";
my $errorlog = "error.log";
my $errorgraph = "error.graph";

print "running modelchecker with path $replayFile...\n";
system("$mc -RANDOM_REPLAY_FILE $replayFile -TRACE_ALL 1 -max_num_steps 2000 | $parsescoped > $errorlog");
print "generating error graph to $errorgraph...\n";
system("$gengraph -w 2000 -o $errorgraph $errorlog");

