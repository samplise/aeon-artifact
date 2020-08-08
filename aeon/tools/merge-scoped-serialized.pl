#!/usr/bin/perl -w
# 
# merge-scoped-serialized.pl : part of the Mace toolkit for building distributed systems
# 
# Copyright (c) 2011, Charles Killian, Dejan Kostic, Ryan Braud, James W. Anderson, John Fisher-Ogden, Calvin Hubble, Duy Nguyen, Justin Burke, David Oppenheimer, Amin Vahdat, Adolfo Rodriguez, Sooraj Bhat
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
use FileHandle;

if (scalar(@ARGV) < 2) {
    print "usage: $0 dir file [file ...]\n";
    exit(1);
}

my $dir = shift(@ARGV);
mkdir($dir) or die "cannot mkdir $dir: $!\n";

my %f = ();
my %o = ();

for my $el (@ARGV) {
    print "opening $el\n";
    my $fh = new FileHandle;
    if ($fh->open("< $el")) {
	$f{$el} = $fh;
    }
    else {
	die "cannot open $el: $!\n";
    }

    my $oh = new FileHandle;
    if ($oh->open("> $dir/$el\n")) {
	$o{$el} = $oh;
    }
    else {
	die "cannot open $dir/el: $!\n";
    }
}

my %s = ();

my %lines = ();
my $last = "";

while (my ($k, $fh) = each(%f)) {
    if (my $l = <$fh>) {
	if ($l =~ m|\d+\.\d{6}|) {
	    $lines{$k} = $l;
	}
	else {
	    die "no timestamp: $l\n";
	}
    }
    else {
	delete($f{$k});
    }
}

while (scalar(%lines)) {
    my $l = nextLine();
    if ($l =~ m|\[ScopedSerialize\] (\d+) (.*)|) {
	$s{$1} = $2;
# 	print "adding $1 -> $2\n";
    }
    else {
	while ($l =~ m|\(str\)\[(\d+)\]|) {
	    my $k = $1;
	    if (defined($s{$k})) {
		my $v = $s{$k};
		$l =~ s|\(str\)\[$k\]|$v|g;
	    }
	    else {
		$l =~ s|\(str\)\[$k\]|\(str\)$k|g;
	    }
	}
	my $fh = $o{$last};
	print $fh $l;
    }
}

sub nextLine {
    if ($last) {
	my $fh = $f{$last};
	if (my $l = <$fh>) {
	    if ($l =~ m|\d+\.\d{6}|) {
		$lines{$last} = $l;
	    }
	    else {
		die "no timestamp: $l\n";
	    }
	}
	else {
	    delete($f{$last});
	}
    }
    my @sorted = sort {
	substr($lines{$a}, 0, 17) <=> substr($lines{$b}, 0, 17)
    } keys(%lines);
    $last = shift(@sorted);
    my $r = $lines{$last};
    delete($lines{$last});
    return $r;
} # nextLine
