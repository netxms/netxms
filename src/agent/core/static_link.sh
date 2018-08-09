#!/bin/sh
#
# Script for linking static Linux agent
#

g++ -g -O2 -fno-rtti -fno-exceptions -Wl,--export-dynamic -o nxagentd *.o -L/usr/local/lib ../../appagent/.libs/libappagent.a ../../../src/db/libnxdb/.libs/libnxdb.a ../../../src/libnetxms/.libs/libnetxms.a ../../../src/libexpat/libexpat/.libs/libnxexpat.a ../subagents/ecs/.libs/ecs.a ../subagents/filemgr/.libs/filemgr.a ../subagents/logwatch/.libs/logwatch.a ../../libnxlp/.libs/libnxlp.a ../subagents/ping/.libs/ping.a ../subagents/portCheck/.libs/portcheck.a ../subagents/ups/.libs/ups.a ../subagents/ds18x20/.libs/ds18x20.a ../subagents/linux/.libs/linux.a ../../snmp/libnxsnmp/.libs/libnxsnmp.a ../libnxagent/.libs/libnxagent.a -Wl,-whole-archive ../../db/dbdrv/sqlite/.libs/sqlite.a -Wl,-no-whole-archive ../../libnetxms/.libs/libnetxms.a ../../libexpat/libexpat/.libs/libnxexpat.a ../../libtre/.libs/libnxtre.a ../../jansson/.libs/libnxjansson.a ../../zlib/.libs/libnxzlib.a ../../sqlite/.libs/libnxsqlite.a -ldl -lpthread -L/opt/openssl/lib -Wl,-Bstatic -lssl -lcrypto -lz -Wl,-Bdynamic
