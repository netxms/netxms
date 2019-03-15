#!/usr/bin/env perl

my $out;

while(<STDIN>)
{
   chomp;
   $line = $_;

   if ($line =~ /^[ \t]*([A-Za-z0-9_-]+) DEFINITIONS/)
   {
      if (defined $out)
      {
         close $out;
      }

      my $fname = $1 . ".txt";
      open($out, ">", $fname) || die "Cannot open $fname";
      print "Writing $fname\n";
   }

   print $out "$line\n";
}

if (defined $out)
{
   close $out;
}
