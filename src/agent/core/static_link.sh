#!/bin/sh
#
# Script for linking static Linux agent
#

g++ -g -O2 -fno-rtti -fno-exceptions -Wl,--export-dynamic -o nxagentd *.o -L/opt/pcre/lib -L/opt/openssl/lib -L/usr/local/lib ../../appagent/.libs/libappagent.a ../../../src/db/libnxdb/.libs/libnxdb.a ../../../src/libnetxms/.libs/libnetxms.a ../subagents/filemgr/.libs/filemgr.a ../subagents/logwatch/.libs/logwatch.a ../../libnxlp/.libs/libnxlp.a ../subagents/ping/.libs/ping.a ../subagents/ups/.libs/ups.a ../subagents/ds18x20/.libs/ds18x20.a ../subagents/linux/.libs/linux.a ../../snmp/libnxsnmp/.libs/libnxsnmp.a ../libnxagent/.libs/libnxagent.a ../libnxsde/.libs/libnxsde.a -Wl,-whole-archive ../../db/dbdrv/.libs/sqlite.a -Wl,-no-whole-archive ../../libnetxms/.libs/libnetxms.a ../../jansson/.libs/libnxjansson.a ../../sqlite/.libs/libnxsqlite.a -Wl,-Bstatic -lssl -lcrypto -lz -lpcre -lpcre32 -lexpat -Wl,-Bdynamic -lpthread -ldl
