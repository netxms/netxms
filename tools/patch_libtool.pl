#!/usr/bin/perl

while(<>)
{
   chomp;
   $line = $_;
   if ($line =~ /^postdeps\=.*/)
   {
      $line = "postdeps=\"\"";
   }
   $line =~ s/-lc_r/-lc_rXXX/;
   print "$line\n";
}

exit 0;
