#!/usr/bin/env perl

my $file_prefix = shift || "netxms";
my $define_prefix = shift || "NETXMS";
my $h_file = shift || $file_prefix . "-build-tag.h";
my $iss_file = shift || $file_prefix . "-build-tag.iss";
my $property_file = shift || $file_prefix . "-build-tag.properties";
my $rc_file = shift || $file_prefix . "-build-tag.rc";

my $tag = `git describe --always --long`;
chomp $tag;
if ($tag eq "")
{
	print "Cannot read git tag\n";
	exit 0;
}
if ($tag =~ /^Release-(.*)/)
{
	$tag = $1;
}
print "Git tag: $tag\n";

my $version_string = $tag;
my $build_number = 0;
if ($tag =~ /(.*)-([0-9]+)-g.*/)
{
   $version_string = $1 . "." . $2;
   $build_number = $2;
}

if (IsUpdateNeeded($h_file, $tag) == 1)
{
	open(OUT, ">$h_file") or die "Cannot open output file: $!";
	print OUT "/* BUILDTAG:$tag */\n";
	print OUT "#ifndef _" . $file_prefix . "_build_tag_h_\n";
	print OUT "#define _" . $file_prefix . "_build_tag_h_\n";
	print OUT "#define " . $define_prefix . "_BUILD_TAG _T(\"$tag\")\n";
	print OUT "#define " . $define_prefix . "_BUILD_TAG_A \"$tag\"\n";
	print OUT "#define " . $define_prefix . "_BUILD_NUMBER $build_number\n";
	print OUT "#define " . $define_prefix . "_VERSION_BUILD $build_number\n";
	print OUT "#define " . $define_prefix . "_VERSION_STRING _T(\"$version_string\")\n";
	print OUT "#define " . $define_prefix . "_VERSION_STRING_A \"$version_string\"\n";
	print OUT "#endif\n";
	close OUT;

	print "Build tag updated in $h_file\n";
}

if (IsUpdateNeeded($iss_file, $tag) == 1)
{
	open(OUT, ">$iss_file") or die "Cannot open output file: $!";
	print OUT ";* BUILDTAG:$tag *\n";
	print OUT "#define VersionString \"$version_string\"\n";
	print OUT "#define BuildTag \"$tag\"\n";
	close OUT;

	print "Build tag updated in $iss_file\n";
}

if (IsUpdateNeeded($property_file, $tag) == 1)
{
	open(OUT, ">$property_file") or die "Cannot open output file: $!";
	print OUT "#* BUILDTAG:$tag *\n";
	print OUT $define_prefix . "_VERSION=$version_string\n";
	print OUT $define_prefix . "_BUILD_TAG=$tag\n";
	close OUT;

	print "Build tag updated in $property_file\n";
}

if (IsUpdateNeeded($rc_file, $tag) == 1)
{
   my $file_version = $version_string;
   $file_version =~ tr /\./,/;
	open(OUT, ">$rc_file") or die "Cannot open output file: $!";
	print OUT "/* BUILDTAG:$tag */\n";
	print OUT "#include \"winres.h\"\n";
   print OUT "VS_VERSION_INFO VERSIONINFO\n";
   print OUT "   FILEVERSION $file_version,0\n";
   print OUT "   PRODUCTVERSION $file_version,0\n";
   print OUT "{\n";
   print OUT "   BLOCK \"StringFileInfo\"\n";
   print OUT "   {\n";
   print OUT "      BLOCK \"040904b0\"\n";
   print OUT "      {\n";
   print OUT "         VALUE \"CompanyName\",      \"Raden Solutions\\0\"\n";
   print OUT "         VALUE \"FileDescription\",  \"NetXMS Monitoring System Component\\0\"\n";
   print OUT "         VALUE \"FileVersion\",      \"$version_string.0\\0\"\n";
   print OUT "         VALUE \"LegalCopyright\",   \"© 2021 Raden Solutions SIA. All Rights Reserved\\0\"\n";
   print OUT "         VALUE \"ProductName\",      \"NetXMS\\0\"\n";
   print OUT "         VALUE \"ProductVersion\",   \"$version_string.0\\0\"\n";
   print OUT "      }\n";
   print OUT "   }\n";
   print OUT "   BLOCK \"VarFileInfo\"\n";
   print OUT "   {\n";
   print OUT "      VALUE \"Translation\", 0x409, 1200\n";
   print OUT "   }\n";
   print OUT "}\n";
	close OUT;

	print "Build tag updated in $rc_file\n";
}

sub IsUpdateNeeded
{
   my $file = $_[0];
   my $tag = $_[1];
   my $update = 1;
   if (open(IN, "<$file"))
   {
      while(<IN>) 
      {
         chomp;
         my $in = $_;
         if ($in =~ /\* BUILDTAG:(.*) \*/)
         {
            print "Build tag in $file: $1\n";
            if ($1 eq $tag)
            {
               $update = 0;
            }
         }
      }
      close IN;
   }

   return $update;
}
