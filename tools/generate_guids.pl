#!/usr/bin/perl

use Data::UUID;

while(<STDIN>)
{
   chomp;
   $line = $_;

   if ($line =~ /^(.*)____guid____(.*)$/)
   {
      $gen = Data::UUID->new;
      $guid = $gen->create();
      print $1 . $gen->to_string($guid) . $2 . "\n";
   }
   else
   {
      print $line . "\n";
   }
}
