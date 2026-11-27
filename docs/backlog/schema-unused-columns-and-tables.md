---
worth: yes
added: 2026-11-27
---
# schema columns and tables nothing uses

Found while writing the data dictionary (docs repo, component `datadict`, master fc2a1b0ed8). Each is
documented there as unused or reserved; drop it (schema.in plus upgrade) or start using it, then update
the page named in brackets.

- `vnc_credentials`, `slm_agreements`, `organizations`, `persons`, `licenses`: referenced only by
  nxdbmgr (their own pages).
- `config.possible_values`: still created by `sql/schema.in:37`, dropped on upgraded databases by
  `upgrade_v22.cpp:201` / `upgrade_v30.cpp:1371`, read by nothing (`config`).
- `trusted_devices.description`: never written, insert at `2fa.cpp:523` omits it.
- `incident_activity_log.details`: every `logActivity` caller passes nullptr.
- `netconf_queries.flags`: stored and round-tripped, no bit is interpreted.
- `hardware_inventory.description`: never filled from collected data (`hwcomponent.cpp:170`).
- `port_layouts.layout_data`: never parsed (`devdb.cpp:44`, TODO).
- `address_lists.community_id`: always bound as 0 (`addrlist.cpp:202`).
- `clusters.cluster_type`: stored and sent to clients, never interpreted.
- `cloud_domains.last_discovery_status` and `traffic_observers.last_discovery_status`: only 0 or 2
  are ever written.
- `mobile_devices.speed`: varchar(20) but bound and read as double (`mobile.cpp:90,134`).
- `UF_SYNC_EXCEPTION`: nothing sets it any more; only cleared on old databases.
- `NC_IS_PROFINET`: never set by discovery.
- index `idx_ai_task_exec_log_asset_id` is on `task_id` (`schema.in:3730`): name only.
