INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

DRIVER = oracle

pkglib_LTLIBRARIES = oracle.la
oracle_la_SOURCES = oracle.cpp
oracle_la_LDFLAGS = -module -avoid-version
oracle_la_LIBADD = ../../../libnetxms/libnetxms.la @ORACLE_LIBS@

EXTRA_DIST = \
	oracle.vcproj \
	oracledrv.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(pkglibdir)/$(DRIVER).a $(pkglibdir)/$(DRIVER).so ; rm -f $(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(pkglibdir)/dbdrv
	mv $(pkglibdir)/$(DRIVER).so $(pkglibdir)/dbdrv/$(DRIVER).ddr
	rm -f $(pkglibdir)/$(DRIVER).la 
