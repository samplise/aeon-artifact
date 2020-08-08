#!/usr/bin/perl -w
# 
# train-outliers.pl : part of the Mace toolkit for building distributed systems
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
use lib $ENV{HOME}."/lib/perl5";

use Statistics::Descriptive;

my $stat = Statistics::Descriptive::Full->new();

#$stat->add_data(1,2,3,4); 
#$mean = $stat->mean();
#$var  = $stat->variance();
#$tm   = $stat->trimmed_mean(.25);
#$Statistics::Descriptive::Tolerance = 1e-10;

my $simulator = SimulatorScriptsCommon::findSimulator();
my $mc = "$simulator -search_print_mask 0 -MAX_PATHS 50 -PRINT_PREFIX 1 @ARGV ";

my $out = "outliers.log";

my $mcRet2 = system("$mc | tee $out");
my $mcRet = $?;
print "Execution returned $mcRet $mcRet2\n";
my $l = `tail -20 $out | grep "ERROR::SIM::"`;
chomp($l);
system(qq/echo "Execution returned $mcRet $mcRet2" >> $out/);

open(STATS, "<", $out);
while(<STATS>) {
    if (/avg: (\d+)/) {
        $stat->add_data($1);
    }
}

my $q1 = $stat->percentile(25);
my $q3 = $stat->percentile(75);
my $iqr = $q3-$q1;

my $minout = ($q1-1.5*$iqr);
my $maxout = ($q3+1.5*$iqr);

print "Outlier < $minout or > $maxout\n";
system("echo 'Outlier < $minout or > $maxout' >> $out");
print "Q1: $q1  Q3: $q3\n";
system("echo 'Q1: $q1  Q3: $q3' >> $out");

