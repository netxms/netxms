---
worth: yes
where: contrib/nxagentd.conf-dist
added: 2026-11-05
---
# nxagentd.conf-dist misstates SessionIdleTimeout default

The `SessionIdleTimeout` section says "Default value is 60 seconds" and shows `SessionIdleTimeout = 60` as the example.
The code default is `g_dwIdleTimeout = 120` in `src/agent/core/nxagentd.cpp`, replaced on BSD/macOS by
`net.inet.tcp.keepinit` / 1000 + 15 (90 s with the stock 75 s keepinit).

Matters since issue #3876: the server keeps idle TCP proxy / SSH terminal agent connections alive with `nop()` every
`Client.KeepAliveInterval` (60 s default) for console sessions and every 30 s for WebAPI sessions. An admin copying the
example gets an agent timeout equal to the console keepalive period, and the drop comes back intermittently. Fix: state
the real default and the BSD/macOS rule, and use an example above 60 or note it must exceed `Client.KeepAliveInterval`.
Surfaced by revmux round 02 on issue-3876 as pre-existing.
