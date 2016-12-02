#!/usr/bin/perl

my $file = shift || die "Usage : updatetag.pl <file>";

my $tag = `git describe`;
chomp $tag;
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
	print OUT "#ifndef _build_tag_h_\n";
	print OUT "#define _build_tag_h_\n";
	print OUT "#define NETXMS_BUILD_TAG _T(\"$tag\")\n";
	print OUT "#define NETXMS_BUILD_TAG_A \"$tag\"\n";
	print OUT "#endif\n";
	close OUT;

	print "Build tag updated\n";
}
