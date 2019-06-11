bin_PROGRAMS = nxagentd
nxagentd_SOURCES = messages.c actions.cpp appagent.cpp comm.cpp config.cpp \
                   ctrl.cpp datacoll.cpp dcsnmp.cpp dbupgrade.cpp epp.cpp \
                   exec.cpp extagent.cpp getparam.cpp localdb.cpp master.cpp \
                   nxagentd.cpp policy.cpp proxy.cpp push.cpp register.cpp sa.cpp \
                   session.cpp snmpproxy.cpp snmptrapproxy.cpp \
                   static_subagents.cpp subagent.cpp sysinfo.cpp syslog.cpp \
                   systemd.cpp tcpproxy.cpp tools.cpp trap.cpp tunnel.cpp \
                   upgrade.cpp watchdog.cpp

nxagentd_CPPFLAGS = -I@top_srcdir@/include

nxagentd_LDFLAGS = @EXEC_LDFLAGS@
if STATIC_BUILD
if ALL_STATIC
nxagentd_LDFLAGS += -all-static
else
nxagentd_LDFLAGS += -static
endif
endif

if USE_INTERNAL_EXPAT
nxagentd_LDADD = ../../appagent/libappagent.la ../libnxagent/libnxagent.la @top_srcdir@/src/db/libnxdb/libnxdb.la @top_srcdir@/src/libnetxms/libnetxms.la @top_srcdir@/src/snmp/libnxsnmp/libnxsnmp.la @top_srcdir@/src/libexpat/libexpat/libnxexpat.la @AGENT_LIBS@ @SUBAGENT_LIBS@ @EXEC_LIBS@
else
nxagentd_LDADD = ../../appagent/libappagent.la ../libnxagent/libnxagent.la @top_srcdir@/src/db/libnxdb/libnxdb.la @top_srcdir@/src/libnetxms/libnetxms.la @top_srcdir@/src/snmp/libnxsnmp/libnxsnmp.la @AGENT_LIBS@ @SUBAGENT_LIBS@ @EXEC_LIBS@
endif

BUILT_SOURCES = messages.h
CLEANFILES = messages.c messages.h static_subagents.cpp

messages.c: messages.h

messages.h: messages.mc
	$(PERL) @top_builddir@/tools/mc.pl messages.mc

static_subagents.cpp: @top_srcdir@/config.h Makefile
	@top_builddir@/tools/create_ssa_list.sh "@STATIC_SUBAGENT_LIST@" > static_subagents.cpp

EXTRA_DIST = \
    Makefile.w32 \
    nxagentd.vcxproj nxagentd.vcxproj.filters \
    messages.mc messages.h messages.c \
    nxagentd.rc \
    resource.h \
    nxagentd.h \
    hddinfo.cpp \
    service.cpp \
    localdb.h
