#!c:/apps/perl/bin/pelr -w

###############################################################################
#
# Create import file for NLM based on i586-netware-nm.exe output
#
###############################################################################

use strict;

my $inFile = shift || die "Usage : nxmkimp.pl <input_file> <output_file>";
my $outFile = shift || die "Usage : nxmkimp.pl <input_file> <output_file>";

my $i;

open(OUT, ">$outFile") || die "out: $!";

my @a = `i586-netware-nm $inFile | grep -e " [BGTRD] " | cut -d" " -f3`;
for ($i = 0; $i < $#a; $i++) 
{ 
	chomp $a[$i]; 
	print OUT " $a[$i],\n"; 
} 
print OUT " $a[$i]";

close OUT;
exit 0;
