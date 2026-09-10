---
worth: yes
added: 2026-09-10
---
# physical placement patch commits a stale snapshot after its unlocked validation window

`Node::modifyFromJSONInternal` (node.cpp:11761-11778 in PR #3633) stages the six placement fields into
locals, drops the property lock across `ModifyPhysicalPlacementFromJson`, relocks and writes all six back
unconditionally, with no check that the members still hold what was captured.
`ModifyPhysicalPlacementFromJson` itself writes all six through the ref (physical_placement.cpp:168-173)
including fields the document never mentioned, so the commit replays the whole group either way.
`Chassis::modifyFromJSONInternal` has the same block at chassis.cpp:276-281, plus `m_controllerId`, which
is staged at chassis.cpp:223 and written unconditionally at chassis.cpp:287 even when the PATCH omitted
`controllerId` — with the placement unlock window now sitting between the two.

Two API clients cannot race: the HTTP daemon runs `MHD_USE_INTERNAL_POLLING_THREAD` with no thread pool
and no thread-per-connection (webapi.cpp:542,550) and invokes handlers inline, so every REST request is
serialised on one thread. What survives is cross-path:

- `Node::modifyFromMessageInternal` (node.cpp:11033-11050) and the chassis equivalent write the same
  members from per-session client threads. A console placement edit landing inside the window is lost, or
  wins over fields the REST document never touched.
- `onObjectDelete` clears `m_physicalContainer` / `m_controllerId` under the property lock. If the
  container or controller is deleted during a patch that does not carry that key, the commit restores the
  dead object's id into the member and into the database, with no parent link and no rebind scheduled.

Either way the object tree and the stored container id diverge silently, and nothing reconciles them until
the next placement change. The window holds only `FindObjectById` and the `isChild` descendant walk with
no I/O, so hitting it is unlikely — which is why #3633 was merged with it rather than held.

Introduced by the fix for the round-5 lock inversion, which is itself correct: holding the property lock
across `isChild` would establish properties -> child list, the reverse of the configuration poll's order.
The fix is to commit only the keys the merge-patch actually carried, or to re-read
`m_physicalContainer` / `m_controllerId` after relocking and abort on a mismatch. Three sites, all added
by #3633. Related: [chassis-nxcp-controller-id-does-not-rebind](chassis-nxcp-controller-id-does-not-rebind.md)
touches the same `m_controllerId` assignment.
