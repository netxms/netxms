# Copyright (C) 2004 NetXMS Team <bugs@netxms.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

bin_PROGRAMS = nxcproxy
nxcproxy_SOURCES = messages.c listener.cpp nxcproxy.cpp session.cpp tools.cpp
nxcproxy_CPPFLAGS=-I@top_srcdir@/include
nxcproxy_LDADD = ../libnetxms/libnetxms.la

if USE_INTERNAL_EXPAT
nxcproxy_LDADD += ../libexpat/libexpat/libnxexpat.la
endif
if USE_INTERNAL_LIBTRE
nxcproxy_LDADD += ../libtre/libnxtre.la
endif

CLEANFILES = messages.c messages.h

messages.c: messages.h

messages.h: messages.mc
	$(PERL) @top_builddir@/tools/mc.pl messages.mc

EXTRA_DIST = \
	nxcproxy.vcxproj nxcproxy.vcxproj.filters \
	messages.mc messages.h messages.c \
	nxcproxy.rc \
	resource.h \
	nxcproxy.h \
	winsrv.cpp
