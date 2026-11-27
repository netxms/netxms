---
worth: yes
added: 2026-11-27
---
# dead code paths around schema tables

Found while writing the data dictionary (master fc2a1b0ed8). None has a user-visible effect today.

- `DataCollectionTarget::cleanDCIData` single-table branch (`dctarget.cpp:440-460`): `SingleTablePerfData`
  is 1 only on TimescaleDB, where the housekeeper drops chunks instead; the branch would also fail,
  since non-TSDB `idata`/`tdata` have no `node_id`.
- `Container::saveToDatabase` fallback INSERT (`container.cpp:270-273`) binds parameter 3 and omits the
  NOT NULL `object_class`; unreachable because `ContainerBase::saveToDatabase` already merges the row.
- `APF_INCLUDE_IN_ICMP_POLL` = 0x1 shares its bit with `DCF_DISABLE_STATUS_POLL` on access points;
  nothing sets it and the AP branch in `node.cpp:18294` iterates node children, which cannot be APs.
- `RCF_FRONT_ONLY` and `RKF_FRONT_SIDE_ONLY` (both 0x10000, `nxcldefs.h:386,465`), neither used.
- `AITask::logExecution` 'R'/'U' cases (`ai_tasks.cpp:548-552`): every caller sets another status first.
- `ai_messages.cpp:78-79`: status update to expired right before the row is deleted;
  `includeResponsibleUsers` of `CreateAIMessage` never passed as true.
- `ReloadMatchingRules()` (`matching.cpp:235`) has no caller, and `otlp_matching_rules` has no editor
  (console or WebAPI); rules change only in the database, effective after restart.
