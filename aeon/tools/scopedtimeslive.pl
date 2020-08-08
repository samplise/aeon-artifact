#!/usr/bin/perl -w
# 
# scopedtimeslive.pl : part of the Mace toolkit for building distributed systems
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

my %stack = ();
my %stats = ();

while (my $l = <>) {
    chomp($l);
    unless ($l =~ m/(STARTING|ENDING)/) {
	next;
    }
    
    my @a = split(/ /, $l);
    my $sc = scalar(@a);
    
    my $time = 0;
    my $sel = "";
    my $bound = "";
    if (scalar(@a) == 5) {
	$time = shift(@a);
	shift(@a); # thread id
	$sel = shift(@a);
	shift(@a); # scoped log
	$bound = shift(@a);
    }

    if ($bound eq "STARTING") {
	push(@{$stack{$sel}}, $time);
    }
    elsif ($bound eq "ENDING") {
	my $start = pop(@{$stack{$sel}});
	my $diff = $time - $start;
	my $s = { };
	if (!defined($stats{$sel})) {
	    $stats{$sel} = $s;
	}
	$s = $stats{$sel};
	$s->{cum} += $diff;
	$s->{count}++;
	if (!defined($s->{min}) || $diff < $s->{min}) {
	    $s->{min} = $diff;
	}
	if (!defined($s->{max}) || $diff > $s->{max}) {
	    $s->{max} = $diff;
	}
    }
    else {
	warn "ignoring unknown bound $bound\n";
    }
}

my ($sel, $total, $count, $min, $max, $avg);

format STDOUT=
@<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< total: @###.###### count: @###### min: @##### max: @###### avg: @######
$sel,                                                                           $total,           $count,      $min,        $max,        $avg
.



for my $k (sort { $stats{$b}->{cum} <=> $stats{$a}->{cum} } keys(%stats)) {
    my $s = $stats{$k};
    $sel = $k;
    $total = $s->{cum};
    $count = $s->{count};
    $min = $s->{min} * 1000000;
    $max = $s->{max} * 1000000;
    $avg = $total * 1000000 / $count;
    write();
}
