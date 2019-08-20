DRIVER = portech

pkglib_LTLIBRARIES = portech.la
portech_la_SOURCES = portech.cpp pdumode.cpp
portech_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/build
portech_la_LDFLAGS = -module -avoid-version
portech_la_LIBADD = ../../libnetxms/libnetxms.la

EXTRA_DIST = \
	portech.h \
	portech.vcxproj portech.vcxproj.filters \
	README.txt

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/ncdrv
	mv -f $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/ncdrv/$(DRIVER).ncd
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la       
