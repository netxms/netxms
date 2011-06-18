INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

DRIVER = db2

pkglib_LTLIBRARIES = db2.la
db2_la_SOURCES = db2.cpp
db2_la_LDFLAGS = -module -avoid-version
db2_la_LIBADD = ../../../libnetxms/libnetxms.la @DB2_LIBS@

EXTRA_DIST = db2.vcproj db2drv.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(pkglibdir)/$(DRIVER).a $(pkglibdir)/$(DRIVER).so ; rm -f $(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(pkglibdir)/dbdrv
	mv $(pkglibdir)/$(DRIVER).so $(pkglibdir)/dbdrv/$(DRIVER).ddr
	rm -f $(pkglibdir)/$(DRIVER).la 
