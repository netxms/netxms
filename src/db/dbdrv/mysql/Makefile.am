INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

DRIVER = mysql

pkglib_LTLIBRARIES = mysql.la
mysql_la_SOURCES = mysql.cpp
mysql_la_LDFLAGS = -module -avoid-version
mysql_la_LIBADD = ../../../libnetxms/libnetxms.la @MYSQL_LIBS@

EXTRA_DIST = \
	mysql.vcproj \
	mysqldrv.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(pkglibdir)/$(DRIVER).a $(pkglibdir)/$(DRIVER).so ; rm -f $(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(pkglibdir)/dbdrv
	mv $(pkglibdir)/$(DRIVER).so $(pkglibdir)/dbdrv/$(DRIVER).ddr
	rm -f $(pkglibdir)/$(DRIVER).la 
