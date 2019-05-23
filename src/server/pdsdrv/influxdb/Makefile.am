DRIVER = jira

pkglib_LTLIBRARIES = influxdb.la
influxdb_la_SOURCES = influxdb.cpp
influxdb_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
influxdb_la_LDFLAGS = -module -avoid-version
influxdb_la_LIBADD = ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la ../../core/libnxcore.la

EXTRA_DIST = \
	influxdb.vcxproj influxdb.vcxproj.filters

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(DRIVER).pdsd
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la
