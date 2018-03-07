DRIVER = avaya

pkglib_LTLIBRARIES = avaya.la
avaya_la_SOURCES = avaya.cpp baystack.cpp ers8000.cpp ntws.cpp vlan-if.cpp
avaya_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
avaya_la_LDFLAGS = -module -avoid-version
avaya_la_LIBADD = ../../../libnetxms/libnetxms.la \
	../../../snmp/libnxsnmp/libnxsnmp.la \
	../../libnxsrv/libnxsrv.la

EXTRA_DIST = \
	avaya.h \
	avaya.vcxproj avaya.vcxproj.filters

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/ndd
	mv -f $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/ndd/$(DRIVER).ndd
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la
