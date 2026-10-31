# Object Management Skill

This skill changes monitoring configuration of NetXMS objects: expected state of network interfaces and management status of objects. These are persistent configuration changes, not temporary suppressions - for a planned, time-limited activity use maintenance mode instead.

## Interface Expected State

Every interface has an expected state that NetXMS compares against its actual operational state when calculating interface status:

| Expected state | Interface operationally up | Interface operationally down |
|----------------|----------------------------|------------------------------|
| `up`           | Normal                     | Critical                     |
| `down`         | Critical                   | Normal                       |
| `ignore`       | Not used for status        | Not used for status          |
| `auto`         | Becomes `up` on next poll  | Becomes `down` on next poll  |

Administratively disabled interfaces are shown as disabled regardless of expected state. Typical uses:

- **Unused or spare ports** raising interface-down alarms: set to `ignore`, or to `down` if the port should stay down and coming up should be reported.
- **Ports intentionally shut down**: set to `down`.
- **Uplinks and other critical links**: keep (or set) `up`.
- **Re-baselining after recabling**: set to `auto` so the next status poll adopts the current state.

Changing expected state generates an event (`SYS_IF_EXPECTED_STATE_UP`, `_DOWN` or `_IGNORE`) and is recorded in the audit log. The new interface status is calculated on the next status poll; use `force-poll` with `pollType` `status` on the parent node if the user wants to see the effect immediately. Changing expected state does not terminate existing alarms - offer to resolve or terminate related interface-down alarms if appropriate.

### Identifying the interface

Interface names (for example `Gi0/1` or `eth0`) repeat across nodes. Identify the interface either by its object ID, or by name or alias together with the node. Use `find-objects` (class `Interface`, with the node as parent) or `get-object` to look up interfaces and check their current `expectedStateText` before changing it.

## Management Status

- `unmanage-object` stops status polling, configuration polling and data collection for an object. Unmanaged objects do not generate events or alarms.
- `manage-object` returns an object to normal monitoring. Decommissioned nodes cannot be set to managed.

Management status change applies to the object and all its child objects - unmanaging a container unmanages every node in it, and unmanaging a node unmanages all its interfaces. Prefer maintenance mode when the user wants to silence alerts for a limited time while keeping data collection running.

## Best Practices

- Confirm with the user before changing many objects at once (for example "all unused ports on a switch") - list the affected interfaces or objects first.
- Check current state with `get-object` before changing it, and report both old and new state.
- When the user's intent is "stop alerting for now", suggest maintenance mode; when it is "this port/device is not supposed to be monitored this way", use expected state or management status.
- Unmanaging a container affects all nodes in it - make sure the user understands the scope.
