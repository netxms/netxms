DRIVER = slack

pkglib_LTLIBRARIES = slack.la
slack_la_SOURCES = slack.cpp
slack_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
slack_la_LDFLAGS = -module -avoid-version
slack_la_LIBADD = ../../libnetxms/libnetxms.la
if USE_INTERNAL_JANSSON
slack_la_LIBADD += @top_srcdir@/src/jansson/libnxjansson.la
else
slack_la_LIBADD += -ljansson
endif

EXTRA_DIST = \
	slack.vcxproj slack.vcxproj.filters

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(DRIVER).a $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/ncd
	mv -f $(DESTDIR)$(pkglibdir)/$(DRIVER)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/ncd/$(DRIVER).ncd
	rm -f $(DESTDIR)$(pkglibdir)/$(DRIVER).la       
