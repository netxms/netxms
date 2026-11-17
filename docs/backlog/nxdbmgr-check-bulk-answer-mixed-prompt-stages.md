---
worth: yes
where: src/server/tools/nxdbmgr/check.cpp
added: 2026-11-17
---
# nxdbmgr check stages that mix prompt kinds share one bulk answer

An "All" or "Skip" answer in `nxdbmgr check` lasts until `ResetBulkYesNo()`, which `StartStage()` /
`EndStage()` call. Since PR #3701 the grouped stages reset it per relation, table and class. Some
older hand-written stages still ask different questions inside one stage and one row loop, so "All"
given to one kind auto-confirms the other:

- `CheckNodes` ("Node to subnet bindings"): `FindSubnetForNode()` asks "Unlinked node object ...
  can be linked to subnet ... Link?", and for a node it cannot relink `CheckNodes` asks "Unlinked
  node object ... Delete it?". "All" given to a Link? prompt deletes every later unlinkable node,
  with its ACL, inventory, components and properties rows, without asking. This is the most
  destructive case; since PR #3701 the membership parent stage before `CheckNodes` can send more
  nodes down the unlinked path in one run
- `CheckTemplateToTargetMapping`: "possibly misplaced object binding ... Fix it?" and "mapped to
  non-existent object ... Delete this mapping?"
- `CheckAssetNodeLinks`: "linked to component ... which is not a node", "possibly misplaced asset
  linking" and "linked to non-existent node ... Delete this mapping?"; the node-to-asset stage the
  same way
- `CheckDataTables`: the IDATA and TDATA "not found. Create?" prompts for each object
- `CheckBusinessServiceTicketServiceBindings` / `CheckBusinessServiceTicketHierarchy`: two prompts
  each
- `CheckDataCollectionItems`: separate "Delete?" prompts for DCIs and table DCIs

Resetting before every prompt would remove "All" altogether, so the fix is to split each stage per
prompt kind (one pass per kind) or to keep a bulk answer per prompt kind. Pre-existing behaviour on
master, noticed while fixing the grouped stages for the second review of PR #3701.
