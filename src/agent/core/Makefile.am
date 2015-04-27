AM_CPPFLAGS=-I@top_srcdir@/include
bin_PROGRAMS = nxagentd
nxagentd_SOURCES = messages.c actions.cpp appagent.cpp comm.cpp config.cpp \
                   ctrl.cpp epp.cpp exec.cpp extagent.cpp getparam.cpp \
                   master.cpp nxagentd.cpp policy.cpp push.cpp register.cpp \
                   sa.cpp sd.cpp session.cpp snmpproxy.cpp snmptrapproxy.cpp \
                   static_subagents.cpp subagent.cpp sysinfo.cpp tools.cpp \
                   trap.cpp upgrade.cpp watchdog.cpp
if USE_INTERNAL_EXPAT
nxagentd_LDADD = ../appagent/libappagent.la @top_srcdir@/src/db/libnxdb/libnxdb.la @top_srcdir@/src/libnetxms/libnetxms.la @top_srcdir@/src/snmp/libnxsnmp/libnxsnmp.la @top_srcdir@/src/libexpat/libexpat/libnxexpat.la @SUBAGENT_LIBS@
else
nxagentd_LDADD = ../appagent/libappagent.la @top_srcdir@/src/db/libnxdb/libnxdb.la @top_srcdir@/src/libnetxms/libnetxms.la @top_srcdir@/src/snmp/libnxsnmp/libnxsnmp.la @SUBAGENT_LIBS@ 
endif
if USE_ENCRYPTION
nxagentd_LDADD += -lssl
endif
if STATIC_BUILD
if ALL_STATIC
nxagentd_LDFLAGS = -all-static
else
nxagentd_LDFLAGS = -static
endif
endif
BUILT_SOURCES = messages.h
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
    service.cpp
