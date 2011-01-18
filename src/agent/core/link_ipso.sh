#!/bin/sh
gcc -o nxagentd -nostdlib /usr/lib/crt0.o *.o ../subagents/ecs/.libs/libnsm_ecs.a ../subagents/ipso/.libs/libnsm_ipso.a ../subagents/logwatch/.libs/libnsm_logwatch.a ../subagents/ping/.libs/libnsm_ping.a ../subagents/portCheck/.libs/libnsm_portCheck.a ../subagents/ups/.libs/libnsm_ups.a ../../libnxlp/.libs/libnxlp.a ../../libnetxms/.libs/libnetxms.a ../../libexpat/libexpat/.libs/libnxexpat.a /usr/lib/libgcc.a -lkvm /usr/lib/libc.so.2.2 -lnet -lpth
strip nxagentd
