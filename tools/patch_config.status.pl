#!/usr/bin/perl

while(<STDIN>)
{
   chomp;
   $line = $_;

	if ($line =~ /\"\$PRIVATE\"\) CONFIG_FILES/)
	{
		for($i = 0; $i <= $#ARGV; $i++)
		{
			print "\t\"" . $ARGV[$i]. "\") CONFIG_FILES=\"\$CONFIG_FILES " . $ARGV[$i] . "\" ;;\n";
		}
		$line = "";
	}

   if ($line ne "")
   {
      print "$line\n";
   }
}
