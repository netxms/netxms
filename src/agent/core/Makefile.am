EXTRA_DIST = \
    Makefile.nw nxagentd.def \
    nxagentd.dsw nxagentd.dsp \
    messages.mc \
    nxagentd.rc \
    win32.cpp resource.h \
	 nxagentd.h

INCLUDES=-I@top_srcdir@/include

bin_PROGRAMS = nxagentd
nxagentd_SOURCES = messages.c actions.cpp comm.cpp exec.cpp getparam.cpp log.cpp nxagentd.cpp session.cpp subagent.cpp sysinfo.cpp tools.cpp
nxagentd_LDADD = @PTHREAD_LIBS@ ../../libnetxms/libnetxms.la ../../libnxcscp/libnxcscp.la

messages.c: messages.h

messages.h: messages.mc
	$(PERL) @top_builddir@/tools/mc.pl $<
