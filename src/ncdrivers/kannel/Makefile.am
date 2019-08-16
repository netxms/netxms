DRIVER = kannel

pkglib_LTLIBRARIES = kannel.la
kannel_la_SOURCES = kannel.cpp
kannel_la_CPPFLAGS=-I@top_srcdir@/include  -I@top_srcdir@/build
kannel_la_LDFLAGS = -module -avoid-version
kannel_la_LIBADD = ../../libnetxms/libnetxms.la

EXTRA_DIST = \
	kannel.vcxproj kannel.vcxproj.filters

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/ncd
	mv -f $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/ncd/$(DRIVER).ncd
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la       
