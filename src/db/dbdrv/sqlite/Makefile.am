INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/src/sqlite

DRIVER = sqlite

pkglib_LTLIBRARIES = sqlite.la
sqlite_la_SOURCES = sqlite.cpp
sqlite_la_LDFLAGS = -module -avoid-version
sqlite_la_LIBADD = ../../../libnetxms/libnetxms.la ../../../sqlite/libnxsqlite.la

EXTRA_DIST = \
	sqlite.vcproj \
	sqlitedrv.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then $(AR) x $(pkglibdir)/sqlite.a $(pkglibdir)/sqlite.so ; fi
	mkdir -p $(pkglibdir)/dbdrv
	mv $(pkglibdir)/$(DRIVER).so $(pkglibdir)/dbdrv/$(DRIVER).ddr
	rm -f $(pkglibdir)/$(DRIVER).la	
