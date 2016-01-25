#!/usr/bin/perl

@skipnames = ("Queue::getOrBlock", "ConditionWait", "poll", "select", "accept");

my %skiplist;
@skiplist{@skipnames} = ();

@data = ();
$ignore = 1;

while(<STDIN>)
{
   chomp;
   $line = $_;

   if ($line =~ /^(Thread .*)/)
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
   $data[$index++] = $line;
   if (exists $skiplist{$func})
   {
      $ignore = 1;
   }
}
