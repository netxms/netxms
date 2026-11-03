---
worth: yes
where: src/client/nxmc/java/src/rwt/java/org/netxms/nxmc/modules/objects/views/helpers/SshTerminalSocketEndpoint.java:128
added: 2026-11-03
---
# web SSH terminal and VNC websockets close after 30 s of inactivity

`SshTerminalSocketEndpoint` and `VncSocketEndpoint` never set `Session.setMaxIdleTimeout()`, and
`resources/terminal/terminal.html` / `resources/vncviewer/vnc.html` send no keepalive, so Jetty's default
30 s websocket idle timeout applies. A terminal left idle at a shell prompt closes with 1001 "Connection
Idle Timeout" and the endpoint tears down the SSH transport. Affects every deployment (Windows installer
jetty-base and Docker image both use defaults), not just one packaging.

Found by a codex review of the netxms/docker#7 fix (enabling `ee10-websocket-jakarta`), which reproduced the
close at 30.11 s against the 7.0.2 WAR. Fix in the product: set the idle timeout in `@OnOpen`, or add a
client-side ping, rather than per-deployment `WebSocketUpgradeFilter` config.
