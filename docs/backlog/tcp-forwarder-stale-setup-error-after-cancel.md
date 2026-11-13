---
worth: later
where: src/client/nxmc/java/src/main/java/org/netxms/nxmc/modules/objecttools/TcpPortForwarder.java
added: 2026-11-13
---
# cancelled forwarder's pending proxy setup can post its error into a later session

`TcpPortForwarder.close()` closes the listener and registered sessions, but not a connection that is still
inside `session.setupTcpProxy(...)` in the accept loop. That call runs to completion or timeout. If it then
fails, the loop posts "session setup error" straight to the message area passed to `setMessageArea`, without
checking whether the forwarder was closed. In the desktop Remote Control view: Disconnect during proxy setup →
reconnect → old setup times out → the old attempt's error appears over the new, working session.

Surfaced by the codex review of issue #3923. Cosmetic: the session itself is unaffected. Open question that
settles the value: whether anyone hits it in practice (needs a slow or unreachable agent and a quick retry).
If fixed, skip the message once the forwarder is closed, or make the message area attempt-aware.
