---
worth: maybe
added: 2026-11-27
---
# small robustness gaps around schema tables

Found while writing the data dictionary (master fc2a1b0ed8). Each is reachable only in an unusual
setup; none is a user-visible bug with the shipped clients.

- `nxdbmgr unlock` clears the HA lease but not `holder_address` (`unlock.cpp:51`).
- NXCP AI disabled-list update does not validate `item_type` (`session.cpp:21225-21234`); WebAPI does.
- `*.EnableStorage` read with fallback false (`otellog.cpp:539`, `syslogd.cpp:1286`,
  `winevent.cpp:608`) while setup and upgrades create them as 1.
- `server_action_execution_log.success` is 1 for notification and forward actions even when the
  channel or target name is empty (`actions.cpp:615,645`).
- Timed acknowledgement expires only with the sticky bit (`alarm.cpp:2722`); the console always sends
  sticky, a custom NXCP client may not. Force sticky when a time is given.
- `TimeFrame.java:385` `date |= 0XFFFFFFFF` sign-extends to all 64 bits; the console never sends an
  empty day list, third-party `netxms-client` users can. Use `0xFFFFFFFFL`.
- `network_map_deleted_nodes` save is guarded by `!m_seedObjects.isEmpty()` instead of the deleted list
  (`netmap.cpp:465`).
- `DCTable::checkThresholds` ignores `DCF_ALL_THRESHOLDS` for already active thresholds
  (`dctable.cpp:510-511`); the flag cannot be set on table DCIs today.
- AI operator checks persist `last_run`/`run_count` only when the verdict changes
  (`ai_operator.cpp:1493-1526`); counters revert after a restart.
