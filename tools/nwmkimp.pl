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
my $j;

open(OUT, ">$outFile") || die "out: $!";

my @sym_list = `i586-netware-nm $inFile | grep -e " [BGTRD] " | cut -d" " -f3`;
my @exp_list;

for ($i = 0, $j = 0; $i <= $#sym_list; $i++) 
{ 
	chomp $sym_list[$i]; 
	if ($sym_list[$i] =~ /^main$|^_init$|^_fini$|^__EH_FRAME.*|^g_.*|^I_.*/)
	{
	}
	else
	{
		$exp_list[$j++] = $sym_list[$i];
	}
} 

for ($i = 0; $i < $#exp_list; $i++) 
{ 
	print OUT " $exp_list[$i],\n"; 
} 
print OUT " $exp_list[$i]\n";

close OUT;
exit 0;
