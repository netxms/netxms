INCLUDES=-I@top_srcdir@/include

SUBAGENT = aix

pkglib_LTLIBRARIES = aix.la
aix_la_SOURCES = cpu.cpp disk.cpp iostat.cpp main.cpp net.cpp proc.cpp system.cpp
aix_la_LDFLAGS = -module -avoid-version
aix_la_LIBADD = -lperfstat ../../../libnetxms/libnetxms.la

EXTRA_DIST = aix_subagent.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(pkglibdir)/$(SUBAGENT).a $(pkglibdir)/$(SUBAGENT).so ; rm -f $(pkglibdir)/$(SUBAGENT).a ; fi
	mv $(pkglibdir)/$(SUBAGENT).so $(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(pkglibdir)/$(SUBAGENT).la
	rm -f $(libdir)/libnsm_$(SUBAGENT).so
	ln -s $(pkglibdir)/$(SUBAGENT).nsm $(libdir)/libnsm_$(SUBAGENT).so
