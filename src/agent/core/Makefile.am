bin_PROGRAMS = nxagentd
nxagentd_SOURCES = actions.cpp appagent.cpp bkgnd_metrics.cpp certinfo.cpp comm.cpp \
		   config.cpp ctrl.cpp datacoll.cpp dcsnmp.cpp dbupgrade.cpp event.cpp exec.cpp \
		   extagent.cpp extdp.cpp filemon.cpp hddinfo.cpp localdb.cpp master.cpp \
		   metrics.cpp modbus.cpp nproc.cpp nxagentd.cpp package.cpp policy.cpp \
		   problems.cpp proxy.cpp push.cpp register.cpp sa.cpp session.cpp snmpproxy.cpp \
                   snmptrapproxy.cpp static_subagents.cpp subagent.cpp \
                   sysinfo.cpp syslog.cpp systemd.cpp tcpproxy.cpp tftp.cpp \
                   tools.cpp tunnel.cpp watchdog.cpp websvc.cpp

nxagentd_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build @LIBJQ_CPPFLAGS@ @LIBMODBUS_CPPFLAGS@

nxagentd_LDFLAGS = @EXEC_LDFLAGS@ @LIBJQ_LDFLAGS@ @LIBMODBUS_LDFLAGS@
if STATIC_BUILD
if ALL_STATIC
nxagentd_LDFLAGS += -all-static
else
nxagentd_LDFLAGS += -static
endif
endif

nxagentd_LDADD = ../../appagent/libappagent.la ../libnxagent/libnxagent.la @top_srcdir@/src/agent/libnxsde/libnxsde.la @top_srcdir@/src/db/libnxdb/libnxdb.la @top_srcdir@/src/libnetxms/libnetxms.la @top_srcdir@/src/snmp/libnxsnmp/libnxsnmp.la @AGENT_LIBS@ @SUBAGENT_LIBS@ @EXEC_LIBS@

if USE_INTERNAL_JANSSON
nxagentd_LDADD += @top_srcdir@/src/jansson/libnxjansson.la
else
nxagentd_LDADD += -ljansson
endif

if USE_LIBJQ
nxagentd_LDADD += -ljq
endif

CLEANFILES = static_subagents.cpp

static_subagents.cpp: @top_srcdir@/config.h Makefile
	@top_builddir@/tools/create_ssa_list.sh "@STATIC_SUBAGENT_LIST@" > static_subagents.cpp

EXTRA_DIST = \
    localdb.h \
    messages.mc \
    nxagentd.vcxproj nxagentd.vcxproj.filters \
    nxagentd.h \
    nxagentd.manifest \
    nxagentd.rc \
    pe_cert.cpp \
    resource.h \
    service.cpp
