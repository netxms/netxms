INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

DRIVER = baystack

pkglib_LTLIBRARIES = baystack.la
baystack_la_SOURCES = baystack.cpp
baystack_la_LDFLAGS = -module -avoid-version
baystack_la_LIBADD = ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la ../lib/avaya-ers/libavaya-ers.la

EXTRA_DIST = \
	baystack.vcproj \
	baystack.h

install-exec-hook:
	mkdir -p $(pkglibdir)/ndd
	mv $(pkglibdir)/$(DRIVER).so $(pkglibdir)/ndd/$(DRIVER).ndd
	rm -f $(pkglibdir)/$(DRIVER).la
