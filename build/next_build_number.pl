#!/usr/bin/perl

use File::Copy;

open my $in, '<', "build_number" or die "cannot open \"build_number\" file";
my $build = <$in>;
close $in;

$build++;
print "Setting build number to $build\n";

open my $out, '>', "build_number";
print $out "$build\n";
close $out;

open my $outh, '>', "../include/build.h" or die "cannot open build.h";
print $outh "#ifndef __build_h\n";
print $outh "#define __build_h\n";
print $outh "#define NETXMS_VERSION_BUILD $build\n";
print $outh "#define NETXMS_VERSION_BUILD_STRING _T(\"$build\")\n";
print $outh "#endif\n";
close $outh;

open my $outxml, '>', "../android/src/console/res/values/build_number.xml" or die "cannot open build_number.xml";
print $outxml "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
print $outxml "<resources>\n";
print $outxml "\t<string name=\"build_number\">$build</string>\n";
print $outxml "</resources>\n";
close $outxml;

copy("../android/src/console/res/values/build_number.xml","../android/src/agent/res/values/build_number.xml") or die "Copy failed: $!";

open my $outcmd, '>', "../src/java/build/set_build_number.cmd" or die "cannot open set_build_number.cmd";
print $outcmd "set build_number=$build\n";
close $outcmd;

open my $outsh, '>', "../src/java/build/set_build_number.sh" or die "cannot open set_build_number.sh";
print $outsh "build_number=$build\n";
close $outsh;

open my $outjava, '>', "../src/java/client/netxms-base/src/main/java/org/netxms/base/BuildNumber.java" or die "cannot open BuildNumber.java";
print $outjava "package org.netxms.base;\n";
print $outjava "public final class BuildNumber {\n";
print $outjava "   public static final String TEXT = \"$build\";\n";
print $outjava "   public static final int NUMBER = $build;\n";
print $outjava "}\n";
close $outjava;
