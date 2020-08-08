#!/usr/bin/perl -w
# 
# makedep.pl : part of the Mace toolkit for building distributed systems
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

use Mace::Compiler::GeneratedOn;
use strict;

my @all = <*>;
my @perl = ();
my %dep = ();
my $cmakedep = shift || 0;
my $missingdir = shift || ".";

for my $f (@all) {
    if (-T $f) {
        open(F, $f) or die("cannot open file!");
	my $h = <F>;
        close(F);
	chomp($h);
	if ($h =~ m|^#!.*perl|) {
	    push(@perl, $f);
	    $dep{$f} = [];
	}
    }
}

my %mod = ();
my @mods = ();
for my $f (@perl) {
    my ($u, $r) = readMods($f);
    for my $m (@$u) {
	addDep(\%dep, $f, $m);
	if (!grep(/$m/, @mods)) {
	    push(@mods, $m);
	}
    }
    for my $m (@$r) {
	if (!grep(/$m/, @mods)) {
	    push(@mods, $m);
	}
    }
}

my @visited = ();
while (my $f = shift(@mods)) {
    push(@visited, $f);
    $f =~ s|::|/|g;
    $f .= ".pm";
    $f = findLib($f);
    my ($u, $r) = readMods($f);
    addDep(\%mod, $f);
    for my $m (@$u) {
	addDep(\%mod, $f, $m);
	if (!grep(/$m/, @visited)) {
	    push(@mods, $m);
	}
    }
    for my $m (@$r) {
	if (!grep(/$m/, @visited)) {
	    push(@mods, $m);
	}
    }
}

# use Data::Dumper;

print Mace::Compiler::GeneratedOn::generatedOnHeader("Perl5 Makefile Variables", "commentLinePrefix" => "#");

my %vars;

while (my ($k, $v) = each(%mod)) {
    my $t = makeVar($k);
    if ($cmakedep) {
        my $srcfile = $k;
        if (not $srcfile =~ m|$missingdir|) {
            $srcfile = "\${Mace_SOURCE_DIR}/perl5/$srcfile";
        }
        $vars{$t} = "SET($t $srcfile " . join(" ", map { '${' . makeVar($_) . '}' } @$v) . ")\n";
    }
    else {
        print "$t = \$(MACEPATH)/perl5/$k " . join(" ", map { '$(' . makeVar($_) . ')' } @$v) . "\n";
    }
}

while (my ($k, $v) = each(%dep)) {
    if ($cmakedep) {
        $vars{"${k}_dep"} = "SET(${k}_dep \${Mace_SOURCE_DIR}/perl5/$k " . join(" ", map { '${' . makeVar($_) . '}' } @$v) . ")\n";
        $vars{$k} = "SET($k \${Mace_SOURCE_DIR}/perl5/$k)\n";
    }
    else {
        print "${k}_dep = \$(MACEPATH)/perl5/$k " . join(" ", map { '$(' . makeVar($_) . ')' } @$v) . "\n";
        print "$k = \$(MACEPATH)/perl5/$k\n";
    }
}

if ($cmakedep) {
    my @order;
    my @varvalues = values(%vars);

    while (scalar(@varvalues)) {

        my %tvars = %vars;

        while (my ($key, $value) = each %tvars) {
            if (scalar(grep(/$key/, @varvalues)) == 1) {
                unshift @order, $value;
                delete ( $vars{$key} );
            }
        }

        @varvalues = values(%vars);

    }

    print join("", @order);

    print "SET(PERLBINS ".join(' ', keys(%dep)).")\n";
}
else {
    print "PERLBINS = ".join(' ', keys(%dep))."\n";
}

sub makeVar {
    my $t = shift;
    if ($t =~ m|$missingdir|) {
        $t =~ s|$missingdir(/)?||;
    }
    if ($t =~ m|/|) {
	$t =~ s|/|_|g;
	$t =~ s|\.pm|_dep|;
    }
    else {
	$t =~ s|::|_|g;
	$t .= "_dep";
    }
    return $t;
}

sub readMods {
    my $f = shift;
    my @u = ();
    my @r = ();
    if (not -e $f) {
	warn "WARNING: cannot read file $f\n";
	return ();
    }
    open(F, $f) or die "cannot open $f: $!";
    my @l = <F>;
    close(F);
    for my $l (@l) {
	chomp($l);
	if ($l =~ m|^use\s+(Mace::[\w:]+);$|) {
	    push(@u, $1);
	}
	elsif ($l =~ m|^require\s+(Mace::[\w:]+);$|) {
	    push(@r, $1);
	}
    }
    return (\@u, \@r);
}

sub addDep {
    my ($r, $k, $v) = @_;
    if (defined($r->{$k})) {
	my $ar = $r->{$k};
	if (defined($v) && !grep(/$v/, @$ar)) {
	    push(@$ar, $v);
	}
    }
    else {
	if (defined($v)) {
	    $r->{$k} = [$v];
	}
	else {
	    $r->{$k} = [];
	}
    }
}

sub findLib {
    my $f = shift;
    if (-e $f) {
        return $f;
    }
    for my $if (@INC) {
        if (-e "$if/$f") {
            return "$if/$f";
        }
    }
    return "$missingdir/$f";
}
