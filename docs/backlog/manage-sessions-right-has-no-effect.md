---
worth: later
where: include/nxcldefs.h:982; src/client/nxmc/java/src/main/java/org/netxms/nxmc/modules/users/propertypages/SystemRights.java:103
added: 2026-11-26
---
# "Control user sessions" system right has no effect

67a95978da (7.0.5: 8b4026c1d3) removed `CMD_GET_SESSION_LIST` and `CMD_KILL_SESSION` with their
server handlers, the only code that checked `SYSTEM_ACCESS_MANAGE_SESSIONS`. Nothing in `src/server`
or `private/netxms` tests the bit any more, but nxmc still offers "Control user sessions" in the
system rights page and both ACL reports, and the test server still grants it.

Either drop the right from the client (keep the bit reserved in `nxcldefs.h` and
`UserAccessRights.java` so stored masks stay valid) or give it a job again. The admin guide's
`server/user-management-reference.adoc` system rights table describes it as having no effect;
update that row when this is settled.
