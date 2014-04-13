HDLINK = jira

pkglib_LTLIBRARIES = jira.la
jira_la_SOURCES = jira.cpp
jira_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
jira_la_LDFLAGS = -module -avoid-version
jira_la_LIBADD = ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la ../../core/libnxcore.la

EXTRA_DIST = \
	jira.vcproj \
	jira.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(HDLINK).a $(DESTDIR)$(pkglibdir)/$(HDLINK)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(HDLINK).a ; fi
	mkdir -p $(DESTDIR)$(pkglibdir)/hdlink
	mv -f $(DESTDIR)$(pkglibdir)/$(HDLINK)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/hdlink/$(HDLINK).hdlink
	rm -f $(DESTDIR)$(pkglibdir)/$(HDLINK).la
