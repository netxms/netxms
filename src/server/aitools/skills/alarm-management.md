# Alarm Management Skill

This skill provides alarm lifecycle management for NetXMS. Use it to acknowledge, resolve, and terminate alarms, add operator comments, and inspect full alarm details. It is the action counterpart to the `alarm-list` function, which lists currently active alarms together with their IDs.

## Key Capabilities

- **Inspect an alarm**: `get-alarm-details` returns the full state of one active alarm — state, current and original severity, source object and its status, timestamps, repeat count, originating event processing rule, helpdesk linkage, the complete operator comment history, and recent related events.
- **Acknowledge alarms**: `acknowledge-alarm` marks one or more alarms as seen by an operator. Supports sticky acknowledgment (stays acknowledged even if the condition repeats) with an optional auto-revert timeout.
- **Resolve alarms**: `resolve-alarm` marks one or more alarms as handled. The alarm stays in the active list and is automatically re-activated if the originating condition occurs again.
- **Terminate alarms**: `terminate-alarm` removes one or more alarms from the active list. The alarm will not reappear unless it is raised again.
- **Comment on alarms**: `add-alarm-comment` appends an operator note to an alarm's comment history.

## Alarm States

- **Outstanding** — newly raised, not yet seen by an operator. Only outstanding alarms can be acknowledged.
- **Acknowledged** — an operator is aware of it; still active. May be sticky (survives condition repeats) and may carry an acknowledgment timeout after which it reverts to outstanding.
- **Resolved** — the underlying problem is considered fixed, but the alarm remains tracked and will re-activate if the condition recurs.
- **Terminated** — closed and removed from the active alarm list. Terminated alarms are not retained in memory; `get-alarm-details` and the action tools work only on alarms that are still in the active list.

## Resolve vs Terminate

- Use **resolve** when the underlying problem has been fixed but you want NetXMS to keep watching — if the same condition reappears the alarm comes back automatically. Resolve requires the "update alarms" right on the source object.
- Use **terminate** to permanently clear an alarm (stale, no longer relevant, or a one-off you have fully dealt with). Terminate requires the "terminate alarms" right on the source object. An alarm that is open in an external helpdesk system cannot be terminated until the helpdesk issue is closed.

## Bulk Operations

`acknowledge-alarm`, `resolve-alarm`, and `terminate-alarm` accept either a single alarm ID or an array of IDs in the `alarms` argument. They process every ID and return a structured result:

```json
{ "requested": 3, "succeeded": 2, "failed": 1,
  "results": [
    { "alarm_id": 101, "status": "ok" },
    { "alarm_id": 102, "status": "ok" },
    { "alarm_id": 103, "status": "error", "reason": "access denied" } ] }
```

Always read the `results` array — a successful call can still contain per-alarm failures (invalid ID, access denied, alarm not outstanding, alarm open in helpdesk, etc.).

## Subordinate Alarms

Some alarms group related child ("subordinate") alarms under a root-cause alarm. By default acknowledge / resolve / terminate also apply to the subordinates. Set `include_subordinates` to false to act on only the named alarm.

## Workflow

1. Use `alarm-list` (optionally filtered by `object` and `state`) to find relevant alarms and their numeric IDs.
2. Use `get-alarm-details` on an alarm to review its severity, source object, history, and related events before acting.
3. Acknowledge to signal triage in progress; add a comment documenting what you found or did; resolve once the problem is fixed; terminate to clear stale or no-longer-applicable alarms.

## Best Practices

- Never guess alarm IDs — get them from `alarm-list` (or from incident details when working an incident).
- Acknowledge before investigating so other operators know the alarm is being handled; use a sticky acknowledgment with a timeout when you expect the condition to flap while you work.
- Add a comment whenever you take a non-obvious action — the comment history is the audit trail other operators see.
- Prefer resolve over terminate when the condition could recur and you want it tracked; terminate only when you are confident the alarm should disappear.
- When terminating a large batch you are not fully confident about, use `create-approval-request` to get the operator's sign-off first.
- Alarm actions taken through this skill are recorded as performed by the NetXMS server (the same as actions from event processing policy or NXSL scripts), not by an individual user account.
