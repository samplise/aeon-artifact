#!/usr/bin/perl

my $maxcontexts = 512;
my $ncontexts = 1;

while( $ncontexts <= $maxcontexts ){
  my $topcontexts = 1;
  while( $topcontexts <= $ncontexts ){
    my $lowercontexts = $ncontexts / $topcontexts;
    my $output = `./testrun ../../../tools/contextlattice/params.test2 -ServiceConfig.TestCase2.NCONTEXTS $topcontexts -ServiceConfig.TestCase2.NSUBCONTEXTS $lowercontexts`;
    my @output_lines = split(/\n/,$output);
    
    my $first_commit_count;
    my $first_commit_time;
    my $last_commit_count;
    my $last_commit_time;
    foreach my $line (@output_lines) {
      if ( $line =~ /Accumulator::EVENT_COMMIT_COUNT/ ){
        my @log_fields = split(/ /, $line );
        if( not defined $first_commit_count ){
          $first_commit_time = int( $log_fields[0]);
          $first_commit_count = $log_fields[3];
        }
        $last_commit_time = int( $log_fields[0]);
        $last_commit_count = $log_fields[3];
      }
    }
    my $avg_throughput =  ($last_commit_count - $first_commit_count) / ( $last_commit_time-$first_commit_time );
    print "total= $ncontexts top= $topcontexts bottom= $lowercontexts time_span= " . ( $last_commit_time-$first_commit_time ) . " total_committed= " . ($last_commit_count - $first_commit_count) . " avg_throughput= $avg_throughput\n";

    $topcontexts *=2;
    sleep(1);
  }
  $ncontexts *=2 ;
}
