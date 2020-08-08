#!/usr/bin/perl -w
# 
# visualizeDot.pl : part of the Mace toolkit for building distributed systems
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
use Getopt::Long;

my $LABEL = "";
my $EDGES = "";
my $REVERSE = "0";
GetOptions("label=s" => \$LABEL,
	   "edges=s" => \$EDGES,
           "reverse=s" => \$REVERSE);


my $nodeNum = -1;
 
my $edgeString;
my $label;

my %edgeStrings;
my %labelStrings;

my $started = 0;

while (<>) {
    if (/Application State/) {
        $started = 1;
    }
    unless ($started) { 
        next;
    }
    if (/\[ Node (\d+): state=/) {
        if ($nodeNum >= 0) {
            unless ($label) {
                die("did not find label $LABEL\n");
            }
            unless ($edgeString) {
                die("did not find edgeString $EDGES\n");
            }
            $edgeStrings{$label}=$edgeString;
            $labelStrings{$label}=$nodeNum;
        }
        $nodeNum = $1; 
    }
    elsif (/^$LABEL=(.*)$/) {
        $label = $1;
    }
    elsif (/^$EDGES=(.*)$/) {
        $edgeString=$1;
    }
}

$edgeStrings{$label}=$edgeString;
$labelStrings{$label}=$nodeNum;

my @labels = keys(%edgeStrings);
print "digraph $EDGES {\n";
for my $i (@labels) {
    my $l = $edgeStrings{$i};
    my @endpoints = grep($l =~ /$_/, @labels);
    for my $j (@endpoints) {
        if ($REVERSE) {
            print $labelStrings{$j}." -> ".$labelStrings{$i}.qq{ [label="$EDGES-r"] \n};
        }
        else {
            print $labelStrings{$i}." -> ".$labelStrings{$j}.qq{ [label="$EDGES"] \n};
        }
    }
}
print "}\n";

