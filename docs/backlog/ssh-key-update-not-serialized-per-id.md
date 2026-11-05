---
worth: later
where: src/server/core/sshkeys.cpp
added: 2026-11-05
---
# SSH key create/edit is not serialized per key ID

Both `CreateOrEditSshKey` overloads do an unguarded read-modify-write on `s_sshKeys`: the JSON overload checks
the old key exists, then looks it up again later (a concurrent `DeleteSshKey` in between leaves a name-only
object with null keys), and `ThreadPoolExecuteSerialized` orders only the DB writes — two concurrent edits can
publish B-then-A to the map while the DB ends with A-then-B. Proper fix is one per-ID mutation boundary
covering update and delete. `later`: it needs two admins editing the same key at once; unclear it is worth the
locking change unless it shows up in practice. (The NXCP overload's null `oldKey` dereference on a deleted ID
is being fixed with the SSH key dialog redesign, not here.)
