#!/usr/bin/perl -w
# 
# generateEventGraph.pl : part of the Mace toolkit for building distributed systems
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
use Carp;
use lib ($ENV{HOME} . '/mace/perl5', $ENV{HOME}.'/lib/perl5');
use Mace::Util qw(:all);
use Getopt::Long;
use IO::Handle;
use IO::File;
use FileHandle;
use Term::ReadKey;
use Term::ANSIColor;

use constant NODE_COLOR => "bold green";
use constant APP_EVENT_COLOR => "bold blue";
use constant NET_EVENT_COLOR => "bold magenta";
use constant SCHED_EVENT_COLOR => "bold yellow";
use constant TRANSITION_COLOR => "bold cyan";
use constant DROP_COLOR => "bold red";
use constant FAILURE_COLOR => "bold red";
use constant SEQUENCE_COLOR => "bold white";

use constant SINGLE_INDENT => 6;
use constant LONG_INDENT => 19;

#$SIG{__DIE__} = sub{ confess $_[0]; };
#$SIG{__WARN__} = sub{ confess $_[0]; die; };

# my @termSize = GetTerminalSize();
# my $termWidth = shift(@termSize);
# my $termHeight = shift(@termSize);

my $starttime = 0;

my $termWidth = 0;

my $showStep = undef;
my $graph = 1;
my $uncolored = 0;
my $ignoreSigInt = 0;
my $outfile = "";
my $noRequireNode = 1;
my $showTimes = 0;
my $longGraph = 1;
my $columns = 0;
my $formattedInput = 0;
my $help = 0;

GetOptions("help" => \$help,
	   "width=i" => \$termWidth,
	   "step=i" => \$showStep,
	   "columns" => \$columns,
	   "uncolored" => \$uncolored,
	   "ignoreint" => \$ignoreSigInt,
           "nonode"    => \$noRequireNode,
           "time"      => \$showTimes,
	   "formatted" => \$formattedInput,
	   "outfile=s" => \$outfile);

if ($help) {
    print<<EOF;
usage: $0 [options] file
    -h help
    -w=i width
    -s=i show step
    -c use columns
    -u uncolored
    -i ignore sig int
    -n no node names
    -t show times
    -f formatted input
    -o=s output file
EOF
    exit(0);
}

if ($columns) {
    $longGraph = 0;
}

if (defined($showStep)) {
    $graph = 0;
    STDERR->autoflush(1);
}

unless ($termWidth) {
    my @termSize = GetTerminalSize();
    $termWidth = shift(@termSize);
}

# $SIG{__WARN__} = sub { confess $_[0] };

if ($ignoreSigInt) {
    $SIG{INT} = "IGNORE";
}

# print "using $termWidth width\n";

my $propertyCheck = "";

my $startLog = 0;
my $seBegin = 1;

my %nodes = ();
my %rnodes = ();
my @events = ();
my @pathInputs = ();

if ($formattedInput) {
    readFormattedInput();
}
else {
    readMCInput();
}

if (!scalar(@events)) {
    print "no events\n";
    exit(0);
}

if (!defined($events[-1]->{message})) {
    warn "log ends in incomplete event\n";
    $events[-1]->{message} = "???";
}

my $maxStep = scalar(@events);
my $stepWidth = length($maxStep) + 1;
my $numNodes = scalar(keys(%nodes));
my $nodeWidth = int(($termWidth - $stepWidth) / $numNodes);
my $halfNodeWidth = int($nodeWidth / 2);

my $graphWidth = $numNodes * 2;

my $extrapad = 0;
# print "termWidth=$termWidth nodeWidth=$nodeWidth halfNodeWidth=$halfNodeWidth\n";
# print "numNodes=$numNodes stepWidth=$stepWidth\n";
if ($nodeWidth % 2) {
    $extrapad = 1;
}

my %nodeEvents = ();


my %netEdges = ();
my %sendSteps = ();

my @rows = ();

for my $e (@events) {
    if ($e->{step} % 100 == 0) {
        print STDERR "Generating graph for step ".$e->{step}."\n";
    }

#     use Data::Dumper;
#     print "processing " . Dumper($e) . "\n";
    
    my $s = "";
    my $step = $e->{step};
#    print "processing $step\n";
#     my $tref = $e->{transitions};
#     print Dumper($tref) . "\n";
    if (defined($showStep)) {
	if ($step != $showStep) {
	    next;
	}
    }
    my $stepStr =  ' ' x (length($maxStep) - length($step)) . $step . ' ';
    if ($graph) {
	$s .= ccolor($stepStr, TRANSITION_COLOR);
    }
    my $type = substr($e->{type}, 0, 3);
    my $emsg = $e->{message};
    my $id = $e->{id};
    my $node = (defined($e->{node}) ? $e->{node} : $rnodes{$e->{ip}});
    if (!defined($node)) {
	print $e->{ip} . " " . $rnodes{$e->{ip}} . "\n";
	exit(0);
    }

    my $dotSpace = "";
    for my $i (1..$node * $nodeWidth) {
	if ($i % 2 == 1) {
	    $dotSpace .= '.';
	}
	else {
	    $dotSpace .= ' ';
	}
    }
    if (substr($dotSpace, -1) eq ".") {
	$dotSpace = substr($dotSpace, 0, -1) . " ";
    }
    
    my $space = ' ' x ($node * $nodeWidth);
    if ($graph) {
	if ($longGraph) {
	    for (1..$node) {
		$s .= "  ";
	    }
	    $s .= ccolor("* ", TRANSITION_COLOR);
	    for ($node..($numNodes - 2)) {
		$s .= "  ";
	    }
	}
	else {
	    $s .= ccolor($dotSpace, TRANSITION_COLOR);
	}
    }

    my $netId = -1;
    my $color = "";
    if ($type eq "NET") {
	if ($emsg =~ m|^(.* from \S+)|) {
	    $emsg = $1;
	}
	if ($emsg =~ m|id (\d+) dest_not_ready|) {
	    $netId = $1;
	    for my $r (@rows) {
		if ($r =~ m|route pkt\(id=$netId|) {
		    $r =~ s/(<|>)/x/;
		}
	    }
	    $color = DROP_COLOR;
	}
	else {
	    if ($emsg =~ m|id (\d+)|) {
		$netId = $1;
	    }
# 	    else {
# 		die "could not find net id in $emsg\n";
# 	    }
	    $color = NET_EVENT_COLOR;	
	}
	if ($netId >= 0 && $longGraph) {
	    if (defined($sendSteps{$netId})) {
		$emsg .= " (sent " . $sendSteps{$netId} . ")";
	    }
	    else {
		print STDERR "no step for $netId\n";
		#confess "no step for $netId\n";
	    }
	}
    }
    elsif ($type eq "SCH") {
	if ($emsg !~ m/Assert/) {
	    if ($emsg =~ m|(.*)Service::(.*?)\(\d+\)|) {
		my $service = $1;
		my $timer = $2;
		$service =~ tr/a-z_//d;
		$emsg = "$service $timer";
	    }
	    elsif ($emsg =~ m|(.*)::expire::(.*)::\(timer\)|) {
		my $service = $1;
		my $timer = $2;
		$service =~ tr/a-z_//d;
		$emsg = "$service $timer";
	    }
            elsif ($emsg =~ m|([A-Z]+)\.([a-zA-Z_])+\(|) { }
            elsif ($emsg =~ m|([A-Z]+)\.calling expire into service for (.*)|) { 
                $emsg = "$1 $2";
            }
	    else {
		warn "could not parse service from $emsg in step $step\n";
	    }
	}
	$color = SCHED_EVENT_COLOR;
    }
    else {
	if ($type ne "APP") {
	    die "unknown type: $type\n";
	}
	$color = APP_EVENT_COLOR;
	if ($emsg =~ m|FAILURE|) {
	    $type = "APP FAILURE";
	    $emsg = "";
	    $color = FAILURE_COLOR;
	}
    }

    if (!$formattedInput) {
	while (my ($k, $v) = each(%nodes)) {
	    $emsg =~ s|$v|$k|g;
	}
    }

    if ($e->{tme}) {
	$emsg .= " ".$e->{tme};
    }

    my $indent = 2;
    my $dwidth = $nodeWidth - ($indent + 1);
    if ($longGraph) {
	$dwidth = $termWidth - ($stepWidth + $graphWidth + $indent + 1);
    }

    if ($graph) {
	$s .= ccolor("$type", $color, "/$node ", NODE_COLOR,
		     $emsg, TRANSITION_COLOR, $dwidth - 6);
    }
    else {
	$s .= ccolor("$type", $color, "/$node ", NODE_COLOR,
		     trim($emsg), TRANSITION_COLOR, " [node $node]", NODE_COLOR);

    }

    push(@rows, $s);
    $s = "";

    if ($emsg =~ m|Assert\((.*)\) failed|) {
	$emsg = "\nassertion failed\n$1";
    }
#     my $nodeLabel = "$step: $emsg";
    if (defined($e->{transitions})) {
	for my $t (@{$e->{transitions}}) {
	    $stepStr =~ s/\d/ /g;
	    my $pre = "";
	    if ($graph) {
		if ($longGraph) {
		    $s .= $stepStr;
		}
		else {
		    $pre = $stepStr . $space;
		}
	    }
	    $pre .= ' ' x $indent;
	    my $src = $t->{src};
	    my $dest = $t->{dest};
	    if ($formattedInput) {
		$src = $rnodes{$src};
		$dest = $rnodes{$dest};
	    }
	    my $service = $t->{service};
	    my $message = $t->{message};
	    my $messageFields = $t->{messageFields};
	    my $method = $t->{method};
	    my $detail = $t->{detail};
	    my $direction = $t->{direction};
	    my $sim = $t->{sim};
	    my $drop = $t->{drop};

	    if ($method =~ m/^(route|routeRTS|send|deliver)$/) {
		unless ($src =~ m|^SHA\d*/| or $dest =~ m|^SHA\d*/| or $dest =~ m|^STR/| or $src =~ m|^STR/|) {
		    if (!defined($rnodes{$src})) {
			unless ($noRequireNode) {
			    die "unknown node '$src' $service $method $direction\n";
			}
		    }
		    else {
			$src = $rnodes{$src};
		    }
		    if (!defined($rnodes{$dest})) {
			unless ($noRequireNode) {
			    die "unknown node '$dest'\n";
			}
		    }
		    else {
			$dest = $rnodes{$dest};
		    }
		}
	    }

	    while (my ($k, $v) = each(%nodes)) {
		$messageFields =~ s|$v|$k|g;
		$detail =~ s|$v|$k|g;
	    }

	    my $str = "";
	    if ($direction eq "upcall") {
		$str .= "u ";
	    }
	    elsif ($direction eq "scheduler") {
		$str .= "s ";
	    }
	    elsif ($direction eq "error") {
		$str .= "! ";
	    }
	    else {
		$direction eq "downcall" or die "bad direction: $direction\n";
		$str .= "d ";
	    }
	    my $abbrvService = $service;
	    $abbrvService =~ tr/a-z_//d;
	    $str .= "$abbrvService ";
	    if ($method =~ /^(multicast|route|routeRTS|send)$/) {
		$str .= "$method $message";

		if ($message eq "pkt" && $messageFields =~ m|\(id=(\d+)\)|) {
		    $netId = $1;
#		    $netEdges{$netId}->{src} = $gn;
		    $sendSteps{$netId} = $step;
		}

		my $fields = "";
		if ($graph && length($str . $messageFields) > $dwidth) {
		    if ($longGraph) {
			$str .= compressFields($messageFields);
		    }
		    else {
			$fields = $pre . substr($messageFields, 0, $dwidth);
		    }
		}
		elsif ($longGraph) {
		    $str .= compressFields($messageFields);
		}
		else {
		    $str .= $messageFields;
		}
		if ($graph) {
		    $str = substr($str, 0, ($dwidth - 5));
		    if ($method ne "multicast") {
			if ($dest =~ m=(SHA|IPV|STR)\d*/= or $dest =~ m/client\d+/ or not $dest =~ /^(\d+)$/) {
                            if ($longGraph) {
                                $s .= ' ' x $graphWidth;
                            }
			    $str .= " to $dest";
			}
			elsif ($dest < $src) {
			    my $head = $drop ? "X" : "<";
			    if ($longGraph) {
				my $arrowch = $sim ? "=" : "-";
				for (1..$dest) {
				    $s .= "  ";
				}
				$s .= $head . $arrowch;
				for ($dest..($src - 2)) {
				    $s .= $arrowch x 2;
				}
				$s .= "$arrowch ";
				for ($src..($numNodes - 2)) {
				    $s .= "  ";
				}
			    }
			    else {
				my $arrowch = $sim ? "--" : " -";
				my $nodediff = $src - $dest - 1;
				$pre = $stepStr . (' ' x ($halfNodeWidth + ($dest * $nodeWidth)));
				my $n = $halfNodeWidth + $extrapad + ($nodediff * $nodeWidth);
				my $pad = "";
				if ($n % 2) {
				    $pad = "-";
				}
				$n = int($n / 2);
				$pre .= $head . $pad . ($arrowch x $n) . " ";
			    }
			}
			elsif ($dest > $src) {
			    my $head = $drop ? "X" : ">";
			    if ($longGraph) {
				my $arrowch = $sim ? "=" : "-";
				for (1..$src) {
				    $s .= "  ";
				}
				$s .= $arrowch x 2;
				for ($src..($dest - 2)) {
				    $s .= $arrowch x 2;
				}
				$s .= "$head ";
				for ($dest..($numNodes - 2)) {
				    $s .= "  ";
				}
			    }
			    else {
				my $arrowch = $sim ? "--" : "- ";
				my $n = $nodeWidth - length($str) + $halfNodeWidth - 5;
				$n += (($dest - $src - 1) * $nodeWidth);
				my $pad = "";
				if ($n % 2) {
				    $pad = "-";
				}
				$n = int($n / 2);
				$str .= " " . ($arrowch x $n) . $pad . $head;
			    }
			}
			else {
			    if ($longGraph) {
				for (1..$src) {
				    $s .= "  ";
				}
				$s .= "^ ";
				for ($src..($numNodes - 2)) {
				    $s .= "  ";
				}
			    }
			    else {
				$pre .= " to self";
			    }
			}
		    }
                    else {
                        $s .= ' ' x $graphWidth;
                    }
		}
		if ($dest ne $src) {
		    $str .= " -> $dest";
		}
		else {
		    $str .= " to self";
		}
		$s .= $pre;
		$s .= $str;
		if ($longGraph) {
		    push(@rows, $s);
		}
		else {
		    push(@rows, substr($s, 0, $termWidth - SINGLE_INDENT));
		}
		$s = "";
		if ($fields) {
		    push(@rows, $fields);
		}		
		next;
	    }
	    else {
		if ($longGraph) {
		    $pre .= $s . (' ' x $graphWidth);
		}
	    }

	    if ($method =~ /^deliver$/) {
		$str .= "$method $src -> $message";
		if ($graph && length($str . $messageFields) > $dwidth) {
		    if ($longGraph) {
			$str .= compressFields($messageFields);
# 			my $comma = rindex($str . $messageFields, ",", $dwidth - 1);
# 			push(@rows, $pre . substr($str . $messageFields, 0, $comma + 1));
# 			$str = (' ' x LONG_INDENT) . substr($messageFields, $comma - length($str) + 1);
		    }
		    else {
			push(@rows, $pre . substr($str, 0, $dwidth));
			$str = $messageFields;
		    }
		}
		elsif ($longGraph) {
		    $str .= compressFields($messageFields);
		}
		else {
		    $str .= $messageFields;
		}
	    }
	    elsif ($method) {
		$str .= $method;
		if ($graph && length($str . $detail) > $dwidth) {
		    push(@rows, $pre . substr($str, 0, $dwidth));
		    $str = $detail;
		}
		else {
		    $str .= $detail;
		}
	    }

	    if ($graph) {
		$str = substr($str, 0, $dwidth);
	    }
	    else {
		$str = substr($str, 0, $termWidth - SINGLE_INDENT);
	    }

	    $s = "";
	    push(@rows, $pre . $str);
	}
    }
    if (!$graph) {
	push(@rows, "----PATH----");
	my $i = shift(@pathInputs);
	unless (defined $i) {
	    $i = "?";
	}
	push(@rows, ccolor($i, SEQUENCE_COLOR, $termWidth - 10));
    }
}

if (!$graph) {
    print join("\n", @rows) . "\n";
    exit(0);
}

my $fh = new IO::File();
if ($outfile) {
    $fh->open(">$outfile") or confess "cannot write to $outfile: $! $@\n";
}
else {
    $fh->fdopen(fileno(STDOUT), "w");
}

if ($propertyCheck) {
    print $fh "$propertyCheck\n";
}

# print ' ' x $stepWidth;
# for my $i (sort(keys(%nodes))) {
#     print ' ' x ($halfNodeWidth - length($i)), $i, ' ' x ($nodeWidth - $halfNodeWidth);
# }
# print "\n";

print $fh join("\n", @rows) . "\n";

#while (my ($k, $el) = each(%netEdges)) {
#    if (scalar(keys(%$el)) == 2) {
#	$g->add_edge($el->{src} => $el->{dest});
#    }
#}

# $g->as_canon("graph.dot");
#$g->as_ps("graph.ps");

# printCols();

# for my $event (@events) {
#     print 
# }

# for my $el (@events) {
#     print($el->{step} . " " . $el->{id} . " " . $el->{node} . " " . $el->{type} . " " .
# 	  $el->{message} . "\n");
#     if (defined($el->{transitions})) {
# 	my $ref = $el->{transitions};
# 	for my $t (@$ref) {
# 	    print("\t" . $t->{src} . " -> " . $t->{dest} . " " . $t->{method} . " " .
# 		  $t->{message} . "\n");
# 	}
#     }
# }

sub processLine {
    my $l = shift;
    my $service = "";
    my $method = "";
    my $message = "";
    my $messageFields = "";
    my $detail = "";
    my $direction = "";
    my $src = "";
    my $dest = "";
    my $sim = 0;
    my $drop = 0;

    $l =~ s|SHA160/([0-9a-fA-F]{6})([0-9a-fA-F]{34})|SHA/$1..|g;
    $l =~ s|SHA32/|SHA/|g;
    $l =~ s|STRING/|STR/|g;
    $l =~ s|\btrue\b|1|g;
    $l =~ s|\bfalse\b|0|g;

    if ($l =~ m/^(\S+) .*?\[(TRACE|COMPILER|SIMULATOR|MASSAGE)::(\w+)::(multicast|route|routeRTS|send)::(\w+)::\(downcall\)\] \4(.*)/) {

	$src = $1;
	my $which = $2;
	$service = $3;
	$method = $4;
	$detail = $6;
        if ($5 ne "Message") {
            $message = $5;
        }
        elsif ($detail =~ m/\[(s|msg)_deserialized=(\w+)(.*?)\]/) {
            $message = $2;
        }
        else {
            $message = "Message";
        }

#	$detail =~ m/\[\w+=(.*?)\]\[(s|msg)_deserialized=$message(.*)\]/;
	$detail =~ m/\[dest=(.*?)\].*?\[(s|msg)_deserialized=$message(.*)\]/;
	$dest = $1;
	$messageFields = $3;
	if ($detail =~ m|\[src=(.*?)\]|) {
	    $src = $1;
	}

	$direction = "downcall";
	if ($which eq "SIMULATOR") {
	    $sim = 1;
	    if ($messageFields =~ m|SIM_DROP=1|) {
		$drop = 1;
	    }
	}

    }
    elsif ($l =~ m/^(\S+) .*?\[(TRACE|COMPILER|SIMULATOR|MASSAGE)::(\w+)::(multicast|route|routeRTS|send)::\(downcall\)\] \4(.*)/) {

	$src = $1;
	my $which = $2;
	$service = $3;
	$method = $4;
	$detail = $5;
        if ($detail =~ m/\[s=(\w+)(.*?)\]/) {
            $message = $1;
        }
        else {
            $message = "Message";
        }

#	$detail =~ m/\[\w+=(.*?)\]\[(s|msg)_deserialized=$message(.*)\]/;
	$detail =~ m/\[dest=(.*?)\].*?\[s=$message(.*)\]/;
	$dest = $1;
	$messageFields = $2;
	if ($detail =~ m|\[src=(.*?)\]|) {
	    $src = $1;
	}
	
	$direction = "downcall";
	if ($which eq "SIMULATOR") {
	    $sim = 1;
	    if ($messageFields =~ m|SIM_DROP=1|) {
		$drop = 1;
	    }
	}
	
    }

    elsif ($l =~ m/\[(TRACE|COMPILER)::(\w+)::(deliver)::(\w+)::\(demux\)\] deliver(.*)/) {
	$service = $2;
	$method = $3;
	$message = $4;
	$detail = $5;

	if ($detail =~ m/\[\w+=(.*?)\]\[\w+=(.*?)\]\[s_deserialized=$message(.*)\]/) {
            $src = $1;
            $dest = $2;
            $messageFields = $3;
        }
        elsif ($detail =~ m/\[src=(.*?)\]\[dest=(.*?)\].*?\[(s|msg)_deserialized=$message(.*)\]/) {
            $src = $1;
            $dest = $2;
            $messageFields = $4;
        }
	$direction = "upcall";
    }
    elsif ($l =~ m/\[(TRACE|COMPILER)::(\w+)::expire_(\w+)::\(demux\)\] expire_\3(.*)/) {
	$service = $2;
	$method = $3;
	$detail = $4;
	$direction = "scheduler";
    }
    elsif ($l =~ m/\[(TRACE|COMPILER)::(\w+)::(\w+)::\(upcall\)\] \3(.*)/) {
	$service = $2;
	$method = $3;
	$detail = $4;
	$direction = "upcall";
    }
    elsif ($l =~ m/\[(TRACE|COMPILER)::(\w+)::(\w+)::\(downcall\)\] \3(.*)/) {
	$service = $2;
	$method = $3;
	$detail = $4;
	$direction = "downcall";
    }
    elsif ($l =~ m/\[(ERROR|WARNING)::(\w+::\w+)::.*\] (.*)/) {
	$service = $1;
	$method = $2;
	$detail = " $3";
	$direction = "error";
    }
    elsif ($l =~ m/\[(ERROR|WARNING)\] (.*)/) {
	$service = $1;
	$detail = $2;
	$direction = "error";
    }

    my @skip = qw(forward localAddress);
    if (grep(/$method/, @skip)) {
	return 0;
    }

    if ($method eq "deliver" && !$src) {
	return 0;
    }

    if ($messageFields) {
	$messageFields =~ s|\]\[rid=-?\d+$||;
    }

    if ($service) {
# 	print("returing src=$src dest=$dest service=$service method=$method " .
# 	      "message=$message messageFields=$messageFields detail=$detail " .
# 	      "direction=$direction sim=$sim drop=$drop\n");
# 	print "$l\n";

	
	my $r = {
	    src => $src,
	    dest => $dest,
	    service => $service,
	    method => $method,
 	    message => $message,
	    messageFields => $messageFields,
	    detail => $detail,
	    direction => $direction,
	    sim => $sim,
	    drop => $drop,
	};
	return $r;
    }
    else {
	return 0;
    }
} # processLine

sub printCols {
    my $cols = 176;
    for my $i (1..int($cols / 100)) {
	print ' ' x 99, $i % 10;
    }
    print "\n";
    for my $i (1..int($cols / 10)) {
	print ' ' x 9, $i % 10;
    }
    print "\n";
    for my $i (1..$cols) {
	print $i % 10;
    }
    print "\n";
}

sub ccolor {
    my $r = "";
    my $len = 0;
    my $sslen = 0;
    if (scalar(@_) % 2) {
	$sslen = pop(@_);
    }
    while (defined(my $t = shift(@_))) {
	my $c = shift(@_);
	if ($sslen && (scalar(@_) == 0)) {
	    if ($len >= $sslen) {
		$t = "";
	    }
	    else {
		if ($longGraph) {
		    $t = substr($t, 0, $sslen - $len);
		}
	    }
	}
	if ($uncolored) {
	    $r .= $t;
	}
	else {
	    $r .= colored($t, $c);
	}
    }
    return $r;
}

sub compressFields {
    my $s = shift;
#     $s =~ s|true|1|g;
#     $s =~ s|false|0|g;
    $s =~ s|,||g;

    return $s;
#     my @ch = split(//, $s);
#     my $r = "";

#     my $drop = 0;
#     my $val = 0;
#     for my $c (@ch) {
# 	if ($c =~ m|\W|) {
# 	    $drop = 0;
# 	    $val = 0;
# 	}
# 	if ($c eq "=") {
# 	    $val = 1;
# 	}
# 	if (!$val && $drop) {
# 	    next;
# 	}
# 	$r .= $c;
# 	if ($c =~ m|\w|) {
# 	    $drop = 1;
# 	}
#     }

#     return $r;


#     my @fields = split(/,/, $s);
#     my @r = ();
#     for my $f (@fields) {
# 	$f = trim($f);
#  	my $first = substr($f, 0, 1);
# 	print "f=$f first=$first\n";
# 	push(@r, $first);
# # 	push(@r, $f);
#     }
#     return join(" ", @r);
#     return join(" ", map {
# 	substr($_, 0, 1)
# # 	my $rest = substr($_, 1);
# # 	$rest =~ tr/a-z_//d;
# #         return $first . $rest;
#     } @fields);
}

sub readFormattedInput {
    my $step = 0;
    my %ips = ();
    my %lastsent = ();
    my $assignNodes = 1;

    my $prevIp = 0;
    my $prevClk = 0;
    my @trans = ();
    my $mstr = "";

    while (my $l = <>) {
	chomp($l);

	if ($l =~ m|^\d+\.\d+|) {
	    $ips{$l} = 1;
	    next;
	}
	elsif ($assignNodes) {
	    $assignNodes = 0;
	    my $n = 0;
	    for my $el (sort(keys(%ips))) {
		$nodes{$n} = $el;
		$rnodes{$el} = $n;
		$n++;
	    }
	}

	my @fields = split("\t", $l);
	my $logicalClock = shift(@fields);
	my $sendclk = shift(@fields);
	my $ts = shift(@fields);
	my $ip = shift(@fields);
	if ($prevIp eq "0") {
	    $prevIp = $ip;
	    $prevClk = $logicalClock;
	}

	if (!($ip eq $prevIp && $logicalClock == $prevClk)) {
	    my @copy = @trans;
	    push(@events, {
		step => $step,
		ip => $prevIp,
		type => "NET",
		message => $mstr,
		transitions => \@copy,
		 });
# 	    my $e = $events[-1];
# 	    my $tr = $e->{transitions};
# 	    use Data::Dumper;
# 	    print Dumper($tr) . "\n";
	    $step++;
	    $prevIp = $ip;
	    $prevClk = $logicalClock;
	    $mstr = "";
	    @trans = ();
	}

	my $service = shift(@fields);
	my $method = shift(@fields);
	my $src = shift(@fields);
	if ($src eq "None") {
	    $src = $ip;
	}
	my $dest = shift(@fields);
	my $direction = "downcall";
	if ($method eq "deliver") {
	    $direction = "upcall";
	}
	if (1) {
	    $src =~ s|:\d+||;
	    $dest =~ s|:\d+||;
	}

	my $msg = shift(@fields);
	my $message = "";
	my $messageFields = "";
	if ($msg =~ m|^(\w+)\(|) {
	    $message = $1;
	    $messageFields = substr($msg, length($message));
	}

	$ip =~ m|^(\d+\.\d+)|;
	my $subnet = $1;

	my $emessage = ("$src -> $dest (" .
			$rnodes{$src} . " -> " . $rnodes{$dest} . ")");
	if ($method eq "deliver") {
	    $emessage = "$dest <- $src";
	    my $senttime = 0;
	    my $sentstep = undef;
	    $src =~ m|^(\d+\.\d+)|;
	    my $srcsubnet = $1;

	    if (defined($lastsent{$subnet . $msg})) {
		($senttime, $sentstep) = @{$lastsent{$subnet . $msg}};
	    }
	    elsif (defined($lastsent{$srcsubnet . $msg})) {
		($senttime, $sentstep) = @{$lastsent{$srcsubnet . $msg}};
	    }
	    else {
		print "could not find sent for $subnet$msg or $srcsubnet$msg\n";
	    }
	    if (defined($sentstep)) {
		my $diff = $ts - $senttime;
		$diff *= 1000;
		$diff = int($diff);
		$emessage .= " $diff ms (sent $sentstep)";
	    }
	}
	else {
# 	    print "adding lastsent for $subnet$msg\n";
	    $lastsent{$subnet . $msg} = [$ts, $step];
	}

	if (!$mstr) {
	    $mstr = "$ts $emessage";
	}
# 	print "adding trans $message $messageFields\n";
	push(@trans, {
		detail => "",
		src => $src,
		dest => $dest,
		service => $service,
		method => $method,
		message => $message,
		messageFields => $messageFields,
		ts => $ts,
		direction => $direction,
		sim => 0,
		drop => 0,
	     });
    }
} # readFormattedInput

sub readMCInput {
    my $count = 0;
    while (my $l = <>) {
	$count++;
	chomp($l);
	if ($l =~ m|\[main\] Starting simulation|) {
	    $startLog = 1;
	}
	if ($l =~ m|\[NodeAddress\] Node (\d+) address (\S+)|) {
	    $nodes{$1} = $2;
	    $rnodes{$2} = $1;
	}
	if ($l =~ m|\[Sim::pathComplete\] .* search sequence \[ (.+) \]|) {
	    if (!$graph) {
		print STDERR ".";
	    }
	    push(@pathInputs, $1);
	}
	if ($startLog) {
	    if ($seBegin) {
		if ($l =~ m|^(\S+) .*?\[SimulateEventBegin\] (\d+) (\d+) (\w+)( \d+\.\d+)?|) {
                    if ($2 % 100 == 0) {
                      print STDERR "Reading step $2\n";
                    }
		    $seBegin = 0;
		    my $id = $1;
		    my $step = $2;
		    my $node = $3;
		    my $type = $4;
		    my $time = $5;
		    if (defined $time and $starttime == 0) {
			$starttime = $time;
		    }
		    if (defined $time and $starttime != 0) {
			$time = $time - $starttime;
		    }
#		$nodes{$node} = $id;
#		$rnodes{$id} = $node;
		    push(@events, { step => $step,
				    id => $id,
				    node => $node,
				    type => $type,
				    tme =>  $time,
			 } );
		}
		elsif ($l =~ m/\] (Liveness|Safety) property (.+: failed)/) {
# 		   &&
# 		   $l !~ m|ERROR::stoppingCondition| &&
# 		   $l !~ m|\[ERROR\]|) {
		    $propertyCheck = "$1 property $2";
# 		print "**** setting property check to $propertyCheck\nl=$l\n"
		}
		elsif ($l =~ m/(Liveness|Safety) Properties: check/) {
		    $propertyCheck = "$1 Properties: check";
		}
	    }
	    else {
		if ($l =~ m|^(\S+) .*?\[SimulateEventEnd\] (\d+) (\d+) (\w+) (.*)|) {
		    $seBegin = 1;
		    $events[-1]->{message} = $5;
		}
		elsif ($l =~ m|^(Assert.*failed)|) {
		    $events[-1]->{message} = $1;
		    $propertyCheck .= " - $1";
		}
		else {
		    my $r = processLine($l);
		    if (ref($r)) {
			push(@{$events[-1]->{transitions}}, $r);
		    }
		}
	    }
	}
    }

} # readMCInput
