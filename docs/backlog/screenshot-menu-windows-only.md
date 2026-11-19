---
worth: later
where: src/client/nxmc/java/src/main/java/org/netxms/nxmc/modules/objects/ObjectContextMenuManager.java:967
added: 2026-11-19
---
# Take screenshot is offered only for Windows nodes although Linux session agents support it

The node context menu adds _Take screenshot_ only when `node.getPlatformName().startsWith("windows-")`.
The Linux session agent (`nxsagent`, package `netxms-agent-session`) takes screenshots through X11 or the
GNOME Shell screenshot D-Bus interface, and the agent serves `CMD_TAKE_SCREENSHOT` from it, so a Linux node
with a session agent can be captured with `nxget -E` or the REST `take-screenshot` endpoint but not from the
console. `ScreenshotView` already picks the `Console` session or the first row of `Agent.SessionAgents`,
which would also work for Linux (session name = display, e.g. `:0`).

Open question: show the action when the node reports `Agent.SessionAgentCount > 0` (or for Linux too), or
keep it Windows-only on purpose (Wayland support limited to GNOME).

The admin guide (`agents/session-agent.adoc` in netxms-doc-antora, "Screenshots") documents the current
Windows-only menu; update it when this changes.
