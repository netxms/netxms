#!/bin/sh
gcc -o nxagentd -nostdlib /usr/lib/crt0.o *.o ../subagents/ipso/.libs/libnsm_ipso.a ../subagents/ping/.libs/libnsm_ping.a ../subagents/portCheck/.libs/libnsm_portCheck.a ../subagents/ups/.libs/libnsm_ups.a ../../libnxcscp/.libs/libnxcscp.a ../../libnetxms/.libs/libnetxms.a /usr/lib/libgcc.a /usr/lib/libc_r.a -lkvm /usr/lib/libc.so.2.2
strip nxagentd
