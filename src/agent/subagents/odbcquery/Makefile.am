INCLUDES=-I@top_srcdir@/include

lib_LTLIBRARIES = libnsm_odbcquery.la
libnsm_odbcquery_la_SOURCES = odbcquery.cpp odbcsapi.cpp
libnsm_odbcquery_la_LDFLAGS = ../../../libnetxms/libnetxms.la

EXTRA_DIST = odbcquery.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then \
		rm -f $(libdir)/libnsm_odbcquery.so.0 ; \
		$(AR) x $(libdir)/libnsm_odbcquery.a $(libdir)/libnsm_odbcquery.so.0 ; \
	fi
