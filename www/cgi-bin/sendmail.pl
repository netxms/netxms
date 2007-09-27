#!/usr/bin/perl -w

use CGI qw/:cgi/;
use Net::SMTP;

use strict;

my $q = new CGI;
if (defined $q->param('contact') && defined $q->param('text'))
{
	my $smtp = Net::SMTP->new('mail.alk.lv');

	$smtp->mail($ENV{USER});
	$smtp->to('team@netxms.org');

	$smtp->data();
	$smtp->datasend("From: web\@netxms.org\n");
	$smtp->datasend("To: team\@netxms.org\n");
	$smtp->datasend("Subject: [web form]\n");
	my $date = `date "+%a, %d %b %Y %d %R:%S %z"`;
	$smtp->datasend("Content-Type: text/plain; charset=utf-8\n");
	$smtp->datasend("Date: $date");
	my $contact = $q->param('contact');
	if ($contact =~ /^[a-z][\w\.-]*[a-z0-9]@[a-z0-9][\w\.-]*[a-z0-9]\.[a-z][a-z\.]*[a-z]$/i) {
		$smtp->datasend("Reply-To: $contact\n");
	}
	$smtp->datasend("\n");
	$smtp->datasend("IP: " . $ENV{REMOTE_ADDR} . "\n\n");
	$smtp->datasend("Contact: " . $q->param('contact') . "\n\n");
	$smtp->datasend("Text:\n" . $q->param('text') . "\n");
	$smtp->dataend();

	$smtp->quit;

	if (defined $ENV{HTTP_REFERER})
	{
		print "Location: $ENV{HTTP_REFERER}\n\n";
	}
	else
	{
		print "Content-type: text/html\n\n";
	}
	print "<html><body>\nClick <a href=\"http://netxms.org\">here</a> to go back\n</body></html>";
}
else
{
	print "Content-type: text/plain\n\ninvalid use, reported to administrator";
}
