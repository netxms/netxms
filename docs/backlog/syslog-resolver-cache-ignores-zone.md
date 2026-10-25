---
worth: yes
where: src/server/core/syslogd.cpp:640
added: 2026-10-25
---
# syslog resolver cache key ignores zone

`FindNodeByHostname()` caches the hostname → node ID result keyed by hostname only, but the DNS fallback
resolves through `FindNodeByIP(zoneUIN, ...)`. With zones enabled and `AF_TRAP_SOURCES_IN_ALL_ZONES` off,
the same hostname arriving from two zones returns whichever zone's node was cached first, for the cache TTL.

The same cache also only erases an expired entry when that hostname is looked up again (`:606-616`), so
one-shot hostnames stay until the cache is invalidated by a config change. Bounded by the number of distinct
sender hostnames, so a minor memory issue unless senders are spoofed. Surfaced during the server memory
review with Codex.
