#!/usr/bin/perl -w 

use strict;

my $nodes_per_host = 4;
my $num_threads = $ARGV[0] || 32;
my $sleep_time = 100000;
my $interval = 24000000;
my $num_messages = 10;
my $logging = 0;
my $downgrade_to_none = 0;

my $current_time = (time()+10);
print "Running nodes_per $nodes_per_host num_threads $num_threads current_time $current_time downgrade_to_none $downgrade_to_none";
system(qq{echo "Running nodes_per $nodes_per_host num_threads $num_threads current_time $current_time downgrade_to_none $downgrade_to_none" >> runs.txt});

system(qq{python generatep.py $nodes_per_host $num_threads $num_messages $current_time $logging $downgrade_to_none > /homes/ckillian/groupstream/run_${nodes_per_host}_${num_threads}.sh});
system(qq{chmod 777 /homes/ckillian/groupstream/run_${nodes_per_host}_${num_threads}.sh});
# cd /homes/ckillian/groupstream/
exec("python /homes/ckillian/src/pssh-1.4.3/bin/pssh -v -P -t 700 -h /homes/ckillian/groupstream/hosts.pssh /homes/ckillian/groupstream/run_${nodes_per_host}_${num_threads}.sh $sleep_time $interval");
#pkill groupstream
