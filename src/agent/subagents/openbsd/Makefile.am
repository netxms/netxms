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

SUBAGENT = openbsd

pkglib_LTLIBRARIES = openbsd.la
openbsd_la_SOURCES = openbsd.cpp system.cpp disk.cpp net.cpp
openbsd_la_LDFLAGS = -module -avoid-version -lkvm
openbsd_la_LIBADD = ../../../libnetxms/libnetxms.la

EXTRA_DIST = disk.h net.h system.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(pkglibdir)/$(SUBAGENT).a $(pkglibdir)/$(SUBAGENT).so ; rm -f $(pkglibdir)/$(SUBAGENT).a ; fi
	mv $(pkglibdir)/$(SUBAGENT).so $(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(pkglibdir)/$(SUBAGENT).la
	rm -f $(libdir)/libnsm_$(SUBAGENT).so
	ln -s $(pkglibdir)/$(SUBAGENT).nsm $(libdir)/libnsm_$(SUBAGENT).so
