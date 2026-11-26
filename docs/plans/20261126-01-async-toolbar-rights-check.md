# Async toolbar rights check and group state rights refresh

## Overview

GH #4068: the management console UI thread blocks on a synchronous `CMD_GET_EFFECTIVE_RIGHTS` round-trip when an object is selected or the selected object changes. This plan is the short-term fix for master and stable-7.0. The long-term fix (rights sent with object data) is plan `20261126-02-eager-object-rights.md`.

The two tasks are independent commits. Task 1 is console-only and works against unchanged 7.0.x servers, so it can ship as a console build on its own. Task 2 is a server change. Both land on master and are cherry-picked to stable-7.0 (the user DB code is practically identical there).

## Context

- Profile of a 7.0.5.2 desktop console over an SSH-tunnelled link (NXCP round-trip 133–545 ms, median 183): UI thread blocked in 54 of 150 jstack samples over one minute. Every one was in `AbstractObject.getEffectiveRights` ← `ObjectsPerspective.updateObjectToolBar`, reached from both `navigationSelectionChanged` and the `OBJECT_CHANGED` listener → `updateContext`.
- The call was added with the separate execute-script right (223bd95d8b, #3387) and is in every release since 7.0.0.
- `AbstractObject` caches rights per instance only, and every `OBJECT_CHANGED` delivers a new instance. So each update of the selected object costs a round-trip, and so does selecting any object updated since it was last selected. A burst of updates on a container chains the round-trips into multi-second freezes.
- `AbstractObject` has no read that doesn't block: `effectiveRights` and `effectiveRightsCached` are private and unsynchronized, and `getEffectiveRights()` fetches on a miss.
- The `OBJECT_CHANGED` listener checks the current context before `asyncExec`, not inside the queued callback. A queued update can therefore install the context of an object the user has already left.
- Found during design: group state changes don't refresh cached access rights.
  - The console's disable/enable actions send `MODIFY_FLAGS` only, and neither the NXCP nor the JSON modify path counts that as an access-rights change.
  - LDAP sync disables groups directly (`RemoveDeletedLDAPEntries`) and only clears the inherited cache at the end. It never refreshes the system rights of logged-in sessions.
  - Deleting a group only marks it deleted until the next user DB save purges it, and `Group::isMember` checks `UF_DISABLED` but not `UF_DELETED`. So a deleted group nested in another group keeps granting that group's rights until the purge. `CollectUserGroups` already skips deleted groups, so system rights are unaffected.

## Decisions

- The toolbar is the only caller made asynchronous here. `ObjectContextMenuManager` and the views that check rights stay synchronous until plan 02 removes the round-trip entirely. Making menu construction asynchronous is not worth it for a stop-gap.
- **Display state, not cached rights.** For an update of the object that is already shown, the perspective keeps the last resolved "Execute script" state until the new fetch completes. Otherwise the button would blink on every update and the buttons after it would shift. For a newly selected object, the item stays hidden until its rights arrive.
- **Ruled out:** copying cached rights onto the replacement instance. It would cut round-trips for every caller, but rights fetched once would stay until logout, and group changes have no client-visible refresh yet.
- **Ordering:** results are matched by object ID plus a request sequence number, not by instance identity. A result applies if its object ID is still shown and no newer result for that ID has been applied. With identity matching, a burst of updates faster than the round-trip would drop every result.
- **Known residual for task 2:** an inherited-rights calculation that is already running can repopulate a cache entry the clear just removed, so a revocation can occasionally fail to take effect on a busy server. That race exists today. It is fixed properly in plan 02 (task 1), not here.
- Commit messages describe task 2 neutrally ("group state changes refresh cached access rights") and reference #4068. A private tracking ticket exists; don't reference it publicly.

## Tandem

| Task | Writer | Reviewer |
|---|---|---|
| 1 | claude | codex |
| 2 | codex | claude |

## Implementation Steps

### Task 1: Fetch toolbar rights off the UI thread

Layers: client library, UI (nxmc, shared by desktop and web).

`AbstractObject` gets one safely published read that doesn't block and reports "unknown" when nothing is cached. `updateObjectToolBar` uses it, and on a miss it starts a background fetch using the console's existing background job mechanism. There is at most one running fetch per perspective, plus the latest wanted object. Further updates replace the wanted object instead of starting new jobs. A result that arrives is applied on the UI thread under the ordering rule above. The item is inserted at its normal position, after "Properties", not appended. The `OBJECT_CHANGED` listener rechecks the current context inside the queued callback.

- [x] selection and `OBJECT_CHANGED` handling no longer reach `NXCSession.getEffectiveRights` on the UI thread
- [x] "Execute script" doesn't blink on updates of the shown object, and appears only when `OBJECT_ACCESS_EXECUTE_SCRIPT` is granted
- [x] a burst of updates still ends with the correct item state
- [x] a queued update for an object the user has left doesn't replace the current context
- [x] desktop and web consoles build (`mvn -f src/client/nxmc/java/pom.xml clean package -Pdesktop -Pstandalone` and `-Pweb`)

⚠️ The checks below are left for the user's final review: neither agent can drive the console GUI against a high-latency server inside the run. The checked boxes above follow from the code and from both console builds, not from a UI test.

➕ The user ran a stable-7.0 build with this task cherry-picked against the customer's SSH-tunnelled server and reported the console "way faster for the user". That is the user's observation of toolbar responsiveness. The flicker, rapid-selection and jstack checks below still have no separate evidence.

Verified by hand (no UI test harness):
- high-latency server (the SSH-tunnelled setup from the profile, or locally added delay): clicking containers and letting updates arrive doesn't freeze the UI; re-sampling the UI thread with jstack shows no `waitForRCC` under `updateObjectToolBar`
- user with execute-script right sees the item without flicker during updates; user without it never sees it
- rapid selection changes across many objects leave the toolbar matching the last selected object

### Task 2: Refresh access rights on group state changes

Layers: server (user DB, LDAP sync).

- **Manual edits:** in both the NXCP and the JSON modify path, an actual change of `UF_DISABLED` on a group schedules `UpdateGlobalAccessRights`, the same as a membership change. Compare the flags before and after. A `MODIFY_FLAGS` request that leaves the state unchanged doesn't trigger it.
- **LDAP sync:** at the end of a sync, the server calls `UpdateGlobalAccessRights` instead of only clearing the inherited cache, so open sessions' system rights follow LDAP changes. `updateSystemAccessRights` sends a message only when a session's rights actually changed, so an unconditional call costs no traffic. `RemoveDeletedLDAPEntries` disables a group only if it isn't already disabled.
- **Deleted groups:** `Group::isMember` treats a deleted group like a disabled one.
- **Out of scope:** users. Disabling a user already blocks login, and existing sessions of a disabled user are a separate question.

- [ ] disabling a group removes rights its members inherited through it, without a server restart or another user DB change; enabling restores them
- [ ] logged-in members' system rights follow the group's state, after manual edits and after LDAP sync
- [ ] a deleted group stops granting rights immediately, including through nesting
- [ ] LDAP sync of a group that is already disabled doesn't re-disable it or send `USER_DB_MODIFY`
- [ ] integration test in `UserTest`: a group with an ACL entry on a container and a user member; the user's effective rights on a child object, queried as admin, drop after disabling the group and return after enabling it. The check polls with a timeout, because the refresh runs asynchronously. Same check for deleting a nested group. Gate: `mvn -f tests/integration/pom.xml test -Dtest=UserTest` against a running server built from this tree

Verified by hand: an LDAP sync that disables a group, with a member logged in, changes that member's system rights.

## Post-Completion

- cherry-pick both commits to stable-7.0 and build a console for the customer that reported #4068
- re-profile that console against the customer's server to confirm the freezes are gone
