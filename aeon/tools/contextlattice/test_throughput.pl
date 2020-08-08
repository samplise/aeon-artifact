#!/usr/bin/perl

my @maxprimes = (10, 20, 50, 100 );
my @ncontexts = (1, 2, 4, 6, 8);

my $debug = 0;

foreach my $maxprime ( @maxprimes ){
  foreach my $ncontext ( @ncontexts ){
    my $nevents = 128 * $ncontext;
    my $exec = "./testcase ../../../tools/contextlattice/params.test6 -run_time 100 -ServiceConfig.TestCase6.NCONTEXT $ncontext -ServiceConfig.TestCase6.NEVENT $nevents -ServiceConfig.TestCase6.MAX_PRIME $maxprime";
    if( $debug == 1 ){
      print $exec . "\n";
      next;
    }
    my $output = `$exec`;
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
    print "first_commit_count = $first_commit_count, first_commit_time = $first_commit_time, last_commit_count = $last_commit_count, last_commit_time = $last_commit_time\n";
    my $avg_throughput =  ($last_commit_count - $first_commit_count) / ( $last_commit_time-$first_commit_time );
    print "ncontext= $ncontext maxprime= $maxprime time_span= " . ( $last_commit_time-$first_commit_time ) . " total_committed= " . ($last_commit_count - $first_commit_count) . " avg_throughput= $avg_throughput\n";

    sleep(1);
  }
}

