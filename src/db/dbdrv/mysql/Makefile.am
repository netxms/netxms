DRIVER = mysql

pkglib_LTLIBRARIES = mysql.la
mysql_la_SOURCES = mysql.cpp
mysql_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
mysql_la_LDFLAGS = -module -avoid-version
mysql_la_LIBADD = ../../../libnetxms/libnetxms.la @MYSQL_LIBS@

EXTRA_DIST = \
	mysql.vcproj \
	mysqldrv.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER).so ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/dbdrv
	mv -f $(DESTDIR)$(pkglibdir)/$(DRIVER).so $(DESTDIR)$(pkglibdir)/dbdrv/$(DRIVER).ddr
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la	
	rm -f $(DESTDIR)$(libdir)/libnxddr_$(DRIVER).so*
	ln -s netxms/dbdrv/$(DRIVER).ddr $(DESTDIR)$(libdir)/libnxddr_$(DRIVER).so
