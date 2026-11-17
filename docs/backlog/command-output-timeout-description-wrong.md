---
worth: yes
where: sql/setup.in
added: 2026-11-17
---
# Server.CommandOutputTimeout description says "local command object tool"

The description of `Server.CommandOutputTimeout`, which the server configuration view shows, reads "Time (in seconds) to wait for output of a local command object tool." That is wrong on two counts:

- Local Command tools run on the client and never use it. The value is sent to the client at login (`ClientSession` login response, `VID_SERVER_COMMAND_TIMEOUT`) and is applied only in `NXCSession.executeServerCommand` when output is received, that is to **Server Command** tools. Agent Command tools use the 60 s `MessageHandler` default.
- It is an idle timeout. Each output message resets it (`setLastMessageTimestamp` in `NXCSession`). It bounds only the client's wait: `ServerCommandExecutor` in `objtools.cpp` has no timeout, and the console does not stop the command when the wait expires.

Suggested text: "Time (in seconds) the management client waits for the next output of a server command object tool. The server does not stop the command when this time expires."

The fix changes `sql/setup.in` and needs an nxdbmgr upgrade step to update the stored description (the old text was set in `upgrade_v36.cpp` and `upgrade_v40.cpp`). That makes it a schema-level change on every branch it goes to; see the netxms-db-backport skill. The docs were corrected in netxms-doc#77.
