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
nxhttpd_SOURCES = alarm.cpp client.cpp dataview.cpp html.cpp httprequest.cpp \
		  httpresponse.cpp log.cpp nxhttpd.cpp object.cpp session.cpp \
		  smgr.cpp tools.cpp
nxhttpd_LDADD = ../libnetxms/libnetxms.la ../libnxmap/libnxmap.la ../libnxcl/libnxcl.la

EXTRA_DIST = nxhttpd.h winsrv.cpp nxhttpd.dsp nxhttpd.dsw \
	     resource.h nxhttpd.rc messages.mc
