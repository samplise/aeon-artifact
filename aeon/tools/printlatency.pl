#!/usr/bin/perl
# 
# printlatency.pl : part of the Mace toolkit for building distributed systems
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

my $lc = 0;
my %h = ();
my %stats = ();

while (my $l = <>) {
    $lc++;
    if ($l =~ m|^\d+\.\d+\s(\d+)\s+(\w+)|) {
# 	print "$1\t$2\n";
	my $lat = $1;
	my $el = $2;
	$h{$el} = "$lat $lc";
	my $s = { };
	if (!defined($stats{$el})) {
	    $stats{$el} = $s;
	}
	$s = $stats{$el};
	$s->{cum} += $lat;
	$s->{count}++;
	if (!defined($s->{min}) || $lat < $s->{min}) {
	    $s->{min} = $lat;
	}
	if (!defined($s->{max}) || $lat > $s->{max}) {
	    $s->{max} = $lat;
	}
    }
}

# use Data::Dumper;
# while (my ($k, $v) = each(%stats)) {
#     print "$k\n";
#     print Dumper($v);
# }

# for my $el (sort { $b <=> $a } keys(%h)) {
#     print "$el " . $h{$el} . "\n";
# }

my ($sel, $total, $count, $min, $max, $avg);

format STDOUT=
@<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< total: @######### count: @###### min: @##### max: @###### avg: @######
$sel,                                                                           $total,           $count,      $min,        $max,        $avg
.


for my $k (sort { $stats{$b}->{cum} <=> $stats{$a}->{cum} } keys(%stats)) {
    my $s = $stats{$k};
    $sel = $k;
    $total = $s->{cum};
    $count = $s->{count};
    $min = $s->{min};
    $max = $s->{max};
    $avg = $total / $count;
    write();
}
