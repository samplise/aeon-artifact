# 
# ClassCache.pm : part of the Mace toolkit for building distributed systems
# 
# Copyright (c) 2011, Charles Killian, James W. Anderson, Adolfo Rodriguez, Dejan Kostic
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
package Mace::Compiler::ClassCache;

use strict;
use Class::MakeMethods::Utility::Ref qw( ref_clone );

use Mace::Compiler::Globals;
use Mace::Compiler::ClassParser;
use Mace::Util qw(:all);

use vars qw(%cache %dep %handlers @readFiles);

sub getServiceClasses {
    my @n = @_;
    my @r = ();
    for my $ns (@n) {
        my $n = $ns."ServiceClass";
	loadRecurse($n);
	push(@r, $cache{$n});
	for my $el (@{$dep{$n}}) {
	    if ($cache{$el}->isServiceClass() && !grep(/^$el$/, map { $_->name() } @r)) {
		push(@r, $cache{$el});
	    }
	}
    }
    return @r;
} # getServiceClass

sub getHandlers {
    my @n = @_;
    my @r = ();
    for my $ns (@n) {
        my $n = $ns."ServiceClass";
	loadRecurse($n);
	for my $el (@{$handlers{$n}}) {
	    if (!grep(/^$el$/, map { $_->name() } @r)) {
		push(@r, $cache{$el});
	    }
	}
    }
    return @r;
} # getHandlers

sub unionMethods {
    my @c = @_;
    my @r = ();
    for my $el (@c) {
	for my $m ($el->methods()) {
	    my $add = 1;
	    for my $r (@r) {
		if ($m->eq($r)) {
                    push(@{$r->options('class')}, $el);
		    $add = 0;
		}
	    }
	    if ($add) {
                $m->options('class', []);
                push(@{$m->options('class')}, $el); 
		push(@r, $m);
	    }
	}
    }
    my @re;
    for my $m (@r) {
        my @cl = @{$m->options('class')};
        $m->options('class', undef);
        my $m2 = ref_clone($m);
        $m->options('class', \@cl);
        $m2->options('class', \@cl);
        push(@re, $m2);
    }
    return @re;
} # unionMethods

sub loadRecurse {
    my $n = shift;

    unless (defined($cache{$n})) {
	load($n);
	$dep{$n} = [];
    }

    my $c = $cache{$n};
    if ($c->isServiceClass()) {
	for my $el (grep(/ServiceClass/, $c->superclasses())) {
	    addDep(\%dep, $n, $el);
	    loadRecurse($el);
	}
	for my $m ($c->methods()) {
	    if ($m->name() eq "registerHandler") {
		my $h = $m->params()->[0]->type()->type();
		loadRecurse($h);
		addHandler(\%handlers, \%dep, $h, $n);
	    }
	}
    }
} # loadRecurse

sub load {
    my $n = shift;

    if (defined($cache{$n})) {
	return;
    }

    my $f = "$n.h";
    my $path = findPath($f, @Mace::Compiler::Globals::INCLUDE_PATH);
    push(@readFiles, "$path/$f");
    # $Mace::Compiler::Globals::filename = "$path/$f";

    my @text = readFile("$path/$f");

    my @linemap;
    my @filemap;
    my @offsetmap;

    Mace::Compiler::MInclude::getLines("$path/$f", \@text, \@linemap, \@filemap, \@offsetmap);

    my $text = join("", @text);

    my $parser = Mace::Compiler::ClassParser->new();

    my $c = $parser->parse($text, $n, \@linemap, \@filemap, \@offsetmap);
    $cache{$n} = $c;
} # load

sub addHandler {
    my ($hr, $dr, $h, $n) = @_;
    unless (defined($hr->{$n})) {
	$hr->{$n} = [];
    }
    if (!grep(/^$h$/, @{$hr->{$n}})) {
	push(@{$hr->{$n}}, $h);
    }
    while (my ($k, $v) = each(%$dr)) {
	unless (defined($hr->{$k})) {
	    $hr->{$k} = [];
	}
	if (grep(/^$n$/, @$v) && !grep(/^$h$/, @{$hr->{$k}})) {
	    push(@{$hr->{$k}}, $h);
	}
    }
} # addHandler

sub addDep {
    my ($r, $n, $v) = @_;
    unless (defined($r->{$n})) {
	$r->{$n} = [];
    }
    if (!grep(/^$v$/, @{$r->{$n}})) {
	push(@{$r->{$n}}, $v);
    }
    for my $l (values(%$r)) {
	if (grep(/^$n$/, @$l) && !grep(/^$v$/, @$l)) {
	    push(@$l, $v);
	}
    }
} # addDep

1;
