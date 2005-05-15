#!perl

use strict;
use warnings;

###############################################################################
# globals
my @quotes;

###############################################################################
# config
my $quotesFile = "../netxms.quotes";

###############################################################################
# code
print "Content-type: text/html\n\n";

open(F, $quotesFile) || exit 0;

while (<F>)
{
	chomp;

	push @quotes, $_;
}

print @quotes[int(rand($#quotes+1))];