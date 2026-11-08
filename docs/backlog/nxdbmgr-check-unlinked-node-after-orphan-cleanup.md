---
worth: yes
where: src/server/tools/nxdbmgr/check.cpp
added: 2026-11-08
---
# nxdbmgr check removes a node's last binding after the unlinked-node check has passed it

`CheckNodes` accepts a node as linked when it has any raw `nsmap`, `container_members` or
`cluster_members` row, without checking that the subnet, container or cluster exists. The orphan
relation stages (`s_orphanRelations[]`, Object group) run at the end of the check and delete those
dangling rows. A node whose only binding was such a row is now unlinked, but the run still reports
"All errors in database was fixed". The next `nxdbmgr check` reports the node as unlinked and offers
to delete it when no interface allows relinking; with `-f` that deletion is accepted. The same
ordering applies to anything else whose validity a hand-written stage decides from rows that a later
orphan relation removes, so a second run is not guaranteed to be the last one.

Surfaced by the revmux review of PR #3701 (round `pr-3701/01-initial`). Deferred because the fix is
stage restructuring: run the membership relations (`nsmap.subnet_id`, container and cluster
membership parents) before `CheckNodes`, or make `CheckNodes` join the parent tables instead of
testing raw rows.
