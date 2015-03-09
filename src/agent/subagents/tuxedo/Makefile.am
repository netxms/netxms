SUBAGENT = tuxedo

pkglib_LTLIBRARIES = tuxedo.la
tuxedo_la_SOURCES = clients.cpp domain.cpp machines.cpp main.cpp queues.cpp servers.cpp services.cpp
tuxedo_la_CPPFLAGS=-I@top_srcdir@/include @TUXEDO_CPPFLAGS@
tuxedo_la_LDFLAGS = -module -avoid-version -export-symbols ../subagent.sym @TUXEDO_LDFLAGS@
tuxedo_la_LIBADD = ../../../libnetxms/libnetxms.la -ltux -lfml32 @TUXEDO_LIBS@

EXTRA_DIST = tuxedo.vcproj tuxedo_subagent.h

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
endif
