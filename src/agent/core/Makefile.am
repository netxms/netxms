EXTRA_DIST = \
    Makefile.nw nxagentd.def \
    nxagentd.dsw nxagentd.dsp \
    messages.mc \
    nxagentd.rc \
    win32.cpp resource.h \
    nxagentd.h \
    hddinfo.cpp

INCLUDES=-I@top_srcdir@/include

bin_PROGRAMS = nxagentd
nxagentd_SOURCES = messages.c actions.cpp comm.cpp config.cpp exec.cpp getparam.cpp log.cpp lpp.cpp nxagentd.cpp session.cpp static_subagents.cpp subagent.cpp sysinfo.cpp tools.cpp trap.cpp upgrade.cpp
nxagentd_LDADD = @PTHREAD_LIBS@ ../../libnetxms/libnetxms.la ../../libnxcscp/libnxcscp.la @SUBAGENT_LIBS@
AM_CPPFLAGS = $(all_includes) -DPREFIX=\"@prefix@\"
CLEANFILES = messages.c messages.h static_subagents.cpp

messages.c: messages.h

messages.h: messages.mc
	$(PERL) @top_builddir@/tools/mc.pl messages.mc

static_subagents.cpp: @top_srcdir@/config.h
	@top_builddir@/tools/create_ssa_list.sh "@STATIC_SUBAGENT_LIST@" > static_subagents.cpp
