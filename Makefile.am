# Copyright (C) 2004 NetXMS Team <bugs@netxms.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = reconf configure netxms.sln README.HPUX
SUBDIRS = m4 contrib src doc tools debian
SUBDIRS += @TOP_LEVEL_MODULES@

install-exec-hook:
	if test "x@BUILD_SDK@" = "xyes"; then cp config.h $(DESTDIR)$(includedir); mkdir -p $(DESTDIR)$(pkgdatadir)/sdk; cp libtool $(DESTDIR)$(pkgdatadir)/sdk; cp config/install-sh $(DESTDIR)$(pkgdatadir)/sdk; fi
	if test "x@BUILD_SERVER@" = "xyes"; then LD_LIBRARY_PATH="$(DESTDIR)$(libdir):@INSTALL_LIBPATH@:${LD_LIBRARY_PATH}" $(DESTDIR)$(bindir)/nxmibc -d $(DESTDIR)$(pkgdatadir)/mibs -o $(DESTDIR)$(pkgdatadir)/mibs/netxms.mib; fi
