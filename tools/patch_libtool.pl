#!/usr/bin/perl

while(<>)
{
   chomp;
   $line = $_;
   if ($line =~ /^postdeps\=.*/)
   {
      $line = "postdeps=\"\"";
   }
   print "$line\n";
}

exit 0;
