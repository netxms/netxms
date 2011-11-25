INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

DRIVER = cat2900xl

pkglib_LTLIBRARIES = cat2900xl.la
cat2900xl_la_SOURCES = cat2900xl.cpp
cat2900xl_la_LDFLAGS = -module -avoid-version
cat2900xl_la_LIBADD = ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la ../lib/cisco/libcisco.la

EXTRA_DIST = \
	cat2900xl.vcproj \
	cat2900xl.h

install-exec-hook:
	mkdir -p $(DESTDIR)$(pkglibdir)/ndd
	mv $(DESTDIR)$(pkglibdir)/$(DRIVER).so $(DESTDIR)$(pkglibdir)/ndd/$(DRIVER).ndd
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la
