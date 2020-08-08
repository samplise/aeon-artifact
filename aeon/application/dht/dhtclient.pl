#!/usr/bin/perl -w
# 
# dhtclient.pl : part of the Mace toolkit for building distributed systems
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
use File::Basename;
use lib ((dirname($0) || ".")."/../../mace-extras/perl5");

use Frontier::Client;
use Time::HiRes qw(gettimeofday tv_interval);

my $serverURL = 'http://localhost:6666/xmlrpc';
my $client = Frontier::Client->new('url' => $serverURL,
                                   'debug' => 0);

unless (scalar(@ARGV)) {
    printUsage();
}

my $command = shift(@ARGV);
my $key = shift(@ARGV) || "";
unless ($key) {
    printUsage();
}

my @margs = ($key);
my $method = "StringDHT.";
if ($command eq "lookup") {
    $method .= "containsKey";
}
elsif ($command eq "get") {
    $method .= "get";
}
elsif ($command eq "put") {
    $method .= "put";
    my $value = shift(@ARGV) || "";
    unless ($value) {
	printUsage();
    }
    push(@margs, $value);
}
elsif ($command eq "remove") {
    $method .= "remove";
}
else {
    die "unrecognized command $command\n";
}

my $start = [gettimeofday()];
my $r = $client->call($method, @margs);
my $elapsed = tv_interval($start);

# use Data::Dumper;
# print Dumper($r);

if ($command eq "get") {
    my ($v, $f) = @$r;
    print $f->value() . "\n$v\n";
}
elsif ($command eq "lookup") {
    print $r->value() . "\n";
}
else {
    print "$r\n";
}
print "took $elapsed\n";

sub printUsage {
    die "usage: $0 <command> [<options>]\n" .
	"    lookup <key>\n" .
	"    get <key>\n".
	"    put <key> <value>\n" .
	"    remove <key>\n" .
	"\n";
} # printUsage
