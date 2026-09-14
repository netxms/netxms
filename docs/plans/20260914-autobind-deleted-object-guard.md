# Prevent autobind from re-linking objects that are being deleted

## Overview

`NetObj::deleteObject()` unlinks the object only from the parents present in its parent list at the moment
it takes the parent-list write lock. Autobind polls (`Container`, `Collector`, `Cluster`, `Circuit`,
`Template`) snapshot `g_idxObjectById`, evaluate the filter per object and call `NetObj::linkObjects()`
without checking whether the candidate is being deleted. A poll that took its snapshot before the deletion
and reaches the object after the parent list was cleared re-adds the object to the parent's child list.

Result on a production server (6.2.3, diagnostics from 2026-09-14): 12 fully deleted nodes (gone from the
database and from `g_idxObjectById`) were still present in the child lists of autobind containers and
templates, kept alive only by those `shared_ptr`s. Consoles count them (child ID list is sent with the
parent) but cannot display them, NXSL `children` returns them, and `container_members` rows for
non-existent objects were written to the database.

This change stays entirely at `NetObj` level. `NetObj::linkObjects()` refuses a child whose deletion has
started and re-checks after linking, undoing the link if the deletion started in between. The autobind
loops additionally skip such objects before evaluating the filter. No members move and `NObject` in
`libnxsrv` is untouched.

## Context (from discovery)

- Files/components involved:
  - `src/server/include/nms_objects.h` — `NetObj::linkObjects()` signature
  - `src/server/core/netobj.cpp` — `NetObj::linkObjects()` (line ~196), `NetObj::deleteObject()`
    (flag set at line ~946, parent clearing at lines ~1004-1029)
  - `src/server/core/container.cpp` `Container::autobindPoll()` (line ~386)
  - `src/server/core/collector.cpp` `Collector::autobindPoll()` (line ~188)
  - `src/server/core/cluster.cpp` autobind bind site (line ~136)
  - `src/server/core/circuit.cpp` `Circuit::autobindPoll()` (line ~178)
  - `src/server/core/template.cpp` `Template::autobindPoll()` (line ~1128) and
    `src/server/core/dcowner.cpp` `applyToTarget()` (line ~873)
  - `src/server/core/nxslext.cpp` `F_BindObject` (returns result of linking)
- Related patterns found:
  - `NetObj::linkObjects()` is the **only** caller of `NObject::addParentReference()` and
    `NObject::addChildReference()` in the server tree (verified by grep). A guard in `linkObjects()`
    therefore covers every bind path: autobind, NXSL, import, HA sync, topology, console.
  - `m_isDeleteInitiated` is set in `deleteObject()` under `lockProperties()` before any unlinking, and
    `Node` polls check it when they start (node.cpp), but a poll already running when deletion starts continues until `prepareForDeletion()` sees it finish. Its meaning already is "deletion has
    started, stop touching this object". Accessor `isDeleteInitiated()` exists on `NetObj`.
  - `deleteObject()` holds the parent-list **write** lock while it calls `deleteChildReference()` on every
    parent and then `clearParentList()`. `deleteChildReference()` takes the parent's child-list write lock.
    These lock hand-offs are what make the post-check in `linkObjects()` see the flag.
  - `ContainerBase::postLoad()` already drops dangling member IDs on startup with an
    "Inconsistent database" error, and `nxdbmgr check` stage "Container membership" removes such rows.
    Those remain the recovery paths; this plan is prevention only.
- Dependencies identified:
  - `linkObjects()` has ~75 call sites. The autobind loops and NXSL `BindObject` return or act on
    the result. Paths that create a new object and link it to a parent (`Node::createNewInterface()`,
    `CreateObjectFromJSON()`, NXSL `CreateNode`/`CreateContainer` and the `create*` object methods) must not leave
    a published object without a parent when the link is refused: they delete the new object and report failure.
    Tunnel auto-bind falls back to the infrastructure root. Load-time, import and HA sync callers ignore the result.
  - `NetObj::destroy()` needs no change: both callers destroy a freshly created instance whose
    `loadFromDatabase()` failed, before it is inserted into the indexes, so no other thread can link to it.

## Development Approach

- **testing approach**: Regular (code first). There is no unit-test harness for server object graph
  code (tests/ covers libnetxms, libnxdb, libnxsl, libnxsnmp, agent). Verification is a deterministic
  manual reproduction on a dev server (see Testing Strategy) before and after the change.
- complete each task fully before moving to the next
- make small, focused changes; no changes to `NObject` or the relation-list primitives
- keep lock nesting as it is today: the fix must not take a child's parent-list lock and a parent's
  child-list lock nested inside each other, to avoid introducing lock-order inversions
- target `master` first, then backport the same commit to `stable-6.2` (affected customer release)

## Testing Strategy

- **unit tests**: not applicable (no harness for `NetObj`). Do not add one for this change.
- **manual reproduction** (must fail before, pass after):
  1. Create a container with autobind enabled and filter script `sleep(200); return true;`
     (the sleep stretches the poll so the window is seconds wide instead of microseconds).
  2. Have at least ~50 nodes so the poll takes a while; start "Autobind poll" on the container from the
     console and, while it runs, delete a node that the poll has not reached yet.
  3. Before the fix: after the poll ends, `show objects` in the debug console lists the container with the
     deleted node's ID in `Children:` while the node itself has no `Object ID` entry; the console shows a
     child count one higher than the visible tree; `SELECT * FROM container_members WHERE object_id=<id>`
     returns a row after the next sync.
  4. After the fix: none of the above; debug log (tag `obj.relations`, level 5) shows the refused or
     undone link.
  5. Repeat with a template (`Template::autobindPoll` path) and a cluster.
- **regression check**: normal bind/unbind from the console, autobind on/off, template apply/remove,
  node deletion with interfaces in a circuit, server restart with existing data — all unchanged.
- **build**: `make` with `--with-server`; no schema or client changes.

## Progress Tracking

- mark completed items with `[x]` immediately when done
- add newly discovered tasks with ➕ prefix
- document issues/blockers with ⚠️ prefix
- update plan if implementation deviates from original scope

## Solution Overview

Two layers, both at `NetObj` level:

1. **Autobind loops** skip candidates with `isDeleteInitiated()` before evaluating the filter, and post the
   autobind event only when the link actually happened. On its own this shrinks the window from "snapshot
   to reaching the object" (minutes on a large server) to "check to link" (milliseconds), but does not close
   it: the deletion can start after the check and finish clearing before the link lands.
2. **`NetObj::linkObjects()`** closes the window. It refuses up front if the child has deletion initiated,
   links, then re-reads the flag. If the flag is set at that point, it removes both references again and
   returns `false`. This is the guarantee; layer 1 is an optimization that also avoids running the filter
   script and posting `SYS_CONTAINER_AUTOBIND` / `SYS_TEMPLATE_AUTOAPPLY` for a dying node.

Why the post-check in `linkObjects()` is race-free. Let C be the child being deleted and P a parent being
bound. `deleteObject()` sets the flag, then later takes C's parent-list write lock, calls
`P->deleteChildReference(C)` for every P in the list (taking P's child-list write lock), clears the list,
and releases.

- If our `addParentReference` ran before the clearing loop, P is in C's parent list when the loop runs, so
  the loop takes P's child-list lock. Either our `addChildReference` already happened (the loop removes it)
  or it happens later (it takes P's child-list lock after the loop released it, so the post-check sees the
  flag and undoes both references).
- If our `addParentReference` ran after the clearing loop, it took C's parent-list lock after the loop
  released it, so the post-check sees the flag and undoes both references.
- If the flag is not visible at post-check time, the deletion has not reached the clearing loop yet, P is
  in C's parent list, and the loop will remove the child reference when it runs.

In every case the flag write is sequenced before a lock release in the deleting thread, and the post-check
is sequenced after our acquisition of that same lock, so the read is well-ordered without atomics. The
pre-check and the autobind-loop check are plain reads and only need to be "usually right".

The same argument applies symmetrically to the parent: `deleteObject()` clears the parent's child list
under its child-list write lock and then calls `deleteParentReference()` on every child that was in the
list. Checking `parent->isDeleteInitiated()` in the post-check closes the mirror-image race, where a live
node would otherwise keep a deleted container in its parent list.

## Guarantees and limits

What the change guarantees, assuming `deleteObject()` keeps its current structure (flag set first, relation
lists cleared under their write locks, per-parent / per-child unlink inside those critical sections):

- After `deleteObject()` has passed its relation-clearing steps, no call to `linkObjects()` can leave the
  deleted object in any parent's child list or any child's parent list. Every `linkObjects()` that races
  the deletion ends with either the deletion's own loop or the post-check removing the link.
- Because `linkObjects()` is the only writer of relation lists, this covers autobind, NXSL `BindObject`,
  console bind, import, HA sync and topology code alike.
- The in-memory lists drive `container_members` and `dct_node_map`, so no new dangling rows are written.
  The undo path marks the parent modified so a row saved during the microsecond transient window is
  rewritten on the next sync.
- No new locks and no nested locking, so no new deadlock surface.

What it does not do:

- It does not repair references that already exist (the 12 zombies on the customer server). Those need a
  restart or NXSL `UnbindObject()`.
- It does not protect other holders of `shared_ptr<NetObj>` (network map links, DCI-related references,
  module-private structures). Those have their own `onObjectDelete()` handling and are out of scope here.
- The autobind-loop pre-checks are best-effort and only reduce wasted work; the guarantee comes from
  `linkObjects()` alone. A future bind path that bypasses `linkObjects()` would bypass the guarantee, which
  is why the relation primitives stay `protected` in `NObject`.

Key design decisions:

- **Guard in `linkObjects()`, not in `NObject`.** `linkObjects()` is the single choke point for creating
  parent/child links, and it already has `NetObj` access to `m_isDeleteInitiated`. No member moves, no
  change to `libnxsrv`, no signature change on the primitives.
- **Undo instead of refuse-under-lock.** Doing the check inside `addParentReference()` would need the
  flag in `NObject`. Adding then undoing in the rare race costs one extra list operation and is equally
  airtight (argument above).
- **Reuse `m_isDeleteInitiated`.** It is one-way and already means "stop touching this object".
- **No nested locking.** `linkObjects()` keeps its current sequence of independent lock sections.
- **`detachForClusterSync()` is left unchanged.** Not part of the race.

## Technical Details

### `src/server/include/nms_objects.h`

Change `static void linkObjects(...)` to `static bool linkObjects(...)`.

### `src/server/core/netobj.cpp` — `NetObj::linkObjects()`

```cpp
/**
 * Link two objects. Returns false (and leaves no link behind) if deletion of either object has started.
 */
bool NetObj::linkObjects(const shared_ptr<NetObj>& parent, const shared_ptr<NetObj>& child)
{
   if (child->isDeleteInitiated() || parent->isDeleteInitiated())
   {
      nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 5, _T("NetObj::linkObjects: link refused, object is being deleted (parent=%s [%u]; child=%s [%u])"),
            parent->m_name, parent->m_id, child->m_name, child->m_id);
      return false;
   }

   child->addParentReference(parent);
   parent->addChildReference(child);

   // Deletion of either object may have started and cleared its relation list between the check above and
   // the two adds; the lock hand-offs inside deleteObject() guarantee the flag is visible here in that case
   if (child->isDeleteInitiated() || parent->isDeleteInitiated())
   {
      parent->deleteChildReference(child->m_id);
      child->deleteParentReference(parent->m_id);
      parent->markAsModified(MODIFY_RELATIONS);   // in case syncer saved the transient link
      nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 5, _T("NetObj::linkObjects: link undone, object deleted concurrently (parent=%s [%u]; child=%s [%u])"),
            parent->m_name, parent->m_id, child->m_name, child->m_id);
      return false;
   }

   child->markAsModified(MODIFY_RELATIONS);
   parent->markAsModified(MODIFY_RELATIONS);
   child->clearInheritedAccessCache();
   child->notifyClientsOnAccessChange();
   nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 7, ...);   // unchanged
   return true;
}
```

Note: `addChildReference()` propagates inheritable custom attributes into the child. If the link is undone,
those attributes stay on a dead object, which is harmless.

`deleteObject()` and `destroy()` need no change.

### Autobind loops — skip objects being deleted, post event only on success

Same three-line pattern in each site:

- `container.cpp` `Container::autobindPoll()`
- `collector.cpp` `Collector::autobindPoll()`
- `cluster.cpp` autobind bind site
- `circuit.cpp` `Circuit::autobindPoll()`
- `template.cpp` `Template::autobindPoll()` (bind branch calls `applyToTarget()`; make
  `DataCollectionOwner::applyToTarget()` return `false` when `linkObjects()` refused, and skip the
  `EVENT_TEMPLATE_AUTOAPPLY` post in that case)

```cpp
      shared_ptr<NetObj> object = objects->getShared(i);
      if (object->isDeleteInitiated())
         continue;   // Object is being deleted, do not evaluate or bind
      ...
      if ((decision == AutoBindDecision_Bind) && !isDirectChild(object->getId()))
      {
         if (!linkObjects(self(), object))
            continue;
         EventBuilder(EVENT_CONTAINER_AUTOBIND, ...).post();
         calculateCompoundStatus();
      }
```

### `src/server/core/nxslext.cpp` — `F_BindObject`

Return the result of `linkObjects()` instead of unconditional `true`, so scripts can see a refused bind.

### Other `linkObjects()` call sites

Leave as they are. Compilation is unaffected by the `void` → `bool` change. Review the six sites in
`hasync.cpp` and the three in `import.cpp` only to confirm none of them relies on the link having happened
for a subsequent step (none do at first reading; note deviations here with ⚠️).

### Documentation

- `doc/internal/debug_tags.txt`: no new tag (`obj.relations` already exists).
- ChangeLog entry under the next 6.2.x and master release: "Fixed race condition where autobind could re-link
  an object that was being deleted, leaving a dangling child reference in containers and templates".

## Tasks

- [x] `nms_objects.h` / `netobj.cpp`: `linkObjects()` returns `bool`; pre-check, post-check with undo of
      both references, debug lines
- [x] Autobind loops (container, collector, cluster, circuit, template + `applyToTarget()`): skip
      `isDeleteInitiated()`, post event only when link succeeded
      ➕ `Cluster::addNode()` and `Template::autobindPoll()` detect a refused link via `isDirectChild()`
      after `applyToTarget()` instead of its return value, so pre-existing DCI copy failures keep their old
      behaviour (`applyToTarget()` itself now returns `false` early when the link is refused).
      ➕ Fixed cluster poller message that had a `%s` argument without a placeholder.
- [x] `nxslext.cpp` `F_BindObject`: propagate result
- [x] Grep for `linkObjects(` to confirm every caller still compiles and none needs the result: all
      remaining ~70 call sites are plain statements, no function-pointer uses; `objects.cpp` template bind
      path maps a refused link to `RCC_DCI_COPY_ERRORS`, which is acceptable for an object being deleted.
- [x] ➕ Integration tests `tests/integration/.../ObjectDeletionRaceTest.java`:
      `testNodeDeletedDuringAutobind` (container autobind with delayed filter, node deleted mid-poll,
      asserts poll reached the bind step and container child list has no reference) and
      `testContainerDeletedDuringBind` (script `BindObject()` racing container deletion, asserts node
      parent list has no reference) and `testInterfaceCreatedDuringNodeDeletion` (node deleted while its
      configuration poll runs against an unreachable address, asserts the pseudo-interface created by the poll
      after deletion started does not survive as an object without parent).
- [x] Build server and run the integration tests against it: all three fail on a server without the fix
      (first two) and pass on a server with it. Run `ObjectDeletionRaceTest` before and after the server change to confirm it fails
      on the old code.
- [ ] Manual reproduction for the template and cluster autobind paths (not covered by the integration tests)
- [x] ChangeLog: not applicable in this repository (moved to https://github.com/netxms/changelog); add the
      entry there on release
- [ ] Backport to `stable-6.2` (cherry-pick; verify the autobind loops match on that branch)

## Out of scope (noted for later)

- Dashboard autobind binds through `m_dashboards` on the target, not through the relation lists; deletion is
  handled by `onObjectDelete()`. Not affected by this race.
- A runtime consistency sweep that removes dangling child references from live containers without a restart.
  Recovery today is server restart (`postLoad()` drops them) or NXSL `UnbindObject()` on the parent.
