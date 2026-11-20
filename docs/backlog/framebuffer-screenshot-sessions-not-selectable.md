---
worth: later
where: src/client/nxmc/java/src/rwt/java/org/netxms/nxmc/modules/objects/views/ScreenshotView.java
added: 2026-11-19
---
# Framebuffer screenshot sessions cannot be selected from the console

Framebuffer screenshots (`fbdev.nsm`, session names `fb0`..`fb7`) are registered as screenshot providers, not
session agents, so they never appear in `Agent.SessionAgents` and `ScreenshotView` cannot pick them. Today they
are reachable only through `nxget -E` or the REST `take-screenshot` endpoint with `sessionName=fb0`
(`agents/session-agent.adoc`, "Framebuffer Screenshots", in netxms-doc-antora).

Open question: how the console should discover screenshot providers that are not session agents (a separate
list metric, or registering framebuffers alongside session agents), and how the user picks one.

The Linux session-agent half of this item (menu hidden for non-Windows nodes) is #4025.
