SUBAGENT = java

pkglib_LTLIBRARIES = java.la
java_la_SOURCES = ConfigHelper.cpp main.cpp SubAgent.cpp
java_la_CPPFLAGS = -I@top_srcdir@/include
java_la_LDFLAGS = -module -avoid-version -export-symbols ../subagent.sym
java_la_LIBADD = ../../libnxagent/libnxagent.la ../../../libnetxms/libnetxms.la

EXTRA_DIST = \
	java.vcproj \
	java_subagent.h \
	SubAgent.h

SUBDIRS = java sample

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
endif
