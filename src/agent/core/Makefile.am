INCLUDES=-I@top_srcdir@/include

bin_PROGRAMS = nxagentd
nxagentd_SOURCES = messages.c dload.cpp getparam.cpp log.cpp nxagentd.cpp subagent.cpp sysinfo.cpp tools.cpp
nxagentd_LDADD = ../../libnetxms/libnetxms.la ../../libnxcscp/libnxcscp.la

messages.c: messages.mc
	$(PERL) @top_builddir@/tools/mc.pl $<
