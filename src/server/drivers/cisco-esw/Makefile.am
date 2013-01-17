DRIVER = cisco-esw

pkglib_LTLIBRARIES = cisco-esw.la
cisco_esw_la_SOURCES = cisco-esw.cpp
cisco_esw_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
cisco_esw_la_LDFLAGS = -module -avoid-version
cisco_esw_la_LIBADD = ../../../libnetxms/libnetxms.la \
		      ../../../snmp/libnxsnmp/libnxsnmp.la \
		      ../../libnxsrv/libnxsrv.la

EXTRA_DIST = \
	cisco-esw.vcproj \
	cisco-esw.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER).so ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/ndd
	mv $(DESTDIR)$(pkglibdir)/$(DRIVER).so $(DESTDIR)$(pkglibdir)/ndd/$(DRIVER).ndd
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la
