---
worth: later
where: src/server/core/sshkeys.cpp
added: 2026-11-05
---
# SSH key create/edit is not serialized per key ID

Both `CreateOrEditSshKey` overloads go through `StoreSshKey`, which does an unguarded read-modify-write on
`s_sshKeys`: it looks the old key up, then later queues `saveToDatabase` and calls `s_sshKeys.set`. A concurrent
`DeleteSshKey` between the lookup and the `set` is undone in memory — the deleted key is put back into the map,
and depending on how the queued DB writes interleave, the DB either gets the row back or disagrees with the map. `ThreadPoolExecuteSerialized` orders only the DB writes, so two concurrent edits can also
publish B-then-A to the map while the DB ends with A-then-B. Proper fix is one per-ID mutation boundary
covering update and delete. `later`: it needs two admins editing and deleting the same key at once; unclear it
is worth the locking change unless it shows up in practice.
