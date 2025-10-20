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
EXTRA_DIST = reconf configure netxms.sln jemalloc.sym
SUBDIRS = . m4 build src doc tools contrib manpages
SUBDIRS += @TOP_LEVEL_MODULES@

install-exec-hook:
	if test "x@BUILD_SDK@" = "xyes"; then mkdir -p $(DESTDIR)$(includedir); cp config.h $(DESTDIR)$(includedir); mkdir -p $(DESTDIR)$(pkgdatadir)/sdk; cp libtool $(DESTDIR)$(pkgdatadir)/sdk; cp config/install-sh $(DESTDIR)$(pkgdatadir)/sdk; fi

upload-jars-to-central:
	mvn -f src/maven-central-pom.xml deploy -Ppublish,central
