#!/usr/bin/env perl

@skipnames = (
	"Queue::getOrBlock",
	"SQueueBase::getOrBlock",
	"__1cFQdDueueKgetOrBlock6MI_pv_",
	"ConditionWait",
	"__1cNConditionWait6FpnSnetxms_condition_t_I_b_",
	"pthread_cond_reltimedwait_np",
	"pthread_cond_wait",
        "SleepAndCheckForShutdown",
	"ThreadSleep",
	"ThreadSleepMs",
	"poll",
	"select",
	"accept",
	"AgentTunnelCommChannel::recv",
	"_p_nsleep",
	"_event_sleep",
	"__fd_select"
);

my %skiplist;
@skiplist{@skipnames} = ();

@data = ();
$ignore = 1;

while(<STDIN>)
{
   chomp;
   $line = $_;

   if (($line =~ /^Thread .*/) || ($line =~ /^-+  lwp# [0-9]+.*/) || ($line =~ /^---------- tid# [0-9]+.*/))
   {
      if ($ignore == 0)
      {
         for($i = 0; $i < $index; $i++)
         {
            print $data[$i] . "\n";
         }
      }
      $ignore = 0;
      @data = ();
      $index = 0;
      $func = "";
   }
   elsif ($line =~ /^#[0-9]+\s+[0-9a-fx]+\s+in\s+([^ ]+)\s.*/)
   {
      $func = $1;
   }
   elsif ($line =~ /^#[0-9]+\s+([^ ]+)\s.*/)
   {
      $func = $1;
   }
   elsif ($line =~ /^ [0-9a-f]+ ([^ ]+)\s.*/)
   {
      $func = $1;
   }
   elsif ($line =~ /^0x[0-9a-f]+  ([^(]+)\(.*/)
   {
      $func = $1;
   }
   
   if ($func =~ /^([^@(]+)[@(].*$/)
   {
      $func = $1;
   }

   $data[$index++] = $line;
   if (exists $skiplist{$func})
   {
      $ignore = 1;
   }
}
