DRIVER = dell-pwc

pkglib_LTLIBRARIES = dell-pwc.la
dell_pwc_la_SOURCES = dell-pwc.cpp
dell_pwc_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
dell_pwc_la_LDFLAGS = -module -avoid-version
dell_pwc_la_LIBADD = ../../../libnetxms/libnetxms.la \
		      ../../../snmp/libnxsnmp/libnxsnmp.la \
		      ../../libnxsrv/libnxsrv.la

EXTRA_DIST = \
	dell-pwc.vcxproj dell-pwc.vcxproj.filters \
	dell-pwc.h \
	Makefile.w32

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/ndd
	mv -f $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/ndd/$(DRIVER).ndd
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la
