#!/usr/bin/env perl

use File::Basename;

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
	exit 1;
}
if ($tag =~ /^Release-(.*)/)
{
	$tag = $1;
}
print "Git tag: $tag\n";

my $version_major = "0";
my $version_minor = "0";
my $version_release = "0";
my $is_core_module = 0;

my $dirname = dirname(__FILE__);
if (-e "$dirname/../include/netxms-version.h")
{
	$is_core_module = 1;
	$version_major=`cat $dirname/../include/netxms-version.h | grep NETXMS_VERSION_MAJOR | awk '{print \$3}'`;
	$version_minor=`cat $dirname/../include/netxms-version.h | grep NETXMS_VERSION_MINOR | awk '{print \$3}'`;
	$version_release=`cat $dirname/../include/netxms-version.h | grep NETXMS_VERSION_RELEASE | awk '{print \$3}'`;
	chomp $version_major;
	chomp $version_minor;
	chomp $version_release;
}
else
{
	if ($tag =~ /(.*)-([0-9]+)-g.*/)
	{
		$vtmp = $1;
		$version_release = $2;
		if ($vtmp =~ /V?([0-9]+)\.([0-9]+)/)
		{
			$version_major = $1;
			$version_minor = $2;
		}
		else
		{
			$version_major = $vtmp;
		}
	}
}
my $version_base=$version_major . "." . $version_minor . "." . $version_release;

my $build_number = 0;
my $is_release = 0;
my $is_release_candidate = 0;
my $version_qualifier = "";
my $exact_tag=`git describe --exact-match --tags`;
if ($? == 0)
{
   chomp $exact_tag;
   if ($exact_tag eq "release-" . $version_base)
   {
      $is_release=1;
   }
}

my $version_string=$version_base . "-SNAPSHOT";
if ($is_release)
{
   $version_string=$version_base;
}
elsif ($is_core_module)
{
   my $release_tag=`git describe --tags --match release-$version_base`;
   if ($? == 0)
   {
      chomp $release_tag;
      if ($release_tag =~ /release-(.*)-([0-9]+)-g.*/)
      {
         $build_number=$2;
         $version_string=$version_base . "." . $2;
         $version_qualifier="sp" . $2;
      }
   }
   else
   {
      if ($tag =~ /(.*)-([0-9]+)-g.*/)
      {
         $build_number=$2;
         $version_string=$version_base . "-rc" . $2;
         $version_qualifier="rc" . $2;
         $is_release_candidate=1;
      }
   }
}
else
{
	$build_number = $version_release;
	$version_string = $version_base;
}

print "Base version: $version_base\n";
print "Version string: $version_string\n";

if (IsUpdateNeeded($h_file, $tag) == 1)
{
	open(OUT, ">$h_file") or die "Cannot open output file: $!";
	print OUT "/* BUILDTAG:$tag */\n";
	print OUT "#ifndef _" . $file_prefix . "_build_tag_h_\n";
	print OUT "#define _" . $file_prefix . "_build_tag_h_\n";
	print OUT "#define " . $define_prefix . "_BUILD_TAG _T(\"$tag\")\n";
	print OUT "#define " . $define_prefix . "_BUILD_TAG_A \"$tag\"\n";
	print OUT "#define " . $define_prefix . "_BUILD_NUMBER $build_number\n";
	print OUT "#define " . $define_prefix . "_BUILD_NUMBER_STRING _T(\"$build_number\")\n";
	print OUT "#define " . $define_prefix . "_BUILD_NUMBER_STRING_A \"$build_number\"\n";
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
	print OUT "#define BaseVersion \"$version_base\"\n";
	print OUT "#define BuildNumber \"$build_number\"\n";
	print OUT "#define BuildTag \"$tag\"\n";
	close OUT;

	print "Build tag updated in $iss_file\n";
}

if (IsUpdateNeeded($property_file, $tag) == 1)
{
	open(OUT, ">$property_file") or die "Cannot open output file: $!";
	print OUT "#* BUILDTAG:$tag *\n";
	print OUT $define_prefix . "_BUILD_NUMBER=$build_number\n";
	print OUT $define_prefix . "_BUILD_TAG=$tag\n";
	print OUT $define_prefix . "_VERSION=$version_string\n";
	print OUT $define_prefix . "_BASE_VERSION=$version_base\n";
	print OUT $define_prefix . "_VERSION_QUALIFIER=$version_qualifier\n";
	close OUT;

	print "Build tag updated in $property_file\n";
}

if (IsUpdateNeeded($rc_file, $tag) == 1)
{
   my $file_build_number=($is_release_candidate ? 0 : $build_number);
   my $file_version = $version_major . "," . $version_minor . "," . $version_release . "," . $file_build_number;
	open(OUT, ">$rc_file") or die "Cannot open output file: $!";
	print OUT "/* BUILDTAG:$tag */\n";
	print OUT "#include \"winres.h\"\n";
   print OUT "VS_VERSION_INFO VERSIONINFO\n";
   print OUT "   FILEVERSION $file_version\n";
   print OUT "   PRODUCTVERSION $file_version\n";
   print OUT "{\n";
   print OUT "   BLOCK \"StringFileInfo\"\n";
   print OUT "   {\n";
   print OUT "      BLOCK \"040904b0\"\n";
   print OUT "      {\n";
   print OUT "         VALUE \"CompanyName\",      \"Raden Solutions\\0\"\n";
   print OUT "         VALUE \"FileDescription\",  \"NetXMS Monitoring System Component\\0\"\n";
   print OUT "         VALUE \"FileVersion\",      \"$version_major.$version_minor.$version_release.$file_build_number\\0\"\n";
   print OUT "         VALUE \"LegalCopyright\",   \"� 2023 Raden Solutions SIA. All Rights Reserved\\0\"\n";
   print OUT "         VALUE \"ProductName\",      \"NetXMS\\0\"\n";
   print OUT "         VALUE \"ProductVersion\",   \"$version_major.$version_minor.$version_release.$file_build_number\\0\"\n";
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
