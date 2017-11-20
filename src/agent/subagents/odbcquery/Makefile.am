SUBAGENT = odbcquery

pkglib_LTLIBRARIES = odbcquery.la
odbcquery_la_SOURCES = odbcquery.cpp odbcsapi.cpp
odbcquery_la_CPPFLAGS=-I@top_srcdir@/include
odbcquery_la_LDFLAGS = -module -avoid-version -export-symbols ../subagent.sym
odbcquery_la_LIBADD = ../../libnxagent/libnxagent.la ../../../libnetxms/libnetxms.la @ODBC_LIBS@

EXTRA_DIST = odbcquery.vcxproj odbcquery.vcxproj.filters odbcquery.h

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
endif
