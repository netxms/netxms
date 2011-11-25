# Copyright (C) 2006 NetXMS Team
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

INCLUDES=-I@top_srcdir@/include
AM_CXXFLAGS=-fpermissive 

SUBAGENT = netbsd

pkglib_LTLIBRARIES = netbsd.la
netbsd_la_SOURCES = netbsd.cpp system.cpp disk.cpp net.cpp proc.cpp proc2.cpp
netbsd_la_LDFLAGS = -module -avoid-version -lkvm
netbsd_la_LIBADD = ../../../libnetxms/libnetxms.la

EXTRA_DIST = disk.h net.h system.h

install-exec-hook:
	mv $(DESTDIR)$(pkglibdir)/$(SUBAGENT).so $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT).so
	ln -s $(pkglibdir)/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT).so
