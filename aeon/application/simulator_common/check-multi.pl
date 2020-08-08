#!/usr/bin/perl -w
# 
# check-multi.pl : part of the Mace toolkit for building distributed systems
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

use lib $ENV{HOME} . '/mace/perl5';

use Carp;
use Mace::Util qw(:all);
use Mail::Sendmail;
use Getopt::Long;
use Sys::Hostname;
use File::Basename;
use Cwd;

my $email = "";
Getopt::Long::Configure("pass_through");
GetOptions("email=s" => \$email);

my $config = shift(@ARGV) || printUsage();

my %initargs = @ARGV;

# use Data::Dumper;
# print Dumper(%initargs);
# exit(0);

my %p = ();
my @order = ();
Mace::Util::loadConfigHash($config, \%p, \@order);

# use Data::Dumper;
# print Dumper(%p);

my @vals = ();
for my $k (@order) {
    my $v = $p{$k};
    my @t = split(/ /, $v);
    my @a = ();
    for my $el (@t) {
	if ($el =~ m|^(\d+)\.\.(\d+)$|) {
	    my $start = $1;
	    my $end = $2;
	    for my $i ($start..$end) {
		push(@a, $i);
	    }
	}
	else {
	    push(@a, $el);
	}
    }
    push(@vals, [$k, \@a]);
}

# print Dumper(@vals);

my @arglist = ();
genArgs(\@vals);

use FindBin;
my $path = $FindBin::Bin;
my $mc = "$path/run-modelchecker.pl";

for my $el (@arglist) {
    my $cmd = "$mc $el";
    print "$cmd\n";
    if (my $pid = fork()) {
	waitpid($pid, 0);
    }
    else {
	exec($cmd);
    }
    my $returnStatus = $? >> 8;
    if ($returnStatus != 0) {
	print "exiting on args $el\n";

	if ($email) {
	    my $hostname = hostname();
	    my $cwd = getcwd();
	    my $body =<<EOF;
modelchecker failed on $el
$hostname:$cwd
EOF
	    my %mail = ( To => $email,
			 From => $email,
			 Subject => "modelchecker failed",
			 Message => $body,
			 );
	    sendmail(%mail) or die $Mail::Sendmail::error;
	}
	
	exit(0);
    }
}


sub genArgs {
    my $va = shift;
    my $s = shift || "";
    if (scalar(@$va)) {
	my $l = shift(@$va);
	my ($k, $a) = @$l;
	my $useInit = 0;
	my $test = 0;
	if (defined($initargs{"-$k"})) {
	    $useInit = 1;
	    $test = 1;
	}
	for my $el (@$a) {
	    if ($useInit && $test) {
		if ($el eq $initargs{"-$k"}) {
		    $test = 0;
		}
		else {
# 		    print "skipping $k $el\n";
		    next;
		}
	    }
	    delete($initargs{"-$k"});
	    genArgs($va, "$s -$k $el");
	}
	unshift(@$va, $l);
    }
    else {
	push(@arglist, $s);
    }
}

sub printUsage {
    my $usage =<<EOF;
usage: $0 config [-e email] [-initial_param val [...]]
EOF

    die $usage;
}
