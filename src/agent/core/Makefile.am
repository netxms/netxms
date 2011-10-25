INCLUDES=-I@top_srcdir@/include

bin_PROGRAMS = nxagentd
nxagentd_SOURCES = messages.c actions.cpp comm.cpp config.cpp epp.cpp exec.cpp \
                   extagent.cpp getparam.cpp nxagentd.cpp policy.cpp push.cpp \
                   register.cpp session.cpp snmpproxy.cpp static_subagents.cpp \
                   subagent.cpp sysinfo.cpp tools.cpp trap.cpp upgrade.cpp \
                   watchdog.cpp
if USE_INTERNAL_EXPAT
nxagentd_LDADD = @top_srcdir@/src/libnetxms/libnetxms.la @top_srcdir@/src/db/libnxdb/libnxdb.la @top_srcdir@/src/libexpat/libexpat/libnxexpat.la @SUBAGENT_LIBS@
else
nxagentd_LDADD = @top_srcdir@/src/libnetxms/libnetxms.la @top_srcdir@/src/db/libnxdb/libnxdb.la @SUBAGENT_LIBS@
endif
AM_CPPFLAGS = $(all_includes) -DPREFIX=\"@prefix@\"
CLEANFILES = messages.c messages.h static_subagents.cpp

messages.c: messages.h

messages.h: messages.mc
	$(PERL) @top_builddir@/tools/mc.pl messages.mc

static_subagents.cpp: @top_srcdir@/config.h Makefile
	@top_builddir@/tools/create_ssa_list.sh "@STATIC_SUBAGENT_LIST@" > static_subagents.cpp

EXTRA_DIST = \
    nxagentd.vcproj \
    messages.mc messages.h messages.c \
    nxagentd.rc \
    win32.cpp resource.h \
    nxagentd.h \
    hddinfo.cpp \
    netinfo.cpp \
    service.cpp
