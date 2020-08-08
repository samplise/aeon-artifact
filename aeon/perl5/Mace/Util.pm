# 
# Util.pm : part of the Mace toolkit for building distributed systems
# 
# Copyright (c) 2011, James W. Anderson
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
package Mace::Util;

# Copyright 2005 James W. Anderson.  All rights reserved.
# This program is free software; you can redistribute it and/or
# modify it under the same terms as Perl itself.

use strict;

BEGIN {
    use Exporter();
    use vars qw(@ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
    @ISA = qw(Exporter);
    @EXPORT = qw();
    @EXPORT_OK = qw();

    %EXPORT_TAGS = (
                    all => [qw(
			       &removeCComments
			       &removeLatexComments
			       &removeHashComments
                               &trim
			       &readFile
			       &findPath
			       &countOpenBraces
			       &countCloseBraces
			       &irand
			       &indentFile
			       &indentStr
			       &prettifyXmlElement
                               &typeToTable
                               )],
                    );
    &Exporter::export_ok_tags("all");
}

sub indentStr {
    my $s = shift;
    my $inf = "";
    my $inh;
    open($inh, ">", \$inf);
    print $inh $s;
    close($inh);
    open($inh, "<", \$inf);
    my $outf = "";
    my $outh;
    open($outh, ">", \$outf);
    indentFile($inh, $outh);
    close($inh);
    close($outh);
    return $outf;
}

sub indentFile {
    my ($in, $out) = @_;
    my $INDENT = 2;
    my $n = 0;
    while (my $l = <$in>) {
	$l = trim($l);
	my $first = substr($l, 0, 1);
        my $nt = $n;
	if ($first eq ')') {
	    $nt -= $INDENT * 2;
	}
	if ($first eq '}') {
	    $nt -= $INDENT;
	}
	print $out (' ' x $nt) . $l . "\n";
	for my $c (split(//, $l)) {
	    if ($c eq '{') {
		$n += $INDENT;
	    }
	    elsif ($c eq '}') {
		$n -= $INDENT;
	    }
	    elsif ($c eq '(') {
		$n += $INDENT * 2;
	    }
	    elsif ($c eq ')') {
		$n -= $INDENT * 2;
	    }
	}
        if($n < 0) { $n = 0; }
    }
}

sub readFile {
    my $f = shift;
    my @a = ();

    open(IN, $f) or die "cannot open $f: $!\n";

    while (my $l = <IN>) {
	push(@a, $l);
    }

    close(IN);

    return wantarray ? @a : join("", @a);
} # readFile

sub findPath {
    my ($f, @p) = @_;
    for my $p (@p) {
	if (-e "$p/$f") {
	    return $p;
	}
    }
    return "";
} # findPath

sub removeHashComments {
    my @a = @_;
    my @b = ();
    while (my $l = shift(@a)) {
	$l =~ s|(^\s*\#.*\n)$||;
	$l =~ s|(\#.*)$||;
	if (length($l)) {
	    push(@b, $l);
	}
    }
    
    return @b;
} # removeHashComments

sub removeLatexComments {
    my @a = @_;
    my @b = ();
    while (my $l = shift(@a)) {
	$l =~ s|(^\s*%.*\n)$||;
	$l =~ s|(%.*)$||;
	if (length($l)) {
	    push(@b, $l);
	}
    }
    
    return @b;
} # removeLatexComments

sub removeCComments {
    my @a = @_;
    my $comment = 0;
    my @b = ();
    my $i = 0;
    while (my $l = shift(@a)) {
	$i++;
	my $s = "";
	if ($comment) {
	    if ($l =~ m|(.*\*/)(.*)|) {
		$s = $2;
		if ($s =~ m|^\s*$|) {
		    $s = "";
		}
		$comment = 0;
	    }
	}
	else {
	    $l =~ s|(^\s*//.*\n)$||;
	    $l =~ s|(//.*)$||;
	    $l =~ s|(^\s*/\*.*\*/\n)$||;
	    $l =~ s|(/\*.*\*/)||g;

	    if ($l =~ m|(.*?)/\*|) {
		$s = $1;
		if ($s =~ m|^\s*$|) {
		    $s = "";
		}
		$comment = 1;
	    }
	    else {
		$s = $l;
	    }
	}

#  	print "$i:'$s'\n";
	push(@b, $s);
    }

    return @b;
} # removeCComments

sub countCloseBraces {
    my $s = shift;
    $s = cancelBraces($s);
    return $s =~ tr|\}|\}|;
} # countCloseBraces

sub countOpenBraces {
    my $s = shift;
    $s = cancelBraces($s);
    return $s =~ tr|\{|\{|;
} # countOpenBraces

sub cancelBraces {
    my $s = shift;
    my @c = split(//, $s);
    my @o = ();
    my @e = ();
    for (my $i = 0; $i < scalar(@c); $i++) {
	if ($c[$i] eq "{") {
	    push(@o, $c[$i]);
	}
	elsif ($c[$i] eq "}") {
	    if (scalar(@o) == 0) {
		push(@e, $c[$i]);
	    }
	    else {
		pop(@o);
	    }
	}
    }
    return join("", @o) . join("", @e);
} # cancelBraces

sub trim {
    my @out = @_;
    for (@out) {
        s/^\s+//g;
        s/\s+$//g;
    }
    return wantarray ? @out : $out[0];
} # trim

sub irand {
    my $i = shift;
    return int(rand($i));
} # irand

sub prettifyXmlElement {
    my $e = shift;
    my $sp = shift || 0;
    my %args = @_;
    my $INLINE = $args{-inline} || [];
    my $INDENT = $args{-indent} || [];
    my $EXTRALINE = $args{-extraline} || [];
    my $STARTLINE = $args{-startline} || [];
    my $NOINDENTEND = $args{-noindentend} || [];

    my $sps = ' ' x $sp;

    my $tag = $e->tag();
    my $r = "";
    my $inline = grep(/^$tag$/, @$INLINE);
    my $indent = grep(/^$tag$/, @$INDENT);
    my $extraline = grep(/^$tag$/, @$EXTRALINE);

    if ($inline) {
	if ($indent) {
	    $r .= $sps;
	}
    }
    else {
	$r .= $sps;
    }

    $r .= $e->starttag();

    if (scalar($e->content_refs_list())) {
	unless ($inline) {
	    $r .= "\n";
	}

	for my $el ($e->content_refs_list()) {
	    if (ref $$el) {
		my $ntag = $$el->tag();
		if (grep(/^$ntag$/, @$STARTLINE) and $r !~ m|\n\n$|) {
		    $r .= "\n";
		}
		$r .= prettifyXmlElement($$el, $sp + 2, %args);
	    }
	    else {
		$r .= $$el;
	    }
	}

	if ($inline) {
	    $r .= $e->endtag();
	    if ($indent) {
		$r .= "\n";
	    }
	}
	else {
	    unless ($r =~ m|\n$|) {
		$r .= "\n";
	    }
	    unless (grep(/^$tag$/, @$NOINDENTEND)) {
		$r .= $sps;
	    }
	    $r .= $e->endtag() . "\n";
	}
    }
    else {
	$r =~ s|>$| />|;
	$r .= "\n";
    }

    if ($extraline) {
	$r .= "\n";
    }
    
    return $r;
} # prettifyXmlElement

sub loadConfigHash {
    my $f = shift;
    my $h = shift || { };
    my $order = shift || [];
    unless (-e $f) {
	die "cannot load config file $f: $!\n";
    }

    my @d = `cat $f`;
    for my $el (@d) {
	chomp($el);
        $el =~ s|#.*$||;
        if ($el =~ m|^(.*?)=(.*)|) {
	    my $name = trim($1);
	    my $value = trim($2);
	    $h->{$name} = $value;
	    push(@$order, $name);
	}
    }

    return $h;
} # loadConfigHash

sub loadConfig {
    my $f = shift;
    my %args = @_;
    unless (-e $f) {
	die "cannot load config file $f: $!\n";
    }

    my @d = `cat $f`;
    for my $el (@d) {
	chomp($el);
        $el =~ s|#.*$||;
        if ($el =~ m|^(.*?)=(.*)|) {
	    my $name = trim($1);
	    my $value = trim($2);
	    my $found = 0;
	    for my $k (keys(%args)) {
		if ($k =~ m|^$name=(.)|) {
		    my $type = $1;
		    my $ok = 0;
		    if (($type eq 'i') && ($value =~ m|^-?\d+$|)) {
			$ok = 1;
		    }
		    elsif ($type eq 's') {
			$ok = 1;
		    }
		    elsif (($type eq 'b') && (($value == 1) || ($value == 0))) {
			$ok = 1;
		    }
		    else {
			die "unknown parameter type $type\n";
		    }
		    if (!$ok) {
			die "bad $type type $value for $name\n";
		    }
		    ${$args{$k}} = $value;
		    $found = 1;
		    last;
		}
	    }
	    if (!$found) {
		warn "unknown parameter $name\n";
	    }
        }
    }
} # loadConfig

sub typeToTable {
    my $str = shift;
    $str =~ tr/*&<>: /_/;
    return $str;
} # typeToTable

1;
