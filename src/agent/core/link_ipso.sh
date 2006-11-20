#!/bin/sh
gcc -o nxagentd -nostdlib /usr/lib/crt0.o *.o ../subagents/ipso/.libs/libnsm_ipso.a ../subagents/ping/.libs/libnsm_ping.a ../subagents/portCheck/.libs/libnsm_portCheck.a ../subagents/ups/.libs/libnsm_ups.a ../../libnetxms/.libs/libnetxms.a /usr/lib/libgcc.a -lkvm /usr/lib/libc.so.2.2 -lnet -lpth
strip nxagentd
