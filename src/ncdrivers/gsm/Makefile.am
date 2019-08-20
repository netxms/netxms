DRIVER = gsm

pkglib_LTLIBRARIES = gsm.la
gsm_la_SOURCES = gsm.cpp pdumode.cpp
gsm_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/build
gsm_la_LDFLAGS = -module -avoid-version
gsm_la_LIBADD = ../../libnetxms/libnetxms.la

EXTRA_DIST = \
	gsm.vcxproj gsm.vcxproj.filters \
	gsm.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/ncdrv
	mv -f $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/ncdrv/$(DRIVER).ncd
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la       
