lib_LTLIBRARIES = libipfix.la
libipfix_la_SOURCES = ipfix_col.cpp ipfix.cpp ipfix_print.cpp ipfix_ssl.cpp libipfix.cpp mpoll.cpp
libipfix_la_CPPFLAGS=-I@top_srcdir@/include -DLIBNXSL_EXPORTS -I@top_srcdir@/build
libipfix_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libipfix_la_LIBADD = ../../libnetxms/libnetxms.la

EXTRA_DIST = gettimeofday.cpp libipfix.vcxproj libipfix.vcxproj.filters libipfix.h
