#!/usr/bin/perl
use strict;
use Getopt::Long;
# Mace relies on libboost in addition to g++. Libboost relies on glibc, so it's a better idea to build boost using the local build of gcc.

my $mirror_rooturl="http://sourceforge.net/projects/boost/files/boost";
my $parallel_build = 2;
my $boost_ver = "1.53.0";
my $gcc_ver = "gcc-4.8.0";
my $build_type = ""; # possible options: debug, relwithbuginfo, release, minsizerel or empty string for default build.
my $gcc_dir="/scratch/chuangw/opt";

GetOptions("parallel_build=i" => \$parallel_build,
           "gccver=s" => \$gcc_ver,
           "boostver=s" => \$boost_ver,
           "dir=s" => \$gcc_dir,
           "mirror=s" => \$mirror_rooturl,
       );
my $second_dir = $boost_ver;
$second_dir =~ s/\./_/g;
system("wget $mirror_rooturl/$boost_ver/boost_$second_dir.tar.gz/download");
rename "download", "boost_$second_dir.tar.gz";
system("tar zxf boost_$second_dir.tar.gz");
chdir "boost_$second_dir" or die "can't enter the source directory";
chdir "tools/build/v2/";
system("./bootstrap.sh");
system("./b2 install --prefix=/scratch/chuangw/opt/Boost.Build");
# TODO: add PREFIX/bin to PATH environment variable

chdir "../../..";
open CONFIG_FILE, ">", "/tmp/boost-config.tmp";
print CONFIG_FILE "using gcc : 4.8.0 : g++ : root=/scratch/chuangw/opt/gcc-4.8.0/bin ;"
close CONFIG_FILE;
system("b2 --ser-config=/tmp/boost-config.tmp --build-dir=/scratch/chuangw/opt/libboost_$second_dir  toolset=gcc stage");


#system("./bootstrap.sh --prefix=/scratch/chuangw/opt/libboost_$second_dir");
#system("./b2");
