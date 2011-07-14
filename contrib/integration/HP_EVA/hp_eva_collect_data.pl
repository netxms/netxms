#!/usr/bin/perl

### configuration ###
##
$SSSU = "/opt/netxms/bin/sssu_linux_x86";
$DATADIR="/opt/netxms/var/eva";
$MANAGER="172.16.54.45";
$SYSTEM="EVA4400";
$USERNAME="admin";
$PASSWORD="%73tRUe3^*fy+";


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

	my @path;
	my $path = "";
	while(<IN>)
	{
		chomp;
		my $line = $_;
		if ($line =~ /\s*([a-zA-Z0-9]+)\s\.+\:\s(.*)/)
		{
			my $element = $path . "." . $1 . "(" . $instance . ")";
			$PARAMETERS{$element} = $2;
		}
		else
		{
			if ($line =~ /(\s*)(.*)/)
			{
				my $level = length($1) / 2;
				if ($level == 0 && $2 eq "object")
				{
					$path[0] = $prefix;
					$path = $prefix;
				}
				else
				{
					$path[$level] = $2;
					$path = $path[0];
					for(my $i = 1; $i <= $level; $i++)
					{
						$path = $path . "." . $path[$i];
					}
				}
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

while (($key, $value) = each(%PARAMETERS))
{
	print $key . "=" . $value . "\n";
}

exit 0;

