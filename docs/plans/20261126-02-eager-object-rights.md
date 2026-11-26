# Effective rights in object data

## Overview

GH #4068, long-term fix. The server sends the session user's effective rights with every object it sends to the client, so `AbstractObject.getEffectiveRights()` no longer needs a network round-trip while the cached rights are current. This covers every caller, including the ones plan `completed/20261126-01-async-toolbar-rights-check.md` leaves synchronous (`ObjectContextMenuManager`, `DashboardView`, `PredefinedMapView`, `FloorPlanView`, `DeviceConfigBackupView`, `DataCollectionObjectEditor`). A new session notification keeps the rights current when group membership changes.

Master first. Porting it to stable-7.0 (task 4) is a separate release decision, made after it has settled on master. The wire change is an optional field plus a new notification code: no record layout change, no protocol version bump.

Depends on plan 01: the non-blocking cached read and the asynchronous toolbar fetch.

## Context

- `NetObj::fillMessage(msg, userId, full)` already receives the session user ID. The visibility decision (`checkAccessRights(m_userId, OBJECT_ACCESS_READ)`) is computed right next to serialization in pushed updates (`sendObjectUpdates`) and selected-object sync. `MaskNodeCredentials` checks rights again for each node. In full sync (`getObjects`), the filter runs over the object index first and the send loop runs afterwards.
- `NetObj::getUserRights` checks direct ACL entries and group membership on every call. It computes the inherited mask from the parents' caches and only afterwards stores it in its own cache. No lock covers both steps. `ClearAllObjectsInheritedAccessCache` sweeps objects in ID order, not tree order. So a concurrent lookup, or one that started before the change, can store a stale mask in an object the sweep has already cleared.
- Per-subtree clears also happen without any membership change: on ACL edits and on parent binding changes.
- Parent ACL edits already push updates for the whole subtree (`notifyClientsOnAccessChange`), and `sendObjectUpdates` rechecks read access at send time. But `onObjectChange` drops every change until the session's initial object sync has finished. An ACL edit during login sync is therefore never pushed.
- Group membership changes are not covered at all. `UpdateGlobalAccessRights` clears the server cache and refreshes session system rights, but nothing tells the client that object rights changed. The LDAP sync clears the cache on every run, even when nothing changed.
- Existing bug found in discovery: `SyncLDAPGroupMembers` sets `changed` when `isMember` is false, and `isMember` is false for every member of a disabled group. So each sync of a disabled LDAP group re-adds members that are already there and sends a spurious `USER_DB_MODIFY`.
- `NXCSession.reconnect()` logs in again but keeps the object cache. Notifications sent while it was disconnected are lost.

## Decisions

- **Fields:** rights go in the existing `VID_EFFECTIVE_RIGHTS` on object messages. A new `VID_RIGHTS_GENERATION` carries the generation on object messages. On the notification, the generation travels in the existing `VID_NOTIFICATION_DATA`, which the client already reads as 64 bits. That needs the message-building sender, because the convenience one takes 32 bits.
- **Server rights generation:** monotonic, server-wide, incremented by each membership-change batch.
  - Inherited cache entries are tagged with the generation and the object's own cache revision that were current when the calculation started. An entry with an older tag counts as missing. A result is stored only if both are still unchanged.
  - Bumping the generation is the invalidation for membership changes, so that path doesn't need the full-index clear sweep. Local ACL and binding clears bump only the object revision and keep their existing object updates.
  - An object message carries the generation that was current when its mask was computed. If the generation changed during the lookup, the mask is omitted, not retried.
- **Full sync:** if the generation changed between filter and send, `getObjects` rechecks visibility and full/partial eligibility before serializing. Access-change notifications that arrive during the sync are queued and sent after it, not dropped.
- **Invalidation scope:** one notification to all sessions per top-level operation that actually changed the membership graph. It goes through the same path that refreshes session system rights, so LDAP gets both. This covers nested groups and alternative membership paths conservatively, at no per-user traversal cost. Edits that change only `systemRights` keep their system-rights refresh without an object-rights notification. Ruled out for now: targeting only affected users through `CollectUserGroups` with per-user snapshots. It is more precise, but only worth building if measurement shows global invalidation hurts.
- **Client generation:**
  - Each object instance stores the generation of its mask, and a mask counts as current only when that generation equals the session's. Any advance of the session generation therefore invalidates every older mask, including instances held by views, without walking the object list.
  - Notifications and object messages advance the session generation through the same strictly-newer operation, so arrival order doesn't matter and duplicates are harmless.
  - Every (re)login resets the generation baseline and treats all cached masks as stale. That also covers server restart and HA failover, where the counter starts over.
- **Accepted consequence:** after a membership change, the server doesn't resend objects. The first synchronous rights check on each object (context menu, view open) does one round-trip until that object's next update. The visible toolbar refreshes asynchronously. On installations with frequent membership changes this partly brings back plan 01 behaviour for menus; revisit only if it shows up in practice.
- **No time limit on cached rights:** the notification replaces it. An interim 60-second bound was considered and dropped.
- **Unchanged:** arbitrary-user rights queries (`EffectiveRightsDialog`) still go to the server. No WebAPI change.

## Tandem

| Task | Writer | Reviewer |
|---|---|---|
| 1 | codex | claude |
| 2 | claude | codex |
| 3 | claude | codex |
| 4 | outside the tandem run (release decision) | |

## Implementation Steps

### Task 1: Generation-aware rights cache and membership-change notification

Layers: server (objects, user DB, LDAP sync, client session), NXCP constants (C++ and Java), client library (notification decoding).

A top-level operation "changed the membership graph" when any of these really happened:
- an `addUser` / `deleteUser` returned true: LDAP sync, `UpdateGroupMembership`, or removal of a deleted user from its groups
- the sorted diff in `Group::setMembers` changed the list
- a group was enabled or disabled, including by `RemoveDeletedLDAPEntries`
- a group was deleted

Nested calls report the change to the enclosing operation, so an LDAP sync that deletes several groups sends one notification. This task also fixes the `SyncLDAPGroupMembers` disabled-group bug: aggregate the real return values, not the `isMember` result. A no-op LDAP sync and a profile-only LDAP edit send nothing.

- [x] inherited cache entries tagged and validated as described in Decisions; the membership path no longer sweeps the object index
- [x] new notification code in `include/nxcldefs.h` and `SessionNotification.java`; `VID_RIGHTS_GENERATION` in `nms_cscp.h` and `NXCPCodes.java`; the client exposes the notification's generation
- [x] each listed change sends exactly one notification per top-level operation; no-op LDAP syncs send none
- [x] LDAP sync of a disabled group with unchanged members no longer sends `USER_DB_MODIFY`
- [x] integration test: adding a user to a group, removing them, and disabling the group each produce one notification with a higher generation; re-saving unchanged membership produces none. Gate: `mvn -f tests/integration/pom.xml test -Dtest=UserTest`

Verified with a community build and disposable SQLite and LDAP fixtures. All four `UserTest` cases and the native `test-libnxcore` suite passed. The connected LDAP client counted one rights notification when sync added a member, zero on unchanged and profile-only syncs, and zero group updates for an unchanged disabled group.

➕ All inherited entries in one ACL share the generation tag. A local clear increments its revision and drops the entries under the same lock. A native test checks that late cache writes from either an obsolete local revision or an obsolete generation preserve the current entry. LDAP group metadata updates now report edits from the current sync, because the existing persistence dirty flag can stay set across consecutive syncs.

### Task 2: Send effective rights with object data

Layers: server (objects, client session), client library (reading the field).

- Full and partial object messages carry `VID_EFFECTIVE_RIGHTS` for the session user, plus `VID_RIGHTS_GENERATION`. Removal markers carry neither.
- Where the visibility check sits next to serialization, its mask is reused, including by `MaskNodeCredentials`.
- For `getObjects`, measure on a large object database whether carrying masks from the filter beats a repeated `getUserRights`. Weigh it against the longer staleness window over a slow link. Record the result in this plan before choosing.
- The full-sync recheck and the queueing of access changes during sync are part of this task.
- `AbstractObject` takes rights from the message when the field is present (`isFieldPresent` + `getFieldAsInt64`; a present zero is authoritative) and stores its generation. Otherwise it keeps the lazy fetch, so it still works against older servers.

- [x] every full and partial object message to a client carries rights and generation, except when the mask was omitted after a race
- [x] an ACL change made during a session's login sync reaches that session
- [x] measured full-sync cost recorded here, with the choice made
- [x] integration test: after login, the non-blocking cached read from plan 01 returns rights for objects received in the sync without an RPC, matching an admin `getEffectiveRights(objectId, userId)` query, for a user with direct ACL rights and for one with rights only through a group. Gate: `mvn -f tests/integration/pom.xml test -Dtest=ObjectTest`

➕ Measurement: community build, SQLite, localhost, a group-only user syncing 5051 containers (50 × 100 under one ACL root), best of 7 syncs. Without the rights field: 445–447 ms. With it: 462–465 ms. That is about 17 ms, or 3.4 µs per object (3.8 %), and it shrinks relative to transfer time on a real link. Choice: `getObjects` keeps the early filter lookup and takes one more lookup at send time, which hits the inherited cache the filter just filled. Carrying the filter's mask through to sending was ruled out.

➕ One send-time sample per object: `NetObj::getAccessSnapshot()` returns the rights together with the rights generation, and whether the generation held across the lookup. `getObjects`, `getSelectedObjects` and `sendObjectUpdates` use that single snapshot for visibility, full/partial, `MaskNodeCredentials` and the outgoing field pair, which `fillMessage` now takes as a parameter. The pair is omitted when the snapshot isn't consistent. The parameter is justified because the caller's decision and the sent mask must come from the same lookup. The cost of `MaskNodeCredentials` on nodes is not measured, because the tree had no nodes.

➕ The client subscribes to `Core.Objects` only after `CMD_GET_OBJECTS` completes, so access changes could be lost up to the subscription, not only up to the end of the sync. A server-wide access change sequence is stamped on every object an access change notification covers. Stamps only increase (`AtomicStoreMax`), so overlapping notifications can't move one backwards. A full sync records the sequence at its start; overlapping syncs on one session keep the earliest outstanding start. Once the last running sync has ended and the objects channel is subscribed, whichever comes last, the server queues every object stamped since then. This happens once per outstanding baseline. It also replays changes that a re-sync of an already subscribed session could deliver ahead of the bulk data. The session keeps only the sequence number, so clients that never subscribe accumulate nothing, and `onObjectChange` is unchanged. Group membership changes aren't stamped: they are covered by the generation in the snapshot and by the rights notification from task 1. The handoff scan is one pass over the object index, run only when a stamp moved after the baseline. Its cost on a large installation is not measured.

➕ `AbstractObject` stores the generation in task 3, its first reader. The presence check stays even though the client CLAUDE.md rules out `isFieldPresent()` fallbacks for older servers: here the field is absent by design when the mask was omitted after a race. The protocol version bump already rules out older servers on master.

`ObjectTest.testAccessChangeBetweenSyncAndSubscription` covers the handoff. It uses network map groups because they aren't polled: the first autobind poll of a new container sends a regular update, which made a container-based version flaky. Against the previous revision of this task, the test fails on the resubscribe replay.

### Task 3: Client and console invalidation

Layers: client library, UI.

`NXCSession` tracks the session generation as described in Decisions. A lazy fetch records the generation at dispatch, and a completion from an older generation is discarded. The UI checks the generation again when it applies a result, because the same instance can survive an invalidation. On the notification, `ObjectsPerspective` refreshes the visible toolbar with the asynchronous fetch from plan 01.

- [x] with current masks, selecting objects and opening context menus send no `CMD_GET_EFFECTIVE_RIGHTS`
- [x] after a membership change, the visible toolbar follows without blocking, and later synchronous checks fetch once per object
- [x] an object carrying an older generation, delivered after the notification, doesn't restore a revoked toolbar action
- [x] after reconnect, no mask from before the disconnect is used
- [x] integration test: grant and then revoke a right through group membership only, on an object the user has not selected yet, with the user's system rights unchanged; the client's current rights follow both changes. Gate: `mvn -f tests/integration/pom.xml test -Dtest=UserTest`

➕ The session generation is an immutable `ObjectRightsVersion` held by `NXCSession`. Each strictly newer server generation replaces it with a new instance, whether it arrives in an object message (`acceptObjectRightsGeneration`) or in `OBJECT_RIGHTS_CHANGED`. So does every login, including reconnect. `AbstractObject` keeps its mask together with the instance it was valid for, and the mask is current only while that instance is the session's. That covers view-held instances and a server restart, where the counter starts over, without walking the object list. A pushed mask whose generation is older than the session's is dropped. Unit tests (`ObjectRightsVersionTest`, `mvn -f src/client/java/netxms-client/pom.xml test`) cover the ordering rules and the late old-generation object. The toolbar job records the version at dispatch, and on a moved version it repeats the request instead of applying the result, unless a newer request is already waiting.

➕ The integration test grants READ through one group and execute-script through a second group whose membership it toggles. A direct ACL entry for a user takes precedence over that user's group entries, so with one it would never see the group change. Without the client invalidation the test fails: the instance received at login keeps its READ-only mask.

➕ `AbstractObject.getEffectiveRights()` also discards the reply it returns when the version moved during the request. It makes at most three attempts in total, each checked against the version current when it started. If none of them holds, it returns 0 uncached, the same as when rights can't be obtained. The rendered toolbar is refreshed on `RECONNECT_COMPLETED` the same way as on `OBJECT_RIGHTS_CHANGED`, because notifications can be lost while disconnected.

⚠️ Reconnect and server restart are covered by code reading only: `login()` installs a fresh version before `CMD_LOGIN`, and the toolbar refresh on `RECONNECT_COMPLETED` is left for the user's final review (same object selected across a reconnect, no later update of it).

⚠️ One `UserTest` run timed out in `testValidatePassword` at login (41 s). The next two runs passed in full. The server log showed nothing beyond the fixture's driver load errors.

⚠️ The first two hand checks don't apply on master: a master console and a 7.0.x server differ in full protocol version (62 against 60) and refuse each other. They belong to the stable-7.0 port in task 4.

Verified by hand:
- new console against a 7.0.x server: rights still resolve through the lazy fetch, and the toolbar behaves as in plan 01 (task 4 only)
- 7.0.x console against the new server: unaffected (the extra field and notification are ignored) (task 4 only)
- server restart while the console stays open: after it reconnects, rights reflect the restarted server (left for the user's final review)

### Task 4: Port to stable-7.0

Layers: everything touched in tasks 1–3, on the stable-7.0 branch.

Only if the release decision is made after tasks 1–3 have been on master long enough to show no regressions.

- [ ] tasks 1–3 cherry-picked to stable-7.0; the new notification code and VID values identical to master
- [ ] integration tests from tasks 1–3 pass against a stable-7.0 server build
- [ ] mixed-version cases from task 3 re-checked between stable-7.0 and older 7.0.x releases
