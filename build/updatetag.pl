#!/usr/bin/perl

my $file_prefix = shift || "netxms";
my $define_prefix = shift || "NETXMS";
my $file = shift || $file_prefix . "-build-tag.h";

my $tag = `git describe --always`;
chomp $tag;
if ($tag =~ /^Release-(.*)/)
{
	$tag = $1;
}
print "Git tag: $tag\n";

my $update = 1;

if (open(IN, "<$file"))
{
	while(<IN>) 
	{
		chomp;
		my $in = $_;
		if ($in =~ /\* BUILDTAG:(.*) \*/)
		{
			print "Build tag: $1\n";
			if ($1 eq $tag)
			{
				$update = 0;
			}
		}
	}
	close IN;
}

if ($update == 1)
{
	open(OUT, ">$file") or die "Cannot open output file: $!";
	print OUT "/* BUILDTAG:$tag */\n";
	print OUT "#ifndef _" . $file_prefix . "_build_tag_h_\n";
	print OUT "#define _" . $file_prefix . "_build_tag_h_\n";
	print OUT "#define " . $define_prefix . "_BUILD_TAG _T(\"$tag\")\n";
	print OUT "#define " . $define_prefix . "_BUILD_TAG_A \"$tag\"\n";
	print OUT "#endif\n";
	close OUT;

	print "Build tag updated\n";
}
