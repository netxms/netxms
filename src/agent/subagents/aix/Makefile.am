SUBAGENT = aix

pkglib_LTLIBRARIES = aix.la
aix_la_SOURCES = cpu.cpp disk.cpp iostat.cpp main.cpp net.cpp proc.cpp system.cpp
aix_la_CPPFLAGS=-I@top_srcdir@/include
aix_la_LDFLAGS = -module -avoid-version -export-symbols ../subagent.sym
aix_la_LIBADD = -lperfstat ../../libnxagent/libnxagent.la ../../../libnetxms/libnetxms.la

EXTRA_DIST = aix_subagent.h

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
endif
