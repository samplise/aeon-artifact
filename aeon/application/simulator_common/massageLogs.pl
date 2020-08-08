#!/usr/bin/perl -w
# 
# massageLogs.pl : part of the Mace toolkit for building distributed systems
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
use Carp;

$SIG{__DIE__} = sub{ confess $_[0]; };
$SIG{__WARN__} = sub{ confess $_[0]; die; };

if (scalar(@ARGV) < 2) {
    die("usage: $0 outfile infile [infile ... infile]\n");
}

my @filenames;
my %files;
my %hosts;
my %indices;
my $filesdone = 0;
my %host2file;
my %host2node;
my %messagequeue;
my $pktId = 0;
my %message2PktId;
my $outfilename = shift(@ARGV);

open(OUTFILE, ">$outfilename");

for my $i (@ARGV) {
    push @filenames, $i;
    open(INF, $i) or die("cannot open input file");
    $files{$i} = [ <INF> ];
    for my $elt (@{$files{$i}}) {
        $elt =~ s|IPV4/||g; #remove IPV4/ to make mdb and generateEventGraph happier
    }
    print "read ".scalar(@{$files{$i}})." lines\n";
    my @l = grep(/listening on /, @{$files{$i}});
    die("could not find hostname") unless(scalar(@l));
    my $hl = $l[0];
    if ($hl =~ /listening on (\S+) \(port\)\d+(\r?)\n/) {
        $hosts{$i} = $1;
        $host2file{$1} = $i;
        print "file $i had hostname $1\n";
    } else {
        die("line did not match regex");
    }
    close(INF);
}

print OUTFILE "[main] Starting simulation\n";

my $i = 0;
for my $f (@filenames) {
    $indices{$f} = 0;
#    print "putting ".$i." into ".$hosts{$f}."\n";
    $host2node{$hosts{$f}} = $i;
    print OUTFILE "[NodeAddress] Node $i address ".$hosts{$f}."\n";
    $i++;
}

#get preamble
for my $f (@filenames) {
    while (! (@{$files{$f}}[$indices{$f}] =~ /\[ServiceStack\] STARTING/)) {                
        print OUTFILE $hosts{$f}." ".@{$files{$f}}[$indices{$f}];
        $indices{$f}++;
    }
}

$i = 0;
while ($filesdone < scalar(@filenames)) {
    if ($i % 100 == 0) {
        print "[main] Now on simulator step $i\n";
    }
    my $minfile = "";
    my $mintime = "";
    my $minnode = -1;
    my $minthread = -1;

    my $nodenum = 0;
    for my $f (@filenames) {
        if ($indices{$f} < scalar(@{$files{$f}})) {
            if (@{$files{$f}}[$indices{$f}] =~ /^(\d+\.\d+) (\d{2})/) {
                if ($mintime eq "" or $mintime > $1) {
                   $minfile = $f;
                   $mintime = $1;
                   $minnode = $nodenum;
                   $minthread = $2;
                }
            }
            else {
                print @{$files{$f}}[$indices{$f}]."\n";
                die("STARTING line did not have timestamp!");
            }
        }
        $nodenum++;
    }

    my $f = $minfile;
    my $thread = $minthread;
#    print "file $f thread $thread mintime $mintime index ".$indices{$f}."\n";
    
    my $rcvmsg = "";
    my $msgId = "";
    my $srcMaceKey = "";

    # if we see a received message, make sure that the message has been sent (by checking the message queue)
    # if the message is not there, don't continue with this log file. Instead, we should switch to the file
    # that will send the message. This is to ensure the happens-before order relationship
    if($indices{$f} < scalar(@{$files{$f}}) &&    
                            (@{$files{$f}}[$indices{$f}] =~ /RECEIVED (TCP|UDP) MESSAGE\(src_macekey=([a-zA-Z0-9._-]+):([0-9]+), port=([0-9]+), seq=([0-9]+)/)){
        
        $srcMaceKey = "$2:$3";      # macekey of the node that send the message
        $msgId = "$2:$3:$4:$5";     # id of the message

        # if this message has not been sent
        if(!exists $messagequeue{"$msgId"}){

            # check that we have the log file from which the message was coming from 
            if(exists $host2file{$srcMaceKey}){
                my $f2 = $host2file{$srcMaceKey};

                # check that we're still have more to read from the log file
                if ($indices{$f2} < scalar(@{$files{$f2}}) && @{$files{$f2}}[$indices{$f2}] =~ /\d+\.\d+ (\d{2}) .*? RECEIVED (TCP|UDP) MESSAGE/) {
                    $thread = $1;
                    $f = $f2
                }
                else {
                    print "WARNING: Message id=$msgId was sent from host ".$srcMaceKey. ", but unable to find when the message was sent.\n";
                }
            } else {
                print "WARNING: Received a message from ".$srcMaceKey.", but could not find the log file that host. Continuing...\n"; 
            }
        } 
        # if message has been sent 
        else {
            while ($indices{$f} < scalar(@{$files{$f}}) && ! (@{$files{$f}}[$indices{$f}] =~ /STARTING[(]/)){
                print OUTFILE $hosts{$f}." ".@{$files{$f}}[$indices{$f}];
                $indices{$f}++;
            }
            my $fromnode = "UNKNOWN";
           
            if(exists $host2node{$srcMaceKey}){
                $fromnode = $host2node{$srcMaceKey};
            }
            else{ 
                print "WARNING: Received message from unknown host - ".$srcMaceKey."\n";
            }
            # this rcvmsg is needed by generateGraph script to determine where and when the message was sent from
            $rcvmsg = "id ".$message2PktId{"$msgId"}." from ". $fromnode." to ".$host2node{$hosts{$f}};
            delete $messagequeue{"$msgId"};
        }
    }
    print OUTFILE $hosts{$f}." [main] Now on simulator step $i\n";

    my $evtype = "APP_EVENT";
    my $msg = "UNKNOWN";
    if (@{$files{$f}}[$indices{$f}] =~ /::deliver/) {
        $evtype = "NET_EVENT";
    }
    elsif (@{$files{$f}}[$indices{$f}] =~ /::error/) {
        $evtype = "NET_EVENT";
    }
    elsif (@{$files{$f}}[$indices{$f}] =~ /::messageError/) {
        $evtype = "NET_EVENT";
    }
    elsif (@{$files{$f}}[$indices{$f}] =~ /::expire/) {
        $evtype = "SCHED_EVENT";
    }
    elsif (@{$files{$f}}[$indices{$f}] =~ /RECEIVED (TCP|UDP) MESSAGE/) {
        $evtype = "NET_EVENT";
    }
    if (@{$files{$f}}[$indices{$f}] =~ /STARTING[(] (\S+) [)]/) {
        $msg = $1;
    }

    my $firstline = 1;

    my $noip = 0;

    print OUTFILE $hosts{$f}." [SimulateEventBegin] $i $minnode $evtype\n";
    
    my $calldepth = 0;
    do {
        if ((@{$files{$f}}[$indices{$f}] =~ /\d+\.\d+ (\d{2}) .*? STARTING[(]/ or
             @{$files{$f}}[$indices{$f}] =~ /\d+\.\d+ (\d{2}) .*? RECEIVED (TCP|UDP) MESSAGE/)
            and $1 != $thread) {
            my $toShift = $indices{$f};
            while ($toShift < scalar(@{$files{$f}}) and @{$files{$f}}[$toShift] =~ /\d+\.\d+ (\d{2}) .*? / and $1 != $thread) {
                $toShift++;
            }
            if ($toShift < scalar(@{$files{$f}})) {
                my $line = @{$files{$f}}[$toShift];
                #print $hosts{$f}. " MOVING ".$line." "." MOVING to before ".$files{$f}[$indices{$f}]."\n";
                splice @{$files{$f}},$toShift,1;
                splice @{$files{$f}},$indices{$f},0,$line;
                #print $hosts{$f}." MOVING NOW WE'RE AT ".@{$files{$f}}[$indices{$f}];
            }
        }
        if (@{$files{$f}}[$indices{$f}] =~ /SNAPSHOT/) {
            print OUTFILE  "[__Simulator__::dumpState] Application State: [ Node ".$hosts{$f}.": ";
            $noip = 1;
        }
        print OUTFILE ($noip ? "" : $hosts{$f}." ").@{$files{$f}}[$indices{$f}];
        if (@{$files{$f}}[$indices{$f}] =~ /STARTING[(]/) {
            $calldepth++;
        }
        elsif (@{$files{$f}}[$indices{$f}] =~ /ENDING[(]/) {
            $calldepth--
        }
        if (@{$files{$f}}[$indices{$f}] =~ /\[ServiceStack\] END/) {
            $calldepth = 0;
        }

        if ($indices{$f} < scalar(@{$files{$f}}) && @{$files{$f}}[$indices{$f}] =~ /Sending (UDP|TCP) pkt from src=([a-zA-Z0-9._-]+:\d+) to dest=([a-zA-Z0-9._-]+:\d+) on port=(\d+) with seq=(\d+)/) {

            #print $hosts{$f}." ".@{$files{$f}}[$indices{$f}];
            my $pktType = $1;
            my $srchost = $2;
            my $dsthost = $3;
            my $port = $4;
            my $seq = $5;
            print OUTFILE $hosts{$f}." [MASSAGE::".$pktType."::route::pkt::(downcall)] route([dest=$dsthost][s_deserialized=pkt(id=$pktId)])\n";
            #print "Putting $srchost:$port:$seq into the messagequeue\n";
            $messagequeue{"$srchost:$port:$seq"} = 1;
            $message2PktId{"$srchost:$port:$seq"} = $pktId;
            $pktId++;
        }
        $indices{$f}++;

        if ($firstline) {
            my $svc = "";
#            print STDERR "$msg\n";
            if ($msg =~ /^([a-zA-Z1-9_]+)::/) {
                $svc = $1;
                $svc =~ tr/a-z_//d;
                $svc .= ".";
#                print "OK $1 $svc\n";
            }
            $msg =~ s/([^\\])[(]/$1\\(/g;
            $msg =~ s/([^\\])[)]/$1\\)/g;

#            print STDERR "[TRACE::$msg] ::: ".@{$files{$f}}[$indices{$f}]."\n";
            unless (defined @{$files{$f}}[$indices{$f}]) {
                print STDERR "F: $f\n";
                print STDERR "indices{f}: ".$indices{$f}."\n"; 
                print STDERR "files{f}: ".$files{$f}."\n";
#                print STDERR "whole: ".@{$files{$f}}[$indices{$f}]."\n";
            }
            elsif (@{$files{$f}}[$indices{$f}] =~ /\d+\.\d+ \d{2} \[TRACE::$msg\] (.*)/) {
                $firstline = 0;
                $msg = $svc.$1;
            }
            elsif (@{$files{$f}}[$indices{$f}] =~ /\(demux\)\] (.*)/) {
                if ($1 =~ /^STARTING\(/) {
                }
                elsif ($1 =~ /^(deliver|forward)\(.*?s_deserialized=(\w+\(.*?\))/) {
                    $firstline = 0;
                    $msg = $svc.substr($1,0,1)."($2)";
                }
                else {
                    $firstline = 0;
                    $msg = $svc.$1;
                }
            }
        }
        if ($calldepth == 0) {
            while ($indices{$f} < scalar(@{$files{$f}}) && ! (@{$files{$f}}[$indices{$f}] =~ /STARTING[(]/)
                                                        && ! (@{$files{$f}}[$indices{$f}] =~ /RECEIVED (TCP|UDP) MESSAGE/)
                  ) {
                print OUTFILE ($noip ? "" : $hosts{$f}." ").@{$files{$f}}[$indices{$f}];
                $indices{$f}++;
            }
        }
        if ($indices{$f} >= scalar(@{$files{$f}})) {
            $filesdone++;
        }
    } 
    while ($calldepth > 0 && $indices{$f} < scalar(@{$files{$f}}));

    if($noip == 1){
      print OUTFILE " ]\n";
    }

    $noip = 0;

    $msg =~ s/\\\(/(/g;
    $msg =~ s/\\\)/)/g;

    print OUTFILE $hosts{$f}." [SimulateEventEnd] $i $minnode $evtype $rcvmsg $msg\n";

    $i++;
}
print "Found $i steps\n";

close(OUTFILE);
