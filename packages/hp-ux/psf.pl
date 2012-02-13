#!/usr/bin/perl -w
################################################################################
#
#  psf
#
#  Builds a Product Specification File (psf) from the directories listed
#  on the command line.  It assumes that the following naming convention
#  has been used for the directory names.  The first part of the directory
#  name is the product name, and the rest is the product version.
#
#  Author:  David C. Snyder
#
#  $Log: psf,v $
#  Revision 1.2  1997/09/05 11:44:16  dsnyder
#  Changed the name of the script to psf (from psfls)
#
#  Revision 1.1  1997/09/02 15:00:42  dsnyder
#  Initial revision
#
################################################################################

use vars qw($length);
use strict;

die "Usage:  $0: dir ...\n" unless ( @ARGV );

my ($dir, $tag, $revision);
while ( $dir = shift ) {
   next unless ( -d $dir );
   $dir =~ m{.*/(\D+)(.*)$} or $dir =~ m{(\D+)(.*)$};
   $length = length( $1 . $2 );
   $tag = uc $1;
   $revision = $2;
   $tag =~ s/\W+/_/g;            # Change non-word characters to '_'
   $tag =~ s/_+$//;              # Remove trailing '_'s
   $tag = substr( $tag, 0, 16 ); # Truncate to 16 or fewer characters
   $revision =~ s/^\W+//;        # Remove leading punctuation
   print "
product
   tag                  $tag
   revision             $revision
   fileset
      tag               ", lc $tag, "\n";
   listdir( $dir ) if ( -d $dir );
   print "   end\nend\n";
}
exit( 0 );


sub listdir {
   my $dir = shift;
   my ( $mode, $entry );
   my @directories;

   opendir CWD, "$dir" or die "$0: opendir $dir: $!\n";
   printf "      directory         %s=/opt/netxms%s\n",
          $dir, substr( $dir, $length );
   foreach $entry ( sort readdir CWD ) {
      next if ( $entry eq "." or $entry eq ".." or
                $entry eq "..install_finish" or
                $entry eq "..install_start" );
      lstat "$dir/$entry" or die "$0: stat $dir/$entry: $!\n";
      if ( -d _ ) {
         push @directories, $entry;
      } else {
         printf "      file              %s\n", $entry;
      }
   }
   foreach $entry ( sort @directories ) {
      listdir( "$dir/$entry" );
   }
   closedir CWD;
}
