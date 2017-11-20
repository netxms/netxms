SUBAGENT = postgresql

pkglib_LTLIBRARIES = postgresql.la
postgresql_la_SOURCES = main.cpp
postgresql_la_CPPFLAGS=-I@top_srcdir@/include
postgresql_la_LDFLAGS = -module -avoid-version -export-symbols ../subagent.sym
postgresql_la_LIBADD = ../../libnxagent/libnxagent.la ../../../libnetxms/libnetxms.la ../../../db/libnxdb/libnxdb.la
if USE_INTERNAL_LIBTRE
postgresql_la_LIBADD += ../../../libtre/libnxtre.la
endif

EXTRA_DIST = postgresql.vcxproj postgresql.vcxproj.filters postgresql_subagent.h

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
endif
