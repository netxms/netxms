#!/usr/bin/perl -w

use strict;
#use warnings;

use CGI;
use POSIX qw(strftime mktime);

###############################################################################
# globals
my @news;

###############################################################################
# config
my $newsFile = "../netxms.news";

###############################################################################
# code

my $q = new CGI;
my $cut = 0;
my $rss = 0;
if (defined $q->param("cut"))
{
	$cut = 1;
}
if (defined $q->param("rss"))
{
	print "Content-type: text/xml\n\n";
	$rss = 1;
}
else
{
	print "Content-type: text/html\n\n";
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
else
{
	if ($rss == 1)
	{
		$count = 10;
	}
}

if ($rss == 0)
{
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
}
else
{
	#my $buildDate = "Thu, 26 May 2005 12:22:00 EET";
	my $buildDate = strftime("%a, %e %b %Y %H:%M:%S %Z", localtime());
	print <<"__END";
<?xml version="1.0" encoding="UTF-8"?>
<rss version="2.0">
	<channel>
		<title>NetXMS News</title>
		<link>http://netxms.org</link>
		<description>NetMXS - Open source monitoring system</description>
		<lastBuildDate>$buildDate</lastBuildDate>
__END

	my %m;
	$m{Jan} = 0;
	$m{Feb} = 1;
	$m{Mar} = 2;
	$m{Apr} = 3;
	$m{May} = 4;
	$m{Jun} = 5;
	$m{Jul} = 6;
	$m{Aug} = 7;
	$m{Sep} = 8;
	$m{Oct} = 9;
	$m{Nov} = 10;
	$m{Dec} = 11;

	for (my $i = 0; $i < $count; $i++)
	{
		#my $pubDate = strftime("%a, %d %b %Y %H:%M:%S %Z", localtime(time()));
		if ($news[$i]->{date} =~ /^([0-9]+)-([a-z]+)-([0-9]+)$/i)
		{
			my $pubDate = strftime("%a, %d %b %Y %H:%M:%S %Z",
				localtime(mktime(0, 0, 11, $1, $m{$2}, $3 - 1900)));
			my $title = $news[$i]->{title};
			my $description = $news[$i]->{sum};
			$title =~ s/</&lt;/g; $title =~ s/>/&gt;/g;
			$description =~ s/</&lt;/g; $description =~ s/>/&gt;/g;
			print <<"__END";
		<item>
			<pubDate>$pubDate</pubDate>
			<link>http://netxms.org</link>
			<title>$title</title>
			<description>
				$description
			</description>
		</item>
__END
		}
	}
	print <<"__END";
	</channel>
</rss>
__END
}
