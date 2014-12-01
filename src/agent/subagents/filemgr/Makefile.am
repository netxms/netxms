SUBAGENT = filemgr

pkglib_LTLIBRARIES = filemgr.la
filemgr_la_SOURCES = filemgr.cpp logmonitoring.cpp
filemgr_la_CPPFLAGS=-I@top_srcdir@/include
filemgr_la_LDFLAGS = -module -avoid-version
filemgr_la_LIBADD = ../../../libnetxms/libnetxms.la

EXTRA_DIST = filemgr.vcproj filemgr.h

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
endif
