# Copyright (C) 2004 NetXMS Team <bugs@netxms.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

INCLUDES=-I@top_srcdir@/include

SUBDIRS = static

bin_PROGRAMS = nxhttpd
nxhttpd_SOURCES = \
	messages.c alarm.cpp ctrlpanel.cpp client.cpp dataview.cpp html.cpp \
	httprequest.cpp httpresponse.cpp nxhttpd.cpp json.cpp jsonutil.cpp \
	object.cpp piechart.cpp session.cpp smgr.cpp tools.cpp
nxhttpd_LDADD = ../libnetxms/libnetxms.la ../libnxmap/libnxmap.la ../libnxcl/libnxcl.la

CLEANFILES = messages.c messages.h

messages.c: messages.h

messages.h: messages.mc
	$(PERL) @top_builddir@/tools/mc.pl messages.mc

EXTRA_DIST = nxhttpd.h winsrv.cpp nxhttpd.vcproj \
	     resource.h nxhttpd.rc messages.mc
