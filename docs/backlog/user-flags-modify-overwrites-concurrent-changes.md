---
worth: later
where: src/client/nxmc/java/src/main/java/org/netxms/nxmc/modules/users/propertypages/General.java:226
added: 2026-10-30
---
# group properties dialog can revert another admin's flag changes

`General.java` builds the flag word from the object the dialog captured when it opened and sends it with
`MODIFY_FLAGS`. `NXCSession` replaces cached user objects on `USER_DB_OBJECT_MODIFIED` rather than updating
that captured instance, and `modifyFromMessage` (`userdb_objects.cpp:299`) overwrites all editable bits.

Admin A opens a group dialog, admin B disables the group or enables 2FA on it, and A's apply restores the old
values. Disabling a group or enforcing 2FA is security relevant, so a silent revert matters. Unknown that
decides this: whether admins edit the same group concurrently often enough to justify a set/clear mask in the
protocol. Reading a fresh cached object would narrow the window but not close it. The same full-mask write
exists on the user `Authentication` page.

Predates the group "close other sessions" checkbox, which uses the same path. Surfaced in review of that change.
