SUBAGENT           = db2

pkglib_LTLIBRARIES = db2.la
db2_la_SOURCES     = db2_subagent.cpp db2dci.cpp
db2_la_CPPFLAGS    = -I@top_srcdir@/include
db2_la_LDFLAGS     = -module -avoid-version -export-symbols ../subagent.sym
db2_la_LIBADD      = ../../libnxagent/libnxagent.la ../../../libnetxms/libnetxms.la ../../../db/libnxdb/libnxdb.la

EXTRA_DIST         = db2_subagent.h db2dci.h db2.vcxproj db2.vcxproj.filters Makefile.w32

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
endif
