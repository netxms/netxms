DRIVER = procurve

pkglib_LTLIBRARIES = procurve.la
procurve_la_SOURCES = procurve.cpp
procurve_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
procurve_la_LDFLAGS = -module -avoid-version
procurve_la_LIBADD = ../../../libnetxms/libnetxms.la \
		      ../../../snmp/libnxsnmp/libnxsnmp.la \
		      ../../libnxsrv/libnxsrv.la

EXTRA_DIST = \
	procurve.vcproj \
	procurve.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER).so ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/ndd
	mv $(DESTDIR)$(pkglibdir)/$(DRIVER).so $(DESTDIR)$(pkglibdir)/ndd/$(DRIVER).ndd
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la
