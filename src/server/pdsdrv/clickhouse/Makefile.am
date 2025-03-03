DRIVER = clickhouse

pkglib_LTLIBRARIES = clickhouse.la
clickhouse_la_SOURCES = clickhouse.cpp sender.cpp
clickhouse_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
clickhouse_la_LDFLAGS = -module -avoid-version
clickhouse_la_LIBADD = ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la ../../core/libnxcore.la

EXTRA_DIST = \
	clickhouse.h \
	clickhouse.vcxproj clickhouse.vcxproj.filters

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/pdsdrv
	mv -f $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/pdsdrv/$(DRIVER).pdsd
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la
