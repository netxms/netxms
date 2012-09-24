INCLUDES=-I@top_srcdir@/include

SUBAGENT = odbcquery

pkglib_LTLIBRARIES = odbcquery.la
odbcquery_la_SOURCES = odbcquery.cpp odbcsapi.cpp
odbcquery_la_LDFLAGS = -module -avoid-version
odbcquery_la_LIBADD = ../../../libnetxms/libnetxms.la @ODBC_LIBS@

EXTRA_DIST = odbcquery.vcproj odbcquery.h

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT).so ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv $(DESTDIR)$(pkglibdir)/$(SUBAGENT).so $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT).so
	ln -s $(pkglibdir)/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT).so
endif
