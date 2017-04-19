HDLINK = redmine

pkglib_LTLIBRARIES = redmine.la
redmine_la_SOURCES = redmine.cpp
redmine_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
redmine_la_LDFLAGS = -module -avoid-version
redmine_la_LIBADD = ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la ../../core/libnxcore.la -lcurl
if USE_INTERNAL_JANSSON
redmine_la_LIBADD += @top_srcdir@/src/jansson/libnxjansson.la
else
redmine_la_LIBADD += -ljansson
endif

EXTRA_DIST = \
	redmine.h \
	Makefile.w32

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(HDLINK).a $(DESTDIR)$(pkglibdir)/$(HDLINK)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(HDLINK).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(HDLINK)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(HDLINK).hdlink
	rm -f $(DESTDIR)$(pkglibdir)/$(HDLINK).la
