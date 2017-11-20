DRIVER = generic

pkglib_LTLIBRARIES = generic.la
generic_la_SOURCES = main.cpp pdumode.cpp
generic_la_CPPFLAGS=-I@top_srcdir@/include
generic_la_LDFLAGS = -module -avoid-version
generic_la_LIBADD = ../../libnetxms/libnetxms.la

EXTRA_DIST = \
	generic.vcxproj generic.vcxproj.filters \
	generic_smsdrv.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER).so ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/smsdrv
	mv -f $(DESTDIR)$(pkglibdir)/$(DRIVER).so $(DESTDIR)$(pkglibdir)/smsdrv/$(DRIVER).sms
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la       
	rm -f $(DESTDIR)$(libdir)/libnxsms_$(DRIVER).so*
	ln -s netxms/smsdrv/$(DRIVER).sms $(DESTDIR)$(libdir)/libnxsms_$(DRIVER).so
