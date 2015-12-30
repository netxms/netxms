SUBAGENT = linux

pkglib_LTLIBRARIES = linux.la
linux_la_SOURCES = cpu.cpp disk.cpp drbd.cpp hddinfo.cpp iostat.cpp linux.cpp net.cpp packages.cpp proc.cpp system.cpp
linux_la_CPPFLAGS=-I@top_srcdir@/include
linux_la_LDFLAGS = -module -avoid-version -export-symbols ../platform-subagent.sym
linux_la_LIBADD = ../../libnxagent/libnxagent.la ../../../libnetxms/libnetxms.la

EXTRA_DIST = linux_subagent.h

if !STATIC_BUILD
install-exec-hook:
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
endif
