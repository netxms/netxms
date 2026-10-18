#!/bin/sh
#
# Script for linking static Linux agent
#
# libstdc++ and libgcc are linked statically so the resulting binary depends
# on glibc only (libc, libm, libpthread, libdl). Regular expression library
# is chosen to match what configure selected (PCRE2 or legacy PCRE).
#

if grep -q '^#define USE_PCRE2 1' ../../../config.h; then
	REGEX_LIBS="-lpcre2-8 -lpcre2-32"
else
	REGEX_LIBS="-lpcre -lpcre32"
fi

g++ -g -O2 -fno-rtti -fno-exceptions -static-libstdc++ -static-libgcc -Wl,--export-dynamic -o nxagentd *.o -L/opt/pcre/lib -L/opt/openssl/lib -L/opt/libcurl/lib -L/opt/zlib/lib -L/opt/expat/lib -L/usr/local/lib ../../../src/db/libnxdb/.libs/libnxdb.a ../../../src/libnetxms/.libs/libnetxms.a ../subagents/filemgr/.libs/filemgr.a ../subagents/logwatch/.libs/logwatch.a ../../libnxlp/.libs/libnxlp.a ../subagents/ping/.libs/ping.a ../subagents/ups/.libs/ups.a ../subagents/ds18x20/.libs/ds18x20.a ../subagents/linux/.libs/linux.a ../../snmp/libnxsnmp/.libs/libnxsnmp.a ../libnxagent/.libs/libnxagent.a ../libnxsde/.libs/libnxsde.a -Wl,-whole-archive ../../db/dbdrv/.libs/sqlite.a -Wl,-no-whole-archive ../../ethernetip/libethernetip/.libs/libethernetip.a ../../libnetxms/.libs/libnetxms.a ../../jansson/.libs/libnxjansson.a ../../sqlite/.libs/libnxsqlite.a -Wl,-Bstatic -lcurl -lssl -lcrypto -lz $REGEX_LIBS -lexpat -Wl,-Bdynamic -lpthread -ldl
