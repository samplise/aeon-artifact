#!/usr/bin/perl
# download the latest revision in the repository ('default' branch by default)
# and compile the code using different version of gcc
# and mail the result
# usage:
#   add a cron entry ( crontab -e )
# #MIN HOUR DAY MONTH DAYOFWEEK   COMMAND
# 0 4 * * * /scratch/chuangw/mace-fullcontext/tools/gcc_test/nightlybuild.pl


use strict;
use Getopt::Long;
use File::Path qw(remove_tree);
use POSIX qw(strftime);

my $repo_url = "ssh://hg\@bitbucket.org/chuangw/mace-fullcontext";
my $branch_name = "default"; # test default branch
my $prefix_dir = "/scratch/chuangw/nightlybuild"; # this is where the test code is placed
my $report_dir;
my $gcc_dir="/scratch/chuangw/opt";
if( -e $ENV{"HOME"} ){
  $report_dir = $ENV{"HOME"} . "/.www/contextlattice";
}
GetOptions(
  "repo=s" => \$repo_url,
  "branch=s" => \$branch_name,
  "prefix=s" => \$prefix_dir,
  "log" => \$report_dir,
  "dir=s" => \$gcc_dir,
  );

mkdir $prefix_dir;
chdir $prefix_dir or die "can't enter the directory $prefix_dir";
remove_tree("mace-fullcontext");
system("hg clone -b $branch_name $repo_url");
chdir "mace-fullcontext" or die "can't enter the directory mace-fullcontext";
my $latest_revision_id;
my $latest_revision_hash;
chomp( $latest_revision_hash = `hg id -i`); # revision hash id
chomp( $latest_revision_id= `hg id -n`); # local, numerical revision id
if( not -e $report_dir ){
  mkdir "$report_dir" or die "can't create directory $report_dir";
  chmod 0755, "$report_dir";
}
if( not -e "$report_dir/$latest_revision_id" ){
  mkdir "$report_dir/$latest_revision_id" or die "can't create directory $report_dir/$latest_revision_id";
  chmod 0755, "$report_dir/$latest_revision_id";
}
my $all_ver_str = `find $gcc_dir/* -maxdepth 0 -type d`;
my @all_ver_path = split ("\n", $all_ver_str);
my @all_ver;
foreach my $ver ( @all_ver_path ) {
  my $v = $ver;
  $v =~ s/(.*)gcc-(.*)/$2/;
  push @all_ver, $v;
}
my $now_string = strftime "%a %b %e %H:%M:%S %Y", localtime;
my $log_index = "$report_dir/index.html";
open( INDEXFILE, '>>', $log_index );
chmod 0755, $log_index;
print INDEXFILE <<EOF;
<p>$now_string Revision $latest_revision_id ($latest_revision_hash)
  <table>
    <tr>
EOF
foreach my $ver ( @all_ver ) {
  print INDEXFILE "<td><a href='$latest_revision_id/$latest_revision_id-$ver.log'>$ver</a></td>\n";
}
print INDEXFILE <<EOF;
    </tr>
  </table>
</p>
EOF
close INDEXFILE;

chdir ".." or die "can't enter the parent directory of the repository";

foreach my $ver ( @all_ver ) {
  my $log_filename = "$report_dir/$latest_revision_id/$latest_revision_id-$ver.log";
  open( LOGFILE, '>', $log_filename );
  print "log is available at $log_filename\n";
  chmod 0755, $log_filename;
  open(my $compilemace, '-|', 'mace-fullcontext/tools/gcc_test/compilemace.pl', '-p', '1', '-b', 'relwithdebinfo', '-v', "gcc-$ver") or die "can't open pipe to start compiling mace"; # test all available     gcc versions, relwithdebinfo (-O2 -g) build
  while( my $line = <$compilemace> ){
    print LOGFILE $line;
    #print ".";
    print $line;
  }
  close LOGFILE;
  close $compilemace;

  #open( INDEXFILE, '>>', $log_index ); # update report index  file
  #print INDEXFILE <<EOF;
#EOF;
  #close INDEXFILE;
}

