INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

lib_LTLIBRARIES = libnxddr_mysql.la
libnxddr_mysql_la_SOURCES = mysql.cpp
libnxddr_mysql_la_LDFLAGS = -version-info $(DBDRV_LIBRARY_VERSION) ../../../libnetxms/libnetxms.la

EXTRA_DIST = \
	mysql.dsp mysql.dsw \
	mysqldrv.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then $(AR) x $(libdir)/libnxddr_mysql.a $(libdir)/libnxddr_mysql.so.0 ; fi
