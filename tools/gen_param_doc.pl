#!/usr/bin/perl -w
## This script scans the source files of NetXMS agent and platform
## subagents and generates wiki-style parameter compatibility matrix
## Copyright (C) 2013 Raden Solutions
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License version 2 
## as published by the Free Software Foundation.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

use strict;

# NetXMS source tree
my $NXBASE = ".";

# process command line 
L1:
while ($#ARGV >= 0) {
	if ($ARGV[0] eq "-b") {
		shift @ARGV;
		$NXBASE = $ARGV[0];
		shift @ARGV;
		next L1;
	}
	print STDERR "Incorrect input parameter(s)\n";
	exit;
}

# list of sources
my %paths = (
	"common" 	=>  { "path" => "$NXBASE/src/agent/core/getparam.cpp" },
	"win"		=>  { "path" => "$NXBASE/src/agent/subagents/winnt/main.cpp" },
	"linux"		=>  { "path" => "$NXBASE/src/agent/subagents/linux/linux.cpp" },
	"macos"		=> 	{ "path" => "$NXBASE/src/agent/subagents/darwin/darwin.cpp" },
	"freebsd"	=> 	{ "path" => "$NXBASE/src/agent/subagents/freebsd/freebsd.cpp" },
	"openbsd"	=> 	{ "path" => "$NXBASE/src/agent/subagents/openbsd/openbsd.cpp" },
	"netbsd"	=> 	{ "path" => "$NXBASE/src/agent/subagents/netbsd/netbsd.cpp" },
	"hpux"		=> 	{ "path" => "$NXBASE/src/agent/subagents/hpux/main.cpp" },
	"aix"		=> 	{ "path" => "$NXBASE/src/agent/subagents/aix/main.cpp" },
	"sunos"		=> 	{ "path" => "$NXBASE/src/agent/subagents/sunos/main.cpp" },
	"ipso"		=> 	{ "path" => "$NXBASE/src/agent/subagents/ipso/ipso.cpp" },
);

# all parameters across all subagents
my @params;

# create empty arrays and add references to them
for my $os (keys %paths) {
	$paths{$os}->{params} = [];
}

# process sources
for my $os (keys %paths) {
	my $str = do {
		local $/ = undef;
		open my $fh, "<", $paths{$os}->{path} or die "Cannot open file ".$paths{$os}->{path}." $!";
		<$fh>;
	};
	$str =~ s!/\*.*?\*/!!sg; 
	# common is a special case since it contains params available for 
	# all platforms, win32 is an unpleasant exception though
	if ($os eq "common") {
		while ($str =~ /(?:m_parameters|m_stdParams)\[\]\s*?=\s*?\{
				\s*?\#ifdef\s*?_WIN32(.*?)\#endif(.*?)\}\s*?;/sgix) {
			my $cont_w32 = $1;
			my $cont = $2;
			while ($cont =~ /\{\s*?(?:_T\()?\"(.+?)\"/sgix) {
				push @params, $1;
				for ("win", "linux", "macos", "freebsd", "openbsd", 
					"netbsd", "hpux", "aix", "sunos", "ipso") {
					push @{$paths{$_}->{params}}, $1;					
				}
			}
			while ($cont_w32 =~ /\{\s*?(?:_T\()?\"(.+?)\"/sgix) {
				push @params, $1;
				push @{$paths{"win"}->{params}}, $1;
			}
		}
	}
	else { # there subagents go
		while ($str =~ /(?:m_parameters|m_stdParams)\[\]\s*?=\s*?\{(.*?)\}\s*?;/sgix) {
			# print "$os: Matched!\n";
			my $cont = $1;
			while ($cont =~ /\{\s*?(?:_T\()?\"(.+?)\"/sgix) {
				push @params, $1;
				push @{$paths{$os}->{params}}, $1;
			}
		}
	}
}

my %tmp = map { $_ => 1 } @params;
my @params_sorted = sort keys %tmp;

print_header(1);

for (my $i = 0; $i <= $#params_sorted; $i++) {
	my $p = $params_sorted[$i];
	print "|-\n| $p ||";
	for ("win", "linux", "macos", "freebsd", "openbsd", 
		"netbsd", "aix", "hpux", "sunos", "ipso") {
		if (match_array($p, @{$paths{$_}->{params}})) { print_yes(); } else { print_no(); }					
	}
	print "\n";
	print_header(0) if ($i > 0 && $i % 32 == 0);
}

print_footer();

sub match_array
{
	my $val = shift;
	my @arr = @_;
	for (@arr) { return 1 if $_ eq $val; }
	return 0;
}

sub print_header
{
	my $begin = shift;
	print "{| class=\"wikitable\"\n" if ($begin);
	print <<EOF
|-
! Parameter !! Windows !! Linux !! MacOS !! FreeBSD !! OpenBSD !! NetBSD !! AIX !! HP-UX !! Solaris !! IPSO !! Notes
EOF
;
}

sub print_footer
{
	print "|}\n";
}

sub print_yes
{
	print 'bgcolor="ltgreen"| Yes ||';
}

sub print_no
{
	print 'bgcolor="red"| No ||';
}
