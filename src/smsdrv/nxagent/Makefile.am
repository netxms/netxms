DRIVER = nxagent

pkglib_LTLIBRARIES = nxagent.la
nxagent_la_SOURCES = main.cpp
nxagent_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
nxagent_la_LDFLAGS = -module -avoid-version
nxagent_la_LIBADD = ../../libnetxms/libnetxms.la ../../server/libnxsrv/libnxsrv.la

EXTRA_DIST = \
	nxagent.vcxproj nxagent.vcxproj.filters

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER).so ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/smsdrv
	mv -f $(DESTDIR)$(pkglibdir)/$(DRIVER).so $(DESTDIR)$(pkglibdir)/smsdrv/$(DRIVER).sms
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la       
	rm -f $(DESTDIR)$(libdir)/libnxsms_$(DRIVER).so*
	ln -s netxms/smsdrv/$(DRIVER).sms $(DESTDIR)$(libdir)/libnxsms_$(DRIVER).so
