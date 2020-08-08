#!/usr/bin/perl -w
# 
# parse-scoped-serialize.pl : part of the Mace toolkit for building distributed systems
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

# my $f = $ARGV[0] || "";
# unless (-f $f) {
#     print "usage: $0 file\n";
#     exit(1);
# }

my %s = ();

# open(F, $f) or die "cannot open $f: $!\n";

# while (my $l = <F>) {
#     if ($l =~ m|\[ScopedSerialize\] (\d+) (.*)|) {
# 	$s{$1} = $2;
# # 	print "adding $1 -> $2\n";
#     }
# }
# close(F);
# open(F, $f) or die "cannot open $f: $!\n";

# while (my $l = <F>) {
#     if ($l =~ m|\[ScopedSerialize\] (\d+) (.*)|) {
# 	next;
#     }
#     while ($l =~ m|\(str\)\[(\d+)\]|) {
# 	my $k = $1;
# 	if (defined($s{$k})) {
# 	    my $v = $s{$k};
# 	    $l =~ s|\(str\)\[$k\]|$v|g;
# 	}
# 	else {
# 	    $l =~ s|\(str\)\[$k\]|\(str\)$k|g;
# 	}
#     }
#     print $l;
# }

# close(F);

while (my $l = <>) {
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
	print $l;
    }
}
