#!/usr/bin/env perl

use POSIX;

opendir my $dir, "java/target/lib" or die "Cannot open directory: $!";
my @files = readdir $dir;
closedir $dir;

open(OUT, ">jar_list.cpp") || die "Cannot open output file: $!";

print OUT "/**\n * automatically generated JAR list\n * created at " . ctime(time()) . " */\n\n";
print OUT "#include <nms_common.h>\n\n";
print OUT "const TCHAR *g_jarList[] =\n{\n";

foreach(@files)
{
	$file = $_;
	if ($file =~ /.*\.jar$/)
	{
		print OUT "   _T(\"$file\"),\n";
	}
}

print OUT "   nullptr\n};\n";
exit 0;
