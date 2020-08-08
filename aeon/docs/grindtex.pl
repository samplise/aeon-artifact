#!/usr/bin/env perl
# 
# grindtex.pl : part of the Mace toolkit for building distributed systems
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

use lib ("../perl5", "$ENV{HOME}/mace/perl5");

use Mace::Util qw(:all);

my $f = shift(@ARGV);
unless (-e $f) {
    die "usage: $0 file.tex\n";
}

my @fb = readFile($f);
@fb = removeLatexComments(@fb);

my @t = ();
for my $el (@fb) {
    if ($el =~ m|\\input{(.*?)}|) {
	my $t = $1;
	if ($t =~ m|(.*?)-lg$|) {
	    push(@t, $1);
	}
    }
}

for my $el (@t) {
    my $tmp = `mktemp lgtmp.XXXXXXXXX`;
    chomp($tmp);
#     my $tmp2 = `mktemp lgtmp.XXXXXXXXX`;
#     chomp($tmp2);
    my $src = "${el}.tex";
    my $t = "${el}-lg.tex";

#     my $start = q{
# \\\\par\\\\addvspace{0.1in}
# \\\\ifLGnorules\\\\else\\\\hrule\\\\fi
# \\\\vskip .5\\\\baselineskip
# \\\\begingroup\\\\LGfsize\\\\LGindent\\\\z@
# \\\\begin{lgrind}};

#     my $cmd = qq{sed -e '$match' $src > $tmp};
    my $cmd = qq{perl -p -e 's|\\\\begin{programlisting}|\\\\vspace{-1em}\n%[|g' $src > $tmp};
#     my $cmd = qq{perl -p -e 's|\\\\begin{programlisting}|$start|g' $src > $tmp};
    print "$cmd\n";
    system($cmd);

#     my $end = q{
# \\\\end{lgrind}
# \\\\endgroup
# \\\\vskip .5\\\\baselineskip
# \\\\ifLGnorules\\\\else\\\\hrule\\\\vspace{0.1in}\fi
# };
    $cmd = qq{sed -i -e 's/\\\\end{programlisting}/%]/g' $tmp};
#     $cmd = qq{perl -p -e 's|\\\\end{programlisting}|$end|g' $tmp > $tmp2};
    print "$cmd\n";
    system($cmd);
    
    $cmd = qq{lgrind -d lgrindef-mace -lmace -e $tmp > $t};
    print "$cmd\n";
    system($cmd);

    unlink($tmp);
}
