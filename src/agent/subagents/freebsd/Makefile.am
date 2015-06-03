SUBAGENT = freebsd

pkglib_LTLIBRARIES = freebsd.la
freebsd_la_SOURCES = disk.cpp freebsd.cpp net.cpp proc.cpp system.cpp 
freebsd_la_CPPFLAGS=-I@top_srcdir@/include
freebsd_la_LDFLAGS = -module -avoid-version -export-symbols ../subagent.sym
freebsd_la_LIBADD = ../../../libnetxms/libnetxms.la -lkvm

EXTRA_DIST = disk.h net.h system.h

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
endif
