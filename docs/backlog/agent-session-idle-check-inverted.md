---
worth: yes
where: src/agent/core/session.cpp
added: 2026-11-05
---
# Agent session read-timeout idle check is inverted

`CommSession::readThread` on `MSGRECV_TIMEOUT` disconnects when `m_timestamp + g_dwIdleTimeout * 1000 > GetMonotonicClockTime()`,
i.e. when the session was active *recently*. The comparison flipped when 76e84ee573 switched from `time()` to the
monotonic clock (the old form was `m_ts < time(nullptr) - g_dwIdleTimeout`).

Normally harmless: the read times out only after `g_dwIdleTimeout + 1` seconds without inbound messages, so the
timestamp is already older than the limit and the check falls through, and `SessionWatchdog` in `comm.cpp` does the
real idle disconnect. It bites when something else refreshes the timestamp without an inbound message — the file
send progress callbacks call `updateTimeStamp()`, so a long file transfer to a server that does not answer keepalives
can be cut by this "timeout". Fix is flipping the comparison to `<`.
