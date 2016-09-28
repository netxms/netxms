SUBAGENT = vmgr

pkglib_LTLIBRARIES = vmgr.la
vmgr_la_SOURCES = vmgr.cpp hostconnections.cpp dataproviders.cpp
vmgr_la_CPPFLAGS=-I@top_srcdir@/include @VMGR_CPPFLAGS@
vmgr_la_LDFLAGS = -module -avoid-version -export-symbols ../subagent.sym @VMGR_LDFLAGS@
vmgr_la_LIBADD = ../../libnxagent/libnxagent.la ../../../libnetxms/libnetxms.la @VMGR_LIBS@

EXTRA_DIST = vmgr.h

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
endif
