# Copyright (C) 2004 NetXMS Team <bugs@netxms.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

EXTRA_DIST = reconf configure include netxms.dsw Makefile.nw README.HPUX
SUBDIRS = m4 contrib src doc tools netware
SUBDIRS += @TOP_LEVEL_MODULES@

install-exec-hook:
	if test "x@BUILD_SERVER@" = "xyes"; then $(bindir)/nxmibc -d $(pkgdatadir)/mibs -o $(pkgdatadir)/mibs/netxms.mib; fi
