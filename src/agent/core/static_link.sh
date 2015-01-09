#!/bin/sh
#
# Script for linking static Linux agent
#

g++ -g -O2 -fno-rtti -fno-exceptions -o nxagentd messages.o actions.o appagent.o comm.o config.o ctrl.o epp.o exec.o extagent.o getparam.o master.o nxagentd.o policy.o push.o register.o sa.o sd.o session.o snmpproxy.o snmptrapproxy.o static_subagents.o subagent.o sysinfo.o tools.o trap.o upgrade.o watchdog.o -Wl,-Bstatic -Wl,-Bdynamic  -L/usr/local/lib ../appagent/.libs/libappagent.a ../../../src/db/libnxdb/.libs/libnxdb.a ../../../src/libnetxms/.libs/libnetxms.a ../../../src/libexpat/libexpat/.libs/libnxexpat.a ../subagents/ecs/.libs/ecs.a ../subagents/filemgr/.libs/filemgr.a ../subagents/logwatch/.libs/logwatch.a ../../libnxlp/.libs/libnxlp.a ../subagents/ping/.libs/ping.a ../subagents/portCheck/.libs/portcheck.a ../subagents/ups/.libs/ups.a ../subagents/linux/.libs/linux.a ../../snmp/libnxsnmp/.libs/libnxsnmp.a ../../libnetxms/.libs/libnetxms.a ../../libexpat/libexpat/.libs/libnxexpat.a ../../libtre/.libs/libnxtre.a -ldl -lpthread -Wl,-Bstatic -lssl -lcrypto -lz -Wl,-Bdynamic
