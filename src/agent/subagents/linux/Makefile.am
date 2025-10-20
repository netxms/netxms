SUBAGENT = linux

pkglib_LTLIBRARIES = linux.la
linux_la_SOURCES =  count_ranges.cpp cpu.cpp disk.cpp drbd.cpp hardware.cpp hypervisor.cpp \
                   iostat.cpp linux.cpp net.cpp packages.cpp proc.cpp rapl.cpp services.cpp \
		   system.cpp wireguard.c
linux_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/build
linux_la_LDFLAGS = -module -avoid-version -export-symbols ../platform-subagent.sym
linux_la_LIBADD = ../../libnxagent/libnxagent.la ../../../libnetxms/libnetxms.la @SYSTEMD_LIBS@

EXTRA_DIST = cpu.h linux_subagent.h wireguard.h

if !STATIC_BUILD
install-exec-hook:
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
endif
