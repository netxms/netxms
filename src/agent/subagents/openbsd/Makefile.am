# Copyright (C) 2006 NetXMS Team
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AM_CXXFLAGS=-fpermissive 

SUBAGENT = openbsd

pkglib_LTLIBRARIES = openbsd.la
openbsd_la_SOURCES = openbsd.cpp system.cpp disk.cpp net.cpp
openbsd_la_CPPFLAGS=-I@top_srcdir@/include
openbsd_la_LDFLAGS = -module -avoid-version -lkvm
openbsd_la_LIBADD = ../../../libnetxms/libnetxms.la

EXTRA_DIST = disk.h net.h system.h

if !STATIC_BUILD
install-exec-hook:
	mv $(DESTDIR)$(pkglibdir)/$(SUBAGENT).so $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT).so
	ln -s $(pkglibdir)/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT).so
endif
