#!/usr/bin/perl

# chuangw: by default, this script runs for all versions of gcc in $gcc_dir.
# to change the default, appen parameters: e.g. -ver=gcc-4.5.1 or -v for short
# also, you can also change the gcc build directory by -dir=/scratch/chuangw/blah or -d for short
# Finall, the number of parallel builds can be change by e.g. -parallel_build=8 or -p for short

use strict;
use Getopt::Long;

my $gcc_dir="/scratch/chuangw/opt";
my $parallel_build = 4;
my $gcc_ver = "all";
my $build_type = ""; # possible options: debug, relwithbuginfo, release, minsizerel or empty string for default build.

GetOptions("parallel_build=i" => \$parallel_build,
           "ver=s" => \$gcc_ver,
           "dir=s" => \$gcc_dir,
           "buildtype=s" => \$build_type,
       );


if( $gcc_ver eq "all" ){

  my $all_ver_str = `find $gcc_dir/* -maxdepth 0 -type d`;
  my @all_ver = split ("\n", $all_ver_str);
  foreach my $ver ( @all_ver ) {

    test_ver( $ver );
  }
}else{
    test_ver( "$gcc_dir/$gcc_ver" );
}
sub test_ver {
  my $ver = shift;
  print "Testing version $ver\n";

  system("$ver/bin/g++ -v");
  my $v = $ver;
  $v =~ s/(.*)gcc-(.*)/$2/;
  my $build_dir="build-$v";
  if( not -e $build_dir ){
    mkdir($build_dir) or die("can't create the dir '$build_dir'");
  }
  chdir($build_dir) or die("can't enter the dir '$build_dir'");

  system("cmake -D CMAKE_CXX_COMPILER=$ver/bin/g++ -D CMAKE_BUILD_TYPE=$build_type ../mace-fullcontext 2>&1 "); #== 0 or die "failed to configure makefiles";
  system("time make -j $parallel_build 2>&1 "); #== 0 or die "failed to make all services";
  $ENV{LD_LIBRARY_PATH} = "$ver/lib64";
  my $test_ret = Timed::timed("make test 2>&1", 100);
  if( $test_ret != 0 ){
    print "time out. give up the tests...\n";
  }else{
  }
  $ENV{LD_LIBRARY_PATH} = "";
  chdir("..") or die "can't enter the parent directory ";
}

################################################################################
# copied and modified from http://stackoverflow.com/questions/1962985/how-can-i-timeout-a-forked-process-that-might-hang
package Timed;
use strict;
use warnings;

sub timed {
  my $retval;
  my ($program, $num_secs_to_timeout) = @_;
  my $pid = fork;
  if ($pid > 0){ # parent process
    eval{
      local $SIG{ALRM} = sub {kill 9, -$pid; print STDOUT "TIME OUT!$/"; $retval = 124;};
      alarm $num_secs_to_timeout;
      waitpid($pid, 0);
      alarm 0;
    };
    return defined($retval) ? $retval : $?>>8;
  }
  elsif ($pid == 0){ # child process
    setpgrp(0,0);
    exec($program);
  } else { # forking not successful

  }
}

