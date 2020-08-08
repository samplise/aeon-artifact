#!/usr/bin/perl -w
# 
# mcsched.pl : part of the Mace toolkit for building distributed systems
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

use FindBin;
use lib "$FindBin::Bin";
use POSIX qw(setsid);
use IO::Handle;
use Getopt::Long;
use Cwd;
use SimulatorScriptsCommon;

use constant MESSAGE_LOG => "mcsched.log";
use constant ERROR_LOG => "mcsched_error.log";

my $dirpath = ".";
my $foreground = 0;

GetOptions("dirpath=s" => \$dirpath,
	   "foreground" => \$foreground);

unless (-d $dirpath) {
    system("mkdir -p $dirpath") or die "cannot mkdir $dirpath: $!";
}

my $cwd = getcwd();
my $simulator = findSimulator();

unless ($foreground) {
    daemonize();
}

my @runs = ();

while (1) {
    opendir(DIR, ".");
    my @t = grep(/\.conf/, readdir(DIR));
    for my $el (@t) {
	if (!grep(/$el/, @runs)) {
	    push(@runs, $el);
	}
    }
    closedir(DIR);

    while (my $f = shift(@runs)) {
	if (not -e $f) {
	    warn "no such file $f\n";
	    next;
	}
	$f =~ m|(.*)\.conf|;
	my $d = "$dirpath/$1";
	mkdir($d) or die "cannot mkdir $d: $!";
	system("mv -v $f $d");
	chdir($d) or die "cannot chdir $d: $!";
	my $pid = fork();
	if ($pid) {
	    waitpid($pid, 0);
	}
	else {
	    print "running $f\n";
	    exec("$simulator $f | tee out.log");
	}
	print "completed $f\n";
	chdir($cwd);
    }

    sleep(1);
}

sub daemonize {
#     chdir('/') or die "Can't chdir to /: $!\n";
    open(STDIN, '/dev/null') or die "cannot read /dev/null: $!\n";
    open(STDOUT, '>>/dev/null') or die "cannot write to /dev/null: $!\n";
    open(STDERR, '>>/dev/null') or die "cannot write to /dev/null: $!\n";
    defined(my $pid = fork) or die "cannot fork: $!\n";
    if ($pid) {
        exit(0);
    }
    setsid() or die "Can't start a new session: $!\n";

    open(STDERR, ">@{[ERROR_LOG]}") or die "cannot write to @{[ERROR_LOG]}: $!\n";
    open(STDOUT, ">@{[MESSAGE_LOG]}") or die "cannot write to @{[MESSAGE_LOG]}: $!\n";

#     umask(0);
} # daemonize
