#!/usr/bin/perl

$version = `svnversion`;
chomp $version;
if ($version =~ /[0-9]+\:([0-9]+)/)
{
	$version = $1;
}
print "Setting build number to $version\n";

open(OUT, ">build_number.xml") || die "out: $!";

print OUT "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
print OUT "<resources>\n";
print OUT "\t<string name=\"build_number\">$version</string>\n";
print OUT "</resources>\n";

close OUT;
exit 0;
