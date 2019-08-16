DRIVER = mymobile

pkglib_LTLIBRARIES = mymobile.la
mymobile_la_SOURCES = mymobile.cpp
mymobile_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/build
mymobile_la_LDFLAGS = -module -avoid-version
mymobile_la_LIBADD = ../../libnetxms/libnetxms.la
if USE_INTERNAL_JANSSON
mymobile_la_LIBADD += @top_srcdir@/src/jansson/libnxjansson.la
else
mymobile_la_LIBADD += -ljansson
endif

EXTRA_DIST = \
	mymobile.vcxproj mymobile.vcxproj.filters

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/ncd
	mv -f $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/ncd/$(DRIVER).ncd
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la       
