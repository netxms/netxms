DRIVER = cisco

pkglib_LTLIBRARIES = cisco.la
cisco_la_SOURCES = cat2900xl.cpp catalyst.cpp cisco.cpp esw.cpp sb.cpp
cisco_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
cisco_la_LDFLAGS = -module -avoid-version
cisco_la_LIBADD = ../../../libnetxms/libnetxms.la \
	../../../snmp/libnxsnmp/libnxsnmp.la \
	../../libnxsrv/libnxsrv.la

EXTRA_DIST = \
	cisco.h \
	cisco.vcxproj cisco.vcxproj.filters

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/ndd
	mv -f $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/ndd/$(DRIVER).ndd
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la
