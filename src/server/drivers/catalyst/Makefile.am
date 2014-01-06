DRIVER = catalyst

pkglib_LTLIBRARIES = catalyst.la
catalyst_la_SOURCES = catalyst.cpp
catalyst_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
catalyst_la_LDFLAGS = -module -avoid-version
catalyst_la_LIBADD = ../lib/cisco/libcisco.la ../../libnxsrv/libnxsrv.la ../../../snmp/libnxsnmp/libnxsnmp.la ../../../libnetxms/libnetxms.la

EXTRA_DIST = \
	catalyst.vcproj \
	catalyst.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/ndd
	mv -f $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/ndd/$(DRIVER).ndd
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la
