---
worth: later
where: src/server/core/dctarget.cpp
added: 2026-11-17
---
# SYS_MAINTENANCE_MODE_LEFT handlers cannot rely on the node state

`DataCollectionTarget::leaveMaintenanceMode` posts `SYS_MAINTENANCE_MODE_LEFT`, then restores `m_state` from
`m_stateBeforeMaintenance` and forces a status poll if they differ. Events are processed on another thread and
NXSL `$node.state` reads the live value, so an EPP filter on this event can see any of three values.

Status polls keep updating `m_state` during maintenance. A node that was up on entry and went down during
maintenance is down before the restore, up right after it (the stale entry snapshot), and down again after the
forced poll. So just posting the event after the restore, as this item first suggested, only makes the filter
see the stale snapshot. The restore also exists to replay the down/up transition after maintenance, so it
cannot simply be removed.

Unknown that settles it: which state a "re-arm escalation if still down" rule should see. "State observed at
exit" means capturing it as an event parameter before the restore; "freshly checked state" means defining an
ordering with the forced poll. Until then the follow-up `SYS_NODE_*` events from the forced poll are the
reliable signal.

Found while documenting netxms/netxms-doc#78 in netxms-doc-antora.
