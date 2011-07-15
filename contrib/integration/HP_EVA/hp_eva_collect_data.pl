#!/usr/bin/perl

## NetXMS - Network Management System
## Integration script for HP EVA disk arrays
## Copyright (C) 2011 Victor Kirhenshtein
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

### configuration ###
##
$SSSU = "/opt/netxms/bin/sssu_linux_x86";
$DATADIR="/tmp";
$MANAGER="10.0.0.1";
$SYSTEM="EVA4400";
$USERNAME="admin";
$PASSWORD="password";


### Execute SSSU ###
##
sub sssu
{
	my $cmd = $SSSU . " \"SELECT MANAGER $MANAGER USERNAME=$USERNAME PASSWORD=$PASSWORD\" \"SELECT SYSTEM $SYSTEM\"";
	for(my $i = 0; $i <= $#_; $i++)
	{
		$cmd = $cmd . " \"$_[$i]\"";
	}
	system $cmd . " > sssu_exec_log.txt";
}


### Process data for single element ###
##
## Parameters:
##  0: element class (CONTROLLER, DISK, etc.)
##  1: element name (like "\Hardware\Controller Enclosure\Controller 2")
##  2: prefix
##  3: instance
##
sub process_element
{
	my $prefix = $_[2];
	my $instance = $_[3];

	unlink("element.txt");
	sssu("LS $_[0] \\\"$_[1]\\\" > element.txt");

	local *IN;
	open(IN, "<element.txt") || return 1;

	my %refCount = ();
	while(<IN>)
	{
		chomp;
		my $line = $_;
		if (!($line =~ /\s*[a-zA-Z0-9]+\s\.+\:\s.*/))
		{
			if ($line =~ /\s+(.*)/)
			{
				if ($refCount{$1} == undef)
				{
					$refCount{$1} = 1;
				}
				else
				{
					$refCount{$1}++;
				}
			}
		}
	}
	close(IN);

	while(($key, $value) = each(%refCount))
	{ 
		if ($value > 1)
		{
			$refCount{$key} = 0;
		}
		else
		{
			$refCount{$key} = -1;
		}
	}

	open(IN, "<element.txt") || return 1;

	my @path;
	my $path = "";
	my $currentLevel = -1;
	while(<IN>)
	{
		chomp;
		my $line = $_;
		if ($line =~ /(\s*)([a-zA-Z0-9]+)\s\.+\:\s(.*)/)
		{
			my $level = length($1) / 2 - 1;
			if ($level != $currentLevel)
			{
				$currentLevel = $level;
				$path = $path[0];
				for(my $i = 1; $i <= $level; $i++)
				{
					$path = $path . "." . $path[$i];
				}
			}
			my $element = $path . "." . $2;
			$PARAMETERS{$element} = $3;
		}
		else
		{
			if ($line =~ /(\s*)(.*)/)
			{
				my $level = length($1) / 2;
				if ($level == 0 && $2 eq "object")
				{
					$path[0] = $prefix . "[" . $instance . "]";
					$path = $path[0];
				}
				else
				{
					if ($refCount{$2} == -1)
					{
						$path[$level] = $2;
					}
					else
					{
						$refCount{$2}++;
						$path[$level] = $2 . "[" . $refCount{$2} . "]";
					}
					$path = $path[0];
					for(my $i = 1; $i <= $level; $i++)
					{
						$path = $path . "." . $path[$i];
					}
				}
				$currentLevel = $level;
			}
		}
	}

	close IN;
	return 0;
}


### Collect information about specific element class ###
##
## Parameters:
##  0: input file
##  1: element class
##  2: prefix
##
sub collect_elements_data
{
	open(IN, "<" . $_[0]) || return 1;

	my $instance = 0;
	while(<IN>)
	{
		chomp;
		my $line = $_;
		if ($line =~ /[ \t]*(\\.*)/)
		{
			$instance++;
			process_element($_[1], $1, $_[2], $instance);
		}
	}

	close IN;
	return 0;
}


### Main ###
##
chdir $DATADIR;
unlink("controllers.txt");
unlink("diskshelfs.txt");
unlink("disks.txt");
sssu("LS CONTROLLER > controllers.txt", "LS DISKSHELF > diskshelfs.txt", "LS DISK > disks.txt");

%PARAMETERS = ();

collect_elements_data("controllers.txt", "CONTROLLER", "EVA.Controller");
collect_elements_data("diskshelfs.txt", "DISKSHELF", "EVA.DiskShelf");
collect_elements_data("disks.txt", "DISK", "EVA.Disk");

while(($key, $value) = each(%PARAMETERS))
{
	print $key . "=" . $value . "\n";
}

exit 0;

