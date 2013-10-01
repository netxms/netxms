INCLUDES=-I@top_srcdir@/include

SUBAGENT = dbquery

pkglib_LTLIBRARIES = dbquery.la
odbcquery_la_SOURCES = dbquery.cpp
odbcquery_la_LDFLAGS = -module -avoid-version
odbcquery_la_LIBADD = ../../../libnetxms/libnetxms.la ../../../db/libnxdb/libnxdb.la

EXTRA_DIST = dbquery.vcproj dbquery.h

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT).so ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv $(DESTDIR)$(pkglibdir)/$(SUBAGENT).so $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT).so
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT).so
endif
