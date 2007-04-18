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
nxhttpd_SOURCES = httprequest.cpp httpresponse.cpp main.cpp misc.cpp nxhttpd.cpp nxpie.cpp nxweb.cpp nxwebhandlers.cpp nxwebhtml.cpp
nxhttpd_LDADD = ../libnetxms/libnetxms.la ../libnxmap/libnxmap.la ../libnxcl/libnxcl.la

EXTRA_DIST = httprequest.h httpresponse.h misc.h nxhttpd.h \
             nxpie.h nxweb.h winsrv.cpp nxhttpd.dsp nxhttpd.dsw
