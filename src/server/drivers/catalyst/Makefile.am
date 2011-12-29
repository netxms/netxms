INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

DRIVER = catalyst

pkglib_LTLIBRARIES = catalyst.la
catalyst_la_SOURCES = catalyst.cpp
catalyst_la_LDFLAGS = -module -avoid-version
catalyst_la_LIBADD = ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la ../lib/cisco/libcisco.la

EXTRA_DIST = \
	catalyst.vcproj \
	catalyst.h

install-exec-hook:
	mkdir -p $(DESTDIR)$(pkglibdir)/ndd
	mv $(DESTDIR)$(pkglibdir)/$(DRIVER).so $(DESTDIR)$(pkglibdir)/ndd/$(DRIVER).ndd
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la
