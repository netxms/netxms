INCLUDES=-I@top_srcdir@/include

SUBAGENT = linux

pkglib_LTLIBRARIES = linux.la
linux_la_SOURCES = cpu.cpp disk.cpp drbd.cpp hddinfo.cpp iostat.cpp linux.cpp net.cpp proc.cpp system.cpp
linux_la_LDFLAGS = -module -avoid-version
linux_la_LIBADD = ../../../libnetxms/libnetxms.la

EXTRA_DIST = linux_subagent.h

install-exec-hook:
	mv $(pkglibdir)/$(SUBAGENT).so $(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(pkglibdir)/$(SUBAGENT).la
	rm -f $(libdir)/libnsm_$(SUBAGENT).so
	ln -s $(pkglibdir)/$(SUBAGENT).nsm $(libdir)/libnsm_$(SUBAGENT).so
