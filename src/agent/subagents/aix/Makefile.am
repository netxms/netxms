INCLUDES=-I@top_srcdir@/include

SUBAGENT = aix

pkglib_LTLIBRARIES = aix.la
aix_la_SOURCES = cpu.cpp disk.cpp iostat.cpp main.cpp net.cpp proc.cpp system.cpp
aix_la_LDFLAGS = -module -avoid-version
aix_la_LIBADD = -lperfstat ../../../libnetxms/libnetxms.la

EXTRA_DIST = aix_subagent.h

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT).so ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv $(DESTDIR)$(pkglibdir)/$(SUBAGENT).so $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT).so
	ln -s $(pkglibdir)/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT).so
endif
