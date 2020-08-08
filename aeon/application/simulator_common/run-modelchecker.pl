#!/usr/bin/perl -w
# 
# run-modelchecker.pl : part of the Mace toolkit for building distributed systems
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

use FindBin;
use lib "$FindBin::Bin";
use SimulatorScriptsCommon;

use strict;

my $path = $FindBin::Bin;
my $simulator = SimulatorScriptsCommon::findSimulator();
my $mc = "$simulator @ARGV ";
# my $gengraph = "$path/generateEventGraph.pl --uncolor";
my $gengraph = "$path/generateEventGraph.pl";
my $findnail = "$path/findLastNailStep.pl";
my $parsescoped = "$path/parse-scoped-serialize.pl";

system("rm -f mc.log");

my $out = "mc.log";
my $errorlog = "error.log";
my $livelog = "live.log";
my $errorgraph = "error.graph";
my $livegraph = "live.graph";

system("rm -f core");

#my $mcRet2 = system("$mc | tee $out");
open(RUNMC, "$mc |") or die;
open(RUNMCLOG, ">> $out") or die;
while (<RUNMC>) {
    print $_;
    print RUNMCLOG $_;
}
close(RUNMCLOG);
close(RUNMC);

if ($? == -1) {
    print "failed to execute: $!\n";
}
elsif ($? & 127) {
    printf "child died with signal %d, %s coredump\n",
           ($? & 127),  ($? & 128) ? 'with' : 'without';
}
else {
    printf "Execution exited with value %d\n", $? >> 8;
}

my $l = `tail -20 $out | grep "ERROR::SIM::"`;
chomp($l);
#system(qq/echo "Execution returned $mcRet $mcRet2" >> $out/);

my $lastnail = 0;
my $lastnailStep = 0;
if ($l =~ m|SIM::MAX_STEPS| or $l =~ m|SIM::NO_MORE_STEPS| or $l =~ m|SIM::PATH_TOO_LONG|) {
    print "\nrunning last nail...\n\n";
    system("echo '*** LAST NAIL LOG ***' >> $out");
    system("$mc -LAST_NAIL_REPLAY_FILE error1.path | tee -a $out");
    $l = `tail -10 $out`;
    if ($l =~ m|was random number position (\d+)|s) {
	$lastnail = $1 - 1;

        print "\nfound last nail position $lastnail\n\n";

        if ($lastnail) {

            print "generating live path to $livelog...\n";
            system("$mc -RANDOM_REPLAY_FILE live${lastnail}.path -TRACE_ALL 1 > $livelog");

            print "generating live graph to $livegraph...\n";
            system("$gengraph -o $livegraph $livelog");

            system("echo '*** LAST NAIL STEP ***' >> $out");
            system("$findnail $lastnail $livelog | tee -a $out");

            my $sl = `tail -1 $out`;
            if($sl =~ /step (\d+)/) {
              $lastnailStep = $1;
            }

        } else {
            warn "lastnail position was 0\n";
        }
    }
    else {
	warn "could not find last nail";
    }
    $lastnail = 1;
}

if ($lastnail || ($l =~ m/ERROR::SIM/)) {
    print "\ngenerating error path to $errorlog...\n\n";
    if($lastnail) {
      my $stopstep = $lastnailStep + 2000;
#       system("$mc -RANDOM_REPLAY_FILE error1.path -max_num_steps $stopstep -TRACE_ALL 1 > $errorlog");
      system("$mc -RANDOM_REPLAY_FILE error1.path -max_num_steps $stopstep -TRACE_ALL 1 | $parsescoped > $errorlog");
    }
    else {
      system("$mc -RANDOM_REPLAY_FILE error1.path -TRACE_ALL 1 | $parsescoped > $errorlog");
#       system("$mc -RANDOM_REPLAY_FILE error1.path -TRACE_ALL 1 > $errorlog");
    }

    print "generating error graph to $errorgraph...\n";
    system("$gengraph -w 2000 -o $errorgraph $errorlog");
#    system("mplayer",  "/usr/share/sounds/kubuntu-login.ogg");
    exit(1);
}
#system("mplayer", "/usr/share/sounds/kubuntu-login.ogg");
