HDLINK = jira

pkglib_LTLIBRARIES = jira.la
jira_la_SOURCES = jira.cpp
jira_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
jira_la_LDFLAGS = -module -avoid-version
jira_la_LIBADD = ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la ../../core/libnxcore.la -lcurl
if USE_INTERNAL_JANSSON
jira_la_LIBADD += @top_srcdir@/src/jansson/libnxjansson.la
else
jira_la_LIBADD += -ljansson
endif

EXTRA_DIST = \
	jira.vcxproj jira.vcxproj.filters \
	jira.h \
	Makefile.w32

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(HDLINK).a $(DESTDIR)$(pkglibdir)/$(HDLINK)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(HDLINK).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(HDLINK)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(HDLINK).hdlink
	rm -f $(DESTDIR)$(pkglibdir)/$(HDLINK).la
