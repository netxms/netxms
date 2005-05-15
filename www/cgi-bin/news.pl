#!/usr/bin/perl -w

use strict;
use warnings;

use CGI;

###############################################################################
# globals
my @news;

###############################################################################
# config
my $newsFile = "../netxms.news";

###############################################################################
# code
print "Content-type: text/html\n\n";

my $q = new CGI;
my $cut = 0;
if (defined $q->param("cut"))
{
	$cut = 1;
}

open(F, $newsFile) || exit 0;

while (<F>)
{
	chomp;
	if (/^(.*)?\|(.*)?\|(.*)$/)
	{
		my $date = $1;
		my $title = $2;
		my $summary =  $3;
		if ($cut != 0 && length($summary) > 100)
		{
			$summary = substr($summary, 0, 200);
			$summary =~ s/(.*) .[^ ]*$/$1/g;
			$summary .= "&#133;"
		}
		push @news, { date => $date, title => $title, sum => $summary };
	}
}

my $count = $#news + 1;
if (defined $q->param("count"))
{
	$count = $q->param("count") + 0;
}

for (my $i = 0; $i < $count; $i++)
{
	my $date = $news[$i]->{date};
	my $title = $news[$i]->{title};
	my $summary = $news[$i]->{sum};
	print <<"__END";
<a href="/news.shtml" class="newsHeading">$title</a>
<p class="newsDate">$date</p>
<p class="newsSummary">$summary</p>
__END
}
