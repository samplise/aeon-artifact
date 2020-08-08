#!/usr/bin/perl -w
# 
# mhdep.pl : part of the Mace toolkit for building distributed systems
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

#use lib $ENV{HOME} . "/mace/perl5";

use Cwd;
use File::Basename;
use Carp;
use lib ((dirname($0) || "."), (dirname($0) || ".")."/../mace-extras/perl5");

use Mace::Util qw{:all};
use Mace::Dependency::MaceHeaderParser;
use Mace::Dependency::MaceParser;
use Mace::Compiler::VariantRecDescent;
use Mace::Compiler::MInclude;

#my ($inf, $outf) = @ARGV;

#unless (-e $inf) {
#    die "usage: $0 input-file [output-file]\n";
#}

#$SIG{__DIE__} = sub{ confess $_[0]; };
#$SIG{__WARN__} = sub{ confess $_[0]; die; };

my $parser = Mace::Dependency::MaceHeaderParser->new();
my $maceparser = Mace::Dependency::MaceParser->new();

my $srcdir = shift;
my $bindir = shift;
my $dirname = basename($bindir);

my %vars;

while(glob("*ServiceClass.h")) {
    my $we = $_;
    $we =~ s/.h$//;
    $vars{"${we}_h"} = "SET(${we}_h ${srcdir}/$_)\n";
}

while(glob("*Handler.h")) {
    my $we = $_;
    $we =~ s/.h$//;
    $vars{"${we}_h"} = "SET(${we}_h ${srcdir}/$_)\n";
}

while(glob("*.mh")) {
#     print STDERR "Parsing file $_\n";
    my $we = $_;
    $we =~ s/.mh$//;
    $vars{"${we}_h"} = "SET(${we}_h ${bindir}/${we}.h)\n";
    $vars{"${we}_dep"} = "SET(${we}_dep \${${we}_h})\n";
    open(IN, $_) or die "cannot open $_: $!\n";
    my @inl = <IN>;
    my $in = join('',@inl);
    close(IN);

    $parser->parse($in, $we, \%vars);
}

for my $ext (".m",".mac",".mace") {
    while(glob("*$ext")) {
#         print STDERR "Parsing file $_\n";
        my $we = $_;
        $we =~ s/$ext$//;
        print "SET(ALL_SERVICES \${ALL_SERVICES} $we)\n";
        $vars{"${we}Service_lib"} = "SET(${we}Service_lib $dirname)\n";
        open(IN, $_) or die "cannot open $_: $!\n";
        my @inl = <IN>;
        my $in = join('',@inl);
        close(IN);

	my @minclude = map { /"(.*)"/; $_ = ${srcdir}."/".$1 } grep(/#minclude/, @inl);
	if (scalar(@minclude)) {
	    $vars{"${we}Service_mi_dep"} =
		"SET(${we}Service_mi_dep " . join(" ", @minclude) . ")\n";
	}

# 	print STDERR "beginning parse\n";
        $maceparser->parse($in, $we, \%vars);
# 	print STDERR "completed parse\n";
    }
}

while(glob("*.vmac")) {
    my $we = $_;
    $we =~ s/\.vmac$//;

    open(IN, $_) or die "cannot open $_: $!\n";
    my @in = <IN>;
    my $in = join('',@in);
    close(IN);

    my @linemap;
    my @filemap;
    my @offsetmap;

    Mace::Compiler::MInclude::getLines($_, \@in, \@linemap, \@filemap, \@offsetmap);

    $Mace::Compiler::Grammar::text = $in;

    $parser = Mace::Compiler::VariantRecDescent->new();

    $parser->{local}{linemap} = \@linemap;
    $parser->{local}{filemap} = \@filemap;
    $parser->{local}{offsetmap} = \@offsetmap;

    my $variants = $parser->VariantFile($in, 1, $we, 0);
    print "# DEBUG: we $we variants $variants\n";
    if (defined $variants) {
        my @vararr = @{$variants};
        print "SET(${we}_VARIANTS @vararr)\n";
        print "SET(ALL_SERVICES \${ALL_SERVICES} @vararr)\n";
        for my $variant (@{$variants}) {
            $vars{"${variant}Service_lib"} = "SET(${variant}Service_lib $dirname)\n";
            $in = $parser->GetVariant("", 0, $variant);
            $maceparser->parse($in, $variant, \%vars);
        }
    }

    my @minclude = map { /"(.*)"/; $_ = ${srcdir}."/".$1 } grep(/#minclude/, @in);
    if (scalar(@minclude)) {
        $vars{"${we}Service_mi_dep"} =
            "SET(${we}Service_mi_dep " . join(" ", @minclude) . ")\n";
    }
}

while(glob("*-init.h")) {
#     print STDERR "printing service deps\n";
    my $we = $_;
    $we =~ s/-init\.h$//;
    print "SET(ALL_SERVICES \${ALL_SERVICES} $we)\n";
    $vars{"${we}Service_lib"} = "SET(${we}Service_lib $dirname)\n";
    $vars{"${we}Service_sv_dep"} = "SET(${we}Service_sv_dep $we)\n";
}

my @order;
my @varvalues = values(%vars);

while (scalar(@varvalues)) {
#    print STDERR "ordering vars: @varvalues\n";

    my %tvars = %vars;

    while (my ($key, $value) = each %tvars) {
#	print STDERR "key=$key value=$value\n";
        if (scalar(grep(/\{$key\}/, @varvalues)) == 0) {
            unshift @order, $value;
            delete ( $vars{$key} );
        }
    }

    @varvalues = values(%vars);

}

print join("", @order);
