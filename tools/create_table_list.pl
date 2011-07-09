#!/usr/bin/perl

use POSIX;

my $inFile = shift || die "Usage : create_table_list.pl <input_file>";
open(IN, "<$inFile") || die "Cannot open input file: $!";

print "/**\n * automatically generated table list\n * created at " . ctime(time()) . " */\n\n";
print "#include <nms_common.h>\n\n";
print "const TCHAR *g_tables[] =\n{\n";

while(<IN>)
{
	chomp;
	$line = $_;
	if ($line =~ /CREATE TABLE ([A-Za-z0-9_]+).*/)
	{
		if ($1 ne "metadata")
		{
			print "\t_T(\"$1\"),\n";
		}
	}
}

close IN;

print "\tNULL\n};\n";
exit 0;
