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
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(pkglibdir)/$(DRIVER).a $(pkglibdir)/$(DRIVER).so ; rm -f $(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(pkglibdir)/dbdrv
	mv $(pkglibdir)/$(DRIVER).so $(pkglibdir)/dbdrv/$(DRIVER).ddr
	rm -f $(pkglibdir)/$(DRIVER).la	
	rm -f $(libdir)/libnxddr_$(DRIVER).so*
	ln -s $(pkglibdir)/dbdrv/$(DRIVER).ddr $(libdir)/libnxddr_$(DRIVER).so
