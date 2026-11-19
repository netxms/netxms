---
worth: later
where: src/server/core/dci_data_query.cpp:31
added: 2026-11-19
---
# Collected data not yet migrated from v5 tables is missing from history and graphs

After an upgrade from 5.x the per-object `idata_v5_<id>` / `tdata_v5_<id>` tables are copied in the
background (`MigrateV5Data`, `datacoll.cpp`). Until a row is copied
it is invisible to every history read: `PrepareDataSelect` (NXCP scalar and table history, WebAPI history),
on-the-fly aggregates and tier reads, and aggregate rollup/backfill only query the new tables. Other readers
already `UNION ALL` the v5 table: DCI cache load (`dcitem.cpp`), NXSL data functions (`dc_nxsl.cpp`),
reporting, anomaly profiles, `dci_recalc.cpp`. So graphs show gaps for old periods while scripts and reports
see the data. Same at release-6.2.6 and release-7.0.4.

Open question: add the v5 UNION to `PrepareDataSelect` and the aggregate paths while
`ODF_HAS_IDATA_V5_TABLE` / `ODF_HAS_TDATA_V5_TABLE` is set (cost: slower history queries during migration),
or accept the gap as a temporary migration artefact.

The admin guide (`installation/upgrading.adoc`, "Upgrading from Version 5", netxms-doc-antora, all branches)
documents the current behaviour; update it if this changes.
