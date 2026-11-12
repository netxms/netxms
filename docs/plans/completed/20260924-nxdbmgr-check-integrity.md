# nxdbmgr check: referential integrity and logical consistency checks

## Overview

`nxdbmgr check` verifies only a small part of the schema: missing `object_properties` rows for eight
object classes, node-to-subnet and component-to-node bindings, cluster members, template mapping,
container members, EPP source/event/action references, map links, `idata`/`tdata` tables, DCIs on
missing objects, DCI proxy nodes, raw values, thresholds, business service tickets, asset links and
collected data. Everything else is never looked at.

A survey of every `deleteFromDatabase` implementation in `src/server/core/` shows that several
tables are never cleaned on object delete (`dashboard_associations`, `object_ai_data`,
`port_stop_list`, `cluster_resources`, `radios`, `dci_delete_list`), so orphan rows accumulate in
every long-lived installation, and nothing detects references to deleted objects held in optional
pointer columns (node proxies, interface peers, drilldown objects, zone UINs).

This change adds:

1. A **generic orphan relation stage** driven by a static table of child-to-parent relations. Each
   relation runs one set-based query, prompts once per missing parent, and either deletes the rows,
   resets the reference to zero, or reports only.
2. A set of **hand-written logical checks** for cases the generic stage cannot express: full object
   class coverage, ghost property rows, duplicate IDs and GUIDs, container cycles, duplicate
   subnets, template / cluster / instance bindings, coupled interface peer and path-check repairs,
   event code references, notification channel names, and a safety fix in the existing `CheckEPP`.
3. A **defect injection test** that seeds every defect class into a scratch database and verifies
   `nxdbmgr -f check` reports and repairs each one, and that valid fixtures are never reported.

Users, groups and their ACL tables are out of scope. Existing per-row stages are left as they are;
converting them to set-based queries is a separate cleanup. Server-side deletion leaks are tracked
in GitHub issues #3691, #3692, #3693, #3694 (see Post-Completion).

## Context (from discovery)

- `src/server/tools/nxdbmgr/check.cpp` — all check stages and `CheckDatabase()` dispatcher. The
  whole check runs inside one transaction (`DBBegin` at `check.cpp:1929`).
- `src/server/tools/nxdbmgr/nxdbmgr.h` — `g_checkData`, `g_checkDataTablesOnly`, module hooks.
  nxdbmgr includes `nxsrvapi.h`/`nxdbapi.h`, not the full `nms_objects.h`.
- `src/server/tools/nxdbmgr/init.cpp` — `ExecSQLBatch()`: replaces CR/LF outside quotes with
  spaces (so a `--` line comment swallows the following statement), tracks quote state on `'`,
  stops at the first failing statement, prints `SQL query failed`; its return value is discarded
  by `nxdbmgr.cpp`, so `batch` always exits 0. `init` for `pgsql`/`odbc` needs an explicit type
  argument when stdin is not a tty and refuses an already initialized database.
- `src/server/tools/nxdbmgr/nxdbmgr.cpp` — `check` always exits 0; `-E` only fails from inside
  the yes/no prompt (`libnxdbmgr/confirm.cpp`), so report-only findings never fail `-E`.
- `src/server/include/nxdbmgr_tools.h` — `StartStage`/`SetStageWorkTotal`/`UpdateStageProgress`/
  `EndStage`, `GetYesNoEx` (bulk yes/no with `ResetBulkYesNo`), `SQLSelect`, `SQLQuery`,
  `IsDatabaseRecordExist`, `DBMgrGetObjectName`, `DBMgrExecuteQueryOnObject`, `g_dbCheckErrors`,
  `g_dbCheckFixes`, `g_ignoreErrors`.
- `src/server/include/nms_objects.h:107-114` — `BUILTIN_OID_*` (1,2,3,4,5,6,7,9; 8 undefined).
- `include/nxevent.h` — `EVENT_THRESHOLD_REACHED` 17, `EVENT_THRESHOLD_REARMED` 18,
  `EVENT_CONDITION_ACTIVATED` 34, `EVENT_CONDITION_DEACTIVATED` 35, `EVENT_ALARM_TIMEOUT` 43,
  `EVENT_TABLE_THRESHOLD_ACTIVATED` 69, `EVENT_TABLE_THRESHOLD_DEACTIVATED` 70,
  `EVENT_SNMP_UNMATCHED_TRAP` 500.
- `src/server/core/objects.cpp:1646-1804` — loader class table list, built-in roots, `pfLoadObjects`.
  Business services and prototypes are loaded from `object_containers` (class 28 / 15); their own
  tables are extensions.
- `src/server/core/node.cpp:12125` — `Node::onObjectDelete` resets poller, proxy and physical
  container references to zero (reference behavior for `ResetToZero`; `vnc_proxy` is not in that
  list but resetting it is consistent).
- `src/server/core/node.cpp:897-902` — path check result loaded as a unit.
- `src/server/core/interface.cpp:1694,1783` — peer IDs cleared together; AP peer stores AP ID in both.
- `src/server/core/dctarget.cpp:904,3089` and `dcowner.cpp:908` — `template_id` is polymorphic:
  a template ID, a cluster ID (cluster-applied DCIs via `Cluster::addNode` → `applyToTarget`), or
  the owner's own ID (instance-discovered DCIs, `template_item_id` = discovery root).
- `src/server/core/id.cpp:170` — `max(dci_id)` from `dci_delete_list` is a floor for new DCI IDs.
- `src/server/core/epp.cpp:1106,1158` — empty source/event inclusion list means match-all.
  `EPRule::isFilterEmpty()` (`include/nms_events.h:814`) also counts source exclusions, and
  `epp.cpp:1358` skips a rule whose filter is empty, so deleting a rule's last exclusion when it
  has no inclusions changes behavior too (`matchSource(nullptr)` at `epp.cpp:1106` also flips).
- `src/server/core/condition.cpp:111,375-411` — `cond_dci_map` rows are positional inputs
  (`ORDER BY sequence_number`); a missing node or DCI yields a null value at that position, so
  deleting a row shifts every later argument to the condition script.
- `src/server/core/netobj.cpp:1081`, `schedule.cpp:1109` — object deletion removes only
  **system** scheduled tasks for the object; user tasks are retained by design.
- `src/server/core/dashboard.cpp:866-874,974-981` — `dashboard_template_instances` is loaded by
  template only and every mapped dashboard is updated regardless of the instance object.
- `sql/policy.in:7-8` — chain 0 (main chain) has a row in `event_policy_chain`.
- `src/server/core/actions.cpp` — `FORWARD_EVENT` uses `channel_name` for the forwarder name.
- `sql/schema.in` — column names for every table referenced below (all verified).
- `tests/test-server/test-server.cpp:63-100` — existing example of `nxdbmgr init` on SQLite.
- Command line: `-f` force yes, `-E` fail if fix required, `-m` machine readable output.
- Code style: server code is Unicode-only, use `wchar_t` / `L""` literals and `nx_swprintf`, no
  `_T()` in new code, 3-space indent, brace on new line, C++11 maximum.

## Development Approach

- **testing approach**: Regular (code first, then tests). Tests are a defect injection SQL script
  plus a runner script; each task adds its positive and negative injection cases and passes the
  runner before the next.
- complete each task fully before moving to the next
- make small, focused changes
- **CRITICAL: every task MUST include new/updated tests** for the checks it adds: one injected
  defect per relation entry or hand-written check, plus a "must not be reported" fixture wherever
  a false positive is plausible
- **CRITICAL: all tests must pass before starting next task** - no exceptions
- **CRITICAL: update this plan file when scope changes during implementation**
- build with `make -C src/server/tools/nxdbmgr && make -C src/server/tools/nxdbmgr install`; this
  worktree is configured with `--prefix=/Users/alk/netxms/nxdbmgr-check --with-sqlite --with-pgsql`
  so the master server install is never touched
- **roles**: Claude is the only writer in this worktree (tasks 1-11 and 13) and commits after
  each task; Codex reviews every task commit before the next task starts and owns the task 12
  acceptance audit, including the PostgreSQL harness runs and the abort/rollback test (Claude
  applies, builds and reverts the deliberate fault on Codex's instruction)
- stage names of the eight replaced property stages ("Zone object properties", "Node object
  properties", "Interface object properties", "Network service object properties", "Cluster object
  properties", "Access point object properties", "Business services", "Business service
  prototypes", "Assets") are replaced by one stage "Object class coverage". All other existing
  stage names and message formats stay as they are.

## Testing Strategy

- **defect injection test** (`tests/nxdbmgr-check/`, manual, not in build). `run.sh`:
  1. creates a scratch database, writes its own `nxdbmgr.conf` in a scratch directory and runs
     `nxdbmgr init <type>` with the type given explicitly. `sqlite` (default): a fresh file.
     `pgsql`/`tsdb`: the database name is unique per invocation, `nxdbmgr_check_<random suffix>`,
     created with `createdb` (never preceded by a drop) using the `-H host -U user` options
     (password from `PGPASSWORD`); on exit `run.sh` drops only the exact name it created, and
     only if the create succeeded. There is no option to point the harness at an existing
     configuration file, so it can never touch an application database or another concurrent
     harness run;
  2. applies `baseline.sql` (valid fixture graph), runs `nxdbmgr -f check` once so the existing
     stages create the per-object data tables, then `nxdbmgr -E check` and asserts the output
     contains `Database doesn't contain any errors` (baseline is clean);
  3. applies `inject-defects.sql`, runs `nxdbmgr -f check | tee first.log`, asserts every
     fragment in `expected-first-run.txt` is present and every fragment in
     `unexpected-first-run.txt` is absent, then applies `assert-after-first-run.sql` (post-repair
     state: negative fixtures still present, report-only rows retained, coupled fields reset
     together, repaired rows gone);
  4. applies `cleanup-report-only.sql`, runs `nxdbmgr -E check`, asserts exit code 0 **and**
     `Database doesn't contain any errors` in the output (the exit code alone is not enough:
     `check` always exits 0 and `-E` only fires from a prompt), then applies
     `assert-after-cleanup.sql`;
  5. every `batch` invocation fails the run if its output contains `SQL query failed`.
- **state assertions** are plain SQL because nxdbmgr has no query command. `baseline.sql` creates
  `nxdbmgr_check_assert (id integer not null primary key)` with one row `0`. An assertion is
  `INSERT INTO nxdbmgr_check_assert (id) SELECT CASE WHEN (<condition>) THEN <unique n> ELSE 0 END FROM nxdbmgr_check_assert WHERE id=0`:
  a false condition inserts a duplicate key, the statement fails, `ExecSQLBatch` prints
  `SQL query failed` and stops, and step 5 fails the run. The `FROM ... WHERE id=0` keeps the
  statement valid on Oracle. Conditions are scalar subqueries, e.g.
  `(SELECT count(*) FROM acl WHERE object_id=990002)=1`. Assertion IDs are unique across both
  assertion files (first-run file uses 1xx, after-cleanup file uses 2xx); successful rows are never
  cleared and sentinel row 0 is never touched.
- SQL fixture rules (forced by `ExecSQLBatch`): labels are `/* ... */` block comments only, never
  `--`; no `'` or `;` inside a comment; one statement per `;`.
- negative fixtures carry unique IDs (e.g. 990xxx range) so `unexpected-first-run.txt` can grep
  for `[990xxx]` absence.
- default driver is SQLite (self-contained); `run.sh -d pgsql` runs the same script against the
  local PostgreSQL test server.
- **unit tests**: none; `check.cpp` has no unit test harness and the checks are SQL-driven.
- **build check**: `make -C src/server/tools/nxdbmgr` after every task; no new compiler warnings.

## Progress Tracking

- mark completed items with `[x]` immediately when done
- add newly discovered tasks with ➕ prefix
- document issues/blockers with ⚠️ prefix
- update plan if implementation deviates from original scope
- keep plan in sync with actual work done

## Solution Overview

### Generic orphan relation stage

```cpp
enum class OrphanFix
{
   DeleteRow,     // DELETE FROM child WHERE key=X
   ResetToZero,   // UPDATE child SET key=0 WHERE key=X
   ReportOnly
};

struct OrphanRelation
{
   const wchar_t *childTable;
   const wchar_t *childKey;
   const wchar_t *parentSource;  // table name or parenthesized subquery, aliased as p
   const wchar_t *parentKey;
   const wchar_t *parentName;    // "object", "DCI", "template", ... for messages
   OrphanFix fix;
   bool zeroAllowed;             // true: zero means "not set" and is skipped
};
```

One stage per entry, stage name `<childTable>.<childKey>`. Query:

```sql
SELECT c.<childKey>, count(*) FROM <childTable> c
LEFT OUTER JOIN <parentSource> p ON p.<parentKey> = c.<childKey>
WHERE [c.<childKey> <> 0 AND] p.<parentKey> IS NULL
GROUP BY c.<childKey>
```

Aliases without `AS` (Oracle); derived tables always get an alias (PostgreSQL before 16 and
MySQL require one). This join shape already exists at `check.cpp:444`. The parent source may be a
subquery so that polymorphic parents stay declarative:

- DCI parent: `(SELECT item_id FROM items UNION SELECT item_id FROM dc_tables)`
- node-or-AP parent: `(SELECT id FROM nodes UNION SELECT id FROM access_points)`

Each result row is one error and one prompt:
`"<count> rows in <childTable> refer to non-existing <parentName> [<id>]. Delete them?"` or
`"... Reset reference?"`. For `ReportOnly` the row is printed and counted, no prompt. The fix
statement uses the same predicate as the SELECT.

**SQL failure handling.** Any failed statement in a new stage, SELECT or repair, sets a
file-scoped `s_checkAborted` flag (a small `CheckSelect()` / `CheckQuery()` pair wrapping
`SQLSelect` / `SQLQuery` does this so the stages stay short). Helpers reused by new stages use
the wrappers too: `CheckMissingObjectProperties` is converted when it becomes the body of
`CheckObjectClassCoverage` (task 5), since its old callers are removed in the same task.
`CheckDatabase()` treats the flag as fatal: no further stage starts once it is set (each stage
call is guarded, so a poisoned PostgreSQL transaction is not hit with dozens more queries), the
transaction is rolled back and the final line is `Database check aborted`, never
`Database doesn't contain any errors`. `DBBegin` and `DBCommit` return values are checked as
well; a failed commit prints an error and the aborted line. This matters on PostgreSQL where one
failed statement poisons the whole transaction, so every later query would also fail and a
"no errors" result would be false. Existing stages keep their current behavior (a failed repair
just leaves the error counted as unfixed).

Relations are ordered so that a parent's own orphan check runs before its dependents (objects,
then DCIs, then table thresholds, then threshold satellites). Note that `nsmap.subnet_id` rows
are deleted after `CheckNodes` has already run, so a node that becomes unlinked by that deletion
is only found on the next run; the harness keeps `nsmap` injections to deleted-node pairs.

EPP tables keyed by the composite `(chain_id, rule_id)` are a plain `const wchar_t *[]` of table
names; `CheckEppRelations()` joins on both columns against `event_policy` and deletes by both.

### Hand-written checks

Run before the relation stage so that structural repairs (lost properties created, template
bindings unbound) settle before satellites are swept. Each is a normal `StartStage`/`EndStage`
function in the current style. Details in Technical Details below.

### Order in `CheckDatabase()`

```
CheckObjectClassCoverage      (replaces CheckZones, CheckAccessPoints, CheckBusinessServices,
                               CheckAssets and the properties halves of CheckNodes, CheckComponents,
                               CheckClusters)
CheckGhostObjectProperties
CheckDuplicateObjectIds
CheckDuplicateObjectGuids
CheckNodes                    (subnet binding part, unchanged)
CheckComponents x2            (binding part, unchanged)
CheckClusters                 (member part, unchanged)
CheckContainerCycles
CheckDuplicateSubnets
CheckTemplateToTargetMapping  (unchanged)
CheckObjectProperties         (unchanged)
CheckContainerMembership      (unchanged)
CheckEPP                      (modified, last-entry guard on inclusion lists)
CheckMapLinks                 (unchanged)
CheckDataTables               (unchanged)
CheckDataCollectionItems      (unchanged)
CheckDCISourceNodes           (unchanged)
CheckTemplateBindings
CheckInterfacePeers
CheckNodePathCheckResults
CheckEventCodeReferences
CheckNotificationChannelReferences
CheckRawDciValues, CheckThresholds, CheckTableThresholds (unchanged)
CheckBusinessService*         (unchanged)
CheckAssetNodeLinks           (unchanged)
CheckOrphanRelations          (generic stage, all entries)
CheckEppRelations             (composite stage)
collected data checks         (unchanged, -d only)
CheckModuleSchemas            (unchanged)
```

## Technical Details

### Relation table

Parent `object_properties.object_id`, `DeleteRow`, `zeroAllowed=true` unless noted:

| child.column | notes |
|---|---|
| acl.object_id, object_custom_attributes.object_id, object_urls.object_id, trusted_objects.object_id, trusted_objects.trusted_object_id, responsible_users.object_id, object_access_snapshot.object_id | |
| pollable_objects.id, dc_targets.id, auto_bind_target.object_id, versionable_object.object_id, maintenance_journal.object_id, active_downtimes.object_id, downtime_log.object_id | |
| container_members.container_id, nsmap.subnet_id, nsmap.node_id, zone_proxies.object_id, zone_proxies.proxy_node | container_members.object_id is covered by existing CheckContainerMembership |
| icmp_statistics.object_id, icmp_target_address_list.node_id, software_inventory.node_id, hardware_inventory.node_id, node_components.node_id, ospf_areas.node_id, ospf_neighbors.node_id, node_snmp_agents.node_id | |
| interface_address_list.iface_id, interface_vlan_list.iface_id | `zeroAllowed=false`: required ownership link |
| cluster_members.cluster_id, cluster_sync_subnets.cluster_id, rack_passive_elements.rack_id, room_passive_elements.room_id | |
| physical_links.left_object_id, physical_links.right_object_id | |
| network_map_elements.map_id, network_map_links.map_id, network_map_seed_nodes.map_id, network_map_seed_nodes.seed_node_id, network_map_deleted_nodes.map_id | |
| dashboard_elements.dashboard_id, dashboard_template_instances.dashboard_template_id, dashboard_associations.object_id | |
| business_service_downtime.service_id, asset_properties.asset_id, vpn_connector_networks.vpn_id, resource_tags.resource_id, observation_point_hosts.point_id, ap_common.owner_id | |
| cond_dci_map.condition_id, port_stop_list.object_id, object_ai_data.object_id | |

Class-table parents, `DeleteRow`:

| child.column | parent |
|---|---|
| radios.owner_id | `(SELECT id FROM nodes UNION SELECT id FROM access_points)` |
| cluster_resources.cluster_id | clusters.id |
| dashboard_associations.dashboard_id | dashboards.id |
| dc_table_columns.table_id | dc_tables.item_id |
| dct_threshold_conditions.threshold_id, dct_threshold_instances.threshold_id | dct_thresholds.id |

DCI parent `(SELECT item_id FROM items UNION SELECT item_id FROM dc_tables)`, `DeleteRow`:
dci_schedules.item_id, dci_access.dci_id.

`ResetToZero`, parent `object_properties.object_id` (reference behavior `Node::onObjectDelete`;
prompt text for proxy columns must say "polling routing may change"):

| table | columns |
|---|---|
| nodes | poller_node_id, proxy_node, snmp_proxy, eip_proxy, icmp_proxy, ssh_proxy, netconf_proxy, vnc_proxy, mqtt_proxy, modbus_proxy, physical_container_id |
| interfaces | parent_iface |
| object_properties | drilldown_object_id |
| conditions | source_object |
| dashboards | forced_context_object_id |
| items, dc_tables | related_object |

`ReportOnly`:

| child.column | parent | why no fix |
|---|---|---|
| nodes.zone_guid, subnets.zone_guid | zones.zone_guid | moving objects into zone 0 changes addressing context and can create IP conflicts; no server reference behavior |
| dci_delete_list.node_id | object_properties.object_id | `max(dci_id)` of this table is the floor for new DCI IDs (`id.cpp:170`); deleting rows could let a new DCI reuse an ID whose history still exists. `dci_id` itself is never validated: it lists deleted DCIs by design |
| business_service_checks.related_object, business_service_checks.prototype_service_id | object_properties.object_id | server skips state updates when the target is missing; zeroing hides the broken check |
| business_service_checks.related_dci | DCI subquery | same |
| cond_dci_map.node_id | object_properties.object_id | inputs are positional (`ORDER BY sequence_number`); deleting a row shifts the later script arguments. Deletion by missing `condition_id` stays |
| cond_dci_map.dci_id | DCI subquery | same |
| scheduled_tasks.object_id | object_properties.object_id | object deletion keeps user tasks by design (`schedule.cpp:1109` removes system tasks only); a generic delete would erase retained user tasks |
| dashboard_template_instances.instance_object_id | object_properties.object_id | template updates iterate every mapped dashboard without checking the instance object; deleting the mapping stops updates to a surviving dashboard. Deletion by missing `dashboard_template_id` stays |

EPP composite `(chain_id, rule_id)` against `event_policy`, `DeleteRow`: policy_action_list,
policy_timer_cancellation_list, policy_event_list, policy_time_frame_list, policy_source_list,
policy_pstorage_actions, policy_cattr_actions, alarm_category_map, policy_chain_call_list.

Chain level, parent `event_policy_chain.chain_id`, `DeleteRow`, `zeroAllowed=false` (chain 0 is
the main chain and does have a row, so a missing row for chain 0 is corruption too):
event_policy.chain_id, policy_chain_acl.chain_id, policy_chain_call_list.target_chain_id. Plus
alarm_category_map.category_id against alarm_categories.id, `DeleteRow`.

### Hand-written checks

**(a) Object class coverage.** Loop over the loader's class tables: zones, conditions, subnets,
racks, chassis, mobile_devices, sensors, cloud_domains, resources, traffic_observers,
observation_points, nodes, access_points, interfaces, network_services, vpn_connectors, clusters,
facilities, power_domains, cooling_zones, rooms, assets, templates, network_maps, dashboards,
dashboard_templates, business_services, business_service_prototypes, object_containers. Each
row without `object_properties` gets the existing lost-properties fix via
`CheckMissingObjectProperties` (built-in exemption only for zones, ID 4). One stage
"Object class coverage" with progress per table.

**(b) Ghost object_properties.** IDs present in `object_properties` but absent from every class
table above, excluding the built-in IDs. nxdbmgr does not include `nms_objects.h`, so the IDs
are mirrored as a local `static const uint32_t s_builtinObjectIds[] = { 1, 2, 3, 4, 5, 6, 7, 9 };`
with a comment pointing at `BUILTIN_OID_*`. Report only: commercial modules may own object
classes via `pfLoadObjects`, and nxdbmgr cannot establish that no module owns the ID. Findings
are printed and counted like any other error.

**(c) Duplicate object IDs.** Same ID present in two loader-root tables. An `object_containers`
row is an extension of, not a duplicate of, a row in its own class table **only for the matching
pair**: class 15 with business_service_prototypes, 23 with dashboards, 24 with
dashboard_templates, 28 with business_services, 32 with racks, 42 with facilities, 43 with
power_domains, 44 with cooling_zones, 45 with rooms. Any other combination (class 32 plus a
`nodes` row, class 5 plus a `racks` row) is a duplicate. Report only.

**(d) Duplicate GUIDs.** `SELECT guid, count(*) FROM object_properties GROUP BY guid HAVING count(*) > 1`.
Report only.

**(e) Container cycles.** Load `container_members` into memory and find the strongly connected
components (Tarjan, linear). Every component with more than one object, or with a
self-membership, is one finding listing all its objects and all memberships between them, so
overlapping cycles are shown completely in one run and the operator can break them all at once.
Elementary cycles are not enumerated individually (that is exponential). Report only.

**(f) Duplicate subnets.** `GROUP BY ip_addr, ip_netmask, zone_guid HAVING count(*) > 1`.
Report only. Same address with a different mask (10.0.0.0/8 and 10.0.0.0/16) is not a duplicate.

**(g) Template bindings** on `items` and `dc_tables`, `template_id <> 0`. `template_id` is
polymorphic; classify in this order:
- `template_id = node_id`: instance binding. Valid when `template_item_id` is a DCI of the same
  owner; otherwise report "broken instance root". Never unbind.
- `template_id` in `clusters`: cluster binding (cluster-applied DCI). Report when
  `(template_id, node_id)` is not in `cluster_members`. No fix.
- `template_id` in `templates`: template binding. Report when `(template_id, node_id)` is not in
  `dct_node_map`. No fix.
- `template_id` not in `object_properties` at all: deleted owner. Fix:
  `UPDATE ... SET template_id=0, template_item_id=0 WHERE item_id=?`.
- `template_id` in `object_properties` but none of the above: report "unexpected template owner
  class", no fix.

**(h) Interface peers.** Rows where a **non-zero** `peer_node_id` or a **non-zero** `peer_if_id` is
not in `object_properties` (AP peers store the AP ID in both columns, so class tables cannot be
used). Each endpoint is validated on its own: a zero endpoint is never treated as missing, so a row
with a valid `peer_node_id` and `peer_if_id = 0` is not touched.
Fix per interface: `UPDATE interfaces SET peer_node_id=0, peer_if_id=0, peer_proto=0, peer_last_updated=0 WHERE id=?`.

**(i) Node path check results.** Rows where a **non-zero** `path_check_node_id` or a **non-zero**
`path_check_iface_id` is not in `object_properties` (polymorphic: may be AP or VPN connector).
Same per-endpoint rule as (h). Fix per node:
`UPDATE nodes SET path_check_reason=0, path_check_node_id=0, path_check_iface_id=0 WHERE id=?`.

**(j) Event code references.** For each (table, column, default) pair, select rows whose code is
not in `event_cfg`; before offering the fix verify the default exists in `event_cfg`, otherwise
report only. Prompt says "EPP behavior may change".

| table.column | default |
|---|---|
| thresholds.event_code | EVENT_THRESHOLD_REACHED |
| thresholds.rearm_event_code | EVENT_THRESHOLD_REARMED |
| dct_thresholds.activation_event | EVENT_TABLE_THRESHOLD_ACTIVATED |
| dct_thresholds.deactivation_event | EVENT_TABLE_THRESHOLD_DEACTIVATED |
| conditions.activation_event | EVENT_CONDITION_ACTIVATED |
| conditions.deactivation_event | EVENT_CONDITION_DEACTIVATED |
| snmp_trap_cfg.event_code | EVENT_SNMP_UNMATCHED_TRAP |
| event_policy.alarm_timeout_event (when `<> 0`) | EVENT_ALARM_TIMEOUT |

**(k) Notification channels.** `actions` with `action_type` = NOTIFICATION and non-empty
`channel_name` not in `notification_channels.name`. Report only (`FORWARD_EVENT` stores the
forwarder name in the same column and is excluded).

**(l) CheckEPP last-entry guard.** Group dangling references by `(chain_id, rule_id)` and list
kind. A finding is report-only when deleting it would change the rule's matching:
- inclusion (`policy_source_list` with `exclusion=0`, or `policy_event_list`): the rule's
  inclusion list of that kind would become empty ("deleting it would make the rule match all
  sources/events");
- exclusion (`exclusion=1`): the rule has no source inclusions and this is its last exclusion.
  `isFilterEmpty()` counts exclusions, so removing it can make the rule skipped entirely, and
  `matchSource(nullptr)` flips when both lists become empty.
Otherwise the row is deleted per `(chain_id, rule_id, object_id|event_code)`, never as a global
`DELETE ... WHERE object_id=X`.

### Test harness

```
tests/nxdbmgr-check/
  run.sh                   # usage: run.sh [-d sqlite|pgsql|tsdb] [-H host] [-U user] [-b /path/to/nxdbmgr]
                           # pgsql/tsdb: password from PGPASSWORD, database nxdbmgr_check_<random> per run
  baseline.sql             # valid fixture graph: container 990001, node 990002 as its member with a
                           # full object_properties row, interface 990004, DCI 990005 with threshold
                           # 990006; creates the nxdbmgr_check_assert table; must check clean
  inject-defects.sql       # one block per defect, /* label */ comments only
  assert-after-first-run.sql   # post-repair state assertions (see Testing Strategy)
  cleanup-report-only.sql  # removes report-only defects so the final run is clean
  assert-after-cleanup.sql # state assertions after the clean run
  expected-first-run.txt   # message fragments that must appear on the first run
  unexpected-first-run.txt # fragments (negative fixture IDs) that must NOT appear
```

`baseline.sql` must satisfy the existing stages: the node has an `nsmap` or `container_members`
row (otherwise `CheckNodes` deletes it under `-f`), and the first `-f check` run is allowed to
create the per-object `idata`/`tdata` tables before the clean `-E` assertion.

## What Goes Where

- **Implementation Steps** (`[ ]` checkboxes): code and test changes in this repository
- **Post-Completion** (no checkboxes): filed GitHub issues, run against a real customer database dump

## Implementation Steps

### Task 1: Defect injection test harness with baseline fixtures

**Files:**
- Create: `tests/nxdbmgr-check/run.sh`
- Create: `tests/nxdbmgr-check/baseline.sql`
- Create: `tests/nxdbmgr-check/inject-defects.sql`
- Create: `tests/nxdbmgr-check/assert-after-first-run.sql`
- Create: `tests/nxdbmgr-check/cleanup-report-only.sql`
- Create: `tests/nxdbmgr-check/assert-after-cleanup.sql`
- Create: `tests/nxdbmgr-check/expected-first-run.txt`
- Create: `tests/nxdbmgr-check/unexpected-first-run.txt`
- Modify: `tests/CLAUDE.md` (layout table row)

- [x] write `run.sh` implementing the five steps from Testing Strategy: scratch DB per driver with explicit `init <type>` (pgsql/tsdb: unique name `nxdbmgr_check_<random>` created without a prior drop and dropped on exit only if this run created it, `-H`/`-U`/`PGPASSWORD`, no external config file option), baseline apply + `-f check` + clean `-E check` assertion, inject + `-f check` + expected/unexpected greps + `assert-after-first-run.sql`, cleanup + `-E check` + `Database doesn't contain any errors` grep + `assert-after-cleanup.sql`, `SQL query failed` detection on every batch, non-zero exit on any failure
- [x] write `baseline.sql`: container 990001 and node 990002 (member of the container) with complete `object_properties` rows, one interface, one DCI, the `nxdbmgr_check_assert` table with row 0; labels as `/* */` only
- [x] seed `inject-defects.sql` with one defect an existing check already catches (threshold for non-existing DCI) and `expected-first-run.txt` with its fragment; seed `assert-after-first-run.sql` with one assertion that the threshold row is gone and one that node 990002 still exists, and prove the mechanism by running `run.sh` once with a disposable copy of the assertion file in which one condition is negated, and watching it fail
- [x] seed `unexpected-first-run.txt` with `[990002]` to prove the baseline node is never reported
- [x] add the directory to the layout table in `tests/CLAUDE.md` as manual / not in build
- [x] run `run.sh` against SQLite - must pass before task 2 (also passes with `-d pgsql -U alk` against the local PostgreSQL 18; role `alk` has createdb, role `netxms` does not)

### Task 2: Generic orphan relation stage with object satellite relations

**Files:**
- Modify: `src/server/tools/nxdbmgr/check.cpp`
- Modify: `tests/nxdbmgr-check/inject-defects.sql`
- Modify: `tests/nxdbmgr-check/expected-first-run.txt`
- Modify: `tests/nxdbmgr-check/unexpected-first-run.txt`

- [x] add `s_checkAborted` with `CheckSelect()` / `CheckQuery()` wrappers that set it on any SQL failure
- [x] add `OrphanFix`, `OrphanRelation` and `static void CheckOrphanRelation(const OrphanRelation& r)` implementing the grouped LEFT OUTER JOIN query, per-parent prompt with row count, fix by same predicate, using the wrappers
- [x] make `CheckDatabase()` honor `s_checkAborted` (stage calls moved to a `s_checkStages[]` function pointer array iterated until the flag is set; `CheckComponents` got two argument-free wrappers for that) and check `DBBegin` / `DBCommit` results: roll back and print `Database check aborted` instead of the no-errors line
- [x] add `static void CheckOrphanRelations()` iterating the array and call it from `CheckDatabase()` after `CheckAssetNodeLinks()`
- [x] add all `DeleteRow` entries with parent `object_properties.object_id` from the relation table, including the two `zeroAllowed=false` entries, `physical_links`, `ap_common` and `object_access_snapshot`
- [x] inject one orphan row per entry (deleted-object IDs in the 999xxx range; 999101 and up in array order; the injected `network_map_links` row carries two `network_map_elements` rows because `CheckMapLinks` runs first and deletes element-less links; baseline gained subnet 990007 so `nsmap.node_id` can be exercised with an existing subnet) and a row with key zero in `interface_address_list` to prove `zeroAllowed=false`
- [x] inject negative fixtures: an `acl` row and an `object_custom_attributes` row for node 990002; add `[990002]` checks to `unexpected-first-run.txt`
- [x] run `run.sh` - must pass before task 3 (SQLite and pgsql)
- ⚠️ `tsdb` fixtures are not validated: `maintenance_journal.creation_time` is `timestamptz` there, so the integer literal in `inject-defects.sql` would fail; a tsdb variant of that row is needed before `run.sh -d tsdb` can pass

### Task 3: Class-table, DCI, reset and report-only relations

**Files:**
- Modify: `src/server/tools/nxdbmgr/check.cpp`
- Modify: `tests/nxdbmgr-check/inject-defects.sql`
- Modify: `tests/nxdbmgr-check/cleanup-report-only.sql`
- Modify: `tests/nxdbmgr-check/expected-first-run.txt`
- Modify: `tests/nxdbmgr-check/unexpected-first-run.txt`

- [x] add class-table parent entries (radios via node-or-AP subquery, cluster_resources, dashboard_associations.dashboard_id, dc_table_columns, dct_threshold_conditions, dct_threshold_instances)
- [x] add DCI-parent entries (dci_schedules, dci_access) using the items UNION dc_tables subquery with an alias
- [x] add `ResetToZero` entries for nodes, interfaces.parent_iface, object_properties.drilldown_object_id, conditions.source_object, dashboards.forced_context_object_id, items/dc_tables.related_object; proxy prompts include "polling routing may change"
- [x] add `ReportOnly` entries: nodes.zone_guid and subnets.zone_guid (parent zones.zone_guid), dci_delete_list.node_id, business_service_checks.related_object / prototype_service_id / related_dci, cond_dci_map.node_id / dci_id, scheduled_tasks.object_id, dashboard_template_instances.instance_object_id
- [x] verify the array order is topological (objects, DCIs, dct_thresholds, threshold satellites)
- [x] inject one defect per new entry; report-only ones also go into `cleanup-report-only.sql`; negative fixtures: a `radios` row owned by an access point 990003, a DCI on node 990002 with `related_object=990001` (baseline gained node 990008 as holder of the injected proxy references, access point 990003, business service 990020, condition 990012, dashboard 990014, DCI 990010 and table DCI 990011; fixture comments must not contain `;` - `ExecSQLBatch` splits on it inside comments too)
- [x] run `run.sh` - must pass before task 4 (SQLite and pgsql)

### Task 4: EPP composite-key and chain relations

**Files:**
- Modify: `src/server/tools/nxdbmgr/check.cpp`
- Modify: `tests/nxdbmgr-check/inject-defects.sql`
- Modify: `tests/nxdbmgr-check/expected-first-run.txt`
- Modify: `tests/nxdbmgr-check/unexpected-first-run.txt`

- [x] add the EPP table name array and `static void CheckEppRelations()` joining on `(chain_id, rule_id)` against `event_policy`, deleting by both columns
- [x] add chain-level entries to the generic array (event_policy.chain_id, policy_chain_acl.chain_id, policy_chain_call_list.target_chain_id against event_policy_chain, `zeroAllowed=false`) and alarm_category_map.category_id against alarm_categories
- [x] call `CheckEppRelations()` after `CheckOrphanRelations()`
- [x] inject one row per policy table with a non-existing rule, one rule on a non-existing chain, one alarm_category_map row with a bad category; negative fixture: a rule 990010 on chain 0 with valid satellite rows (must not be reported, chain 0 row exists from `init`); baseline gained action 990062, alarm category 990063 and chain 990064 so the injected satellites survive the existing `CheckEPP` stage
- [x] run `run.sh` - must pass before task 5 (SQLite and pgsql)

### Task 5: Object class coverage, ghost properties, duplicate IDs

**Files:**
- Modify: `src/server/tools/nxdbmgr/check.cpp`
- Modify: `tests/nxdbmgr-check/inject-defects.sql`
- Modify: `tests/nxdbmgr-check/cleanup-report-only.sql`
- Modify: `tests/nxdbmgr-check/expected-first-run.txt`
- Modify: `tests/nxdbmgr-check/unexpected-first-run.txt`

- [x] add a static class-table list `{ table, displayName, builtinId }` matching the loader in `objects.cpp`
- [x] add `CheckObjectClassCoverage()` looping the list through `CheckMissingObjectProperties`; remove `CheckZones`, `CheckAccessPoints`, `CheckBusinessServices`, `CheckAssets` and the properties stages inside `CheckNodes`, `CheckComponents`, `CheckClusters`
- [x] add `CheckGhostObjectProperties()` (report only, local `s_builtinObjectIds` array)
- [x] add `CheckDuplicateObjectIds()` (report only, matching extension pairs excluded, all other combinations reported)
- [x] wire the three at the top of `CheckDatabase()`
- [x] inject: a `sensors` row without properties, a properties row 999500 with no class table, ID 999501 in both `nodes` and `object_containers` class 5, ID 999502 in `object_containers` class 32 and `nodes` (must be reported); negative fixture: business service 990020 with both `object_containers` class 28 and `business_services` rows (and dashboard 990014 with class 23; the duplicate node rows are container members so `CheckNodes` keeps them, and cleanup drops the `idata_`/`tdata_` tables the first run created for them)
- [x] run `run.sh` - must pass before task 6 (SQLite and pgsql)

### Task 6: Duplicate GUIDs, container cycles, duplicate subnets

**Files:**
- Modify: `src/server/tools/nxdbmgr/check.cpp`
- Modify: `tests/nxdbmgr-check/inject-defects.sql`
- Modify: `tests/nxdbmgr-check/cleanup-report-only.sql`
- Modify: `tests/nxdbmgr-check/expected-first-run.txt`
- Modify: `tests/nxdbmgr-check/unexpected-first-run.txt`

- [x] add `CheckDuplicateObjectGuids()` (GROUP BY guid HAVING count > 1, report only; lists the object IDs sharing the GUID)
- [x] add `CheckContainerCycles()` (in-memory adjacency from `container_members`, Tarjan SCC, one finding per cyclic component listing objects and memberships; review follow-up replaced the back-edge DFS, which missed overlapping cycles through an already finished node)
- [x] add `CheckDuplicateSubnets()` (GROUP BY ip_addr, ip_netmask, zone_guid, report only; lists the subnet IDs)
- [x] wire them in `CheckDatabase()` per the order in Solution Overview
- [x] inject: two properties rows sharing a GUID, containers 999600→999601→999600, overlapping cycles 999630→999631→999632→999630 plus 999630→999632, self-membership 999640, acyclic diamond 999650..999653 (must not be reported), two subnets with the same address and mask in zone 0; negative fixtures: same address in a different zone (990030), same address with a different mask (990031) (baseline gained zone 990032 so the other-zone subnet does not trip the `subnets.zone_guid` report-only relation)
- [x] run `run.sh` - must pass before task 7 (SQLite and pgsql)

### Task 7: Template, cluster and instance binding check

**Files:**
- Modify: `src/server/tools/nxdbmgr/check.cpp`
- Modify: `tests/nxdbmgr-check/inject-defects.sql`
- Modify: `tests/nxdbmgr-check/cleanup-report-only.sql`
- Modify: `tests/nxdbmgr-check/expected-first-run.txt`
- Modify: `tests/nxdbmgr-check/unexpected-first-run.txt`

- [x] add `CheckTemplateBindings()` for `items` and `dc_tables` classifying instance / cluster / template / deleted / unexpected in the documented order; only "deleted" (template_id absent from `object_properties`) gets the unbind fix, one UPDATE resetting both columns (one query per table with LEFT OUTER JOINs to the root DCI, clusters, templates, object_properties, cluster_members and dct_node_map; classification in code)
- [x] wire after `CheckDCISourceNodes()`
- [x] inject: instance DCI with missing root, cluster DCI on a node not in `cluster_members`, template DCI on a node missing from `dct_node_map`, DCI whose template ID exists nowhere; negative fixtures: instance DCI with valid root on node 990002, cluster 990040 with node 990002 as member and a cluster-applied DCI on that node, template 990041 bound via `dct_node_map` with a template DCI on the node, template 990041 also applied to cluster 990040 (template DCI whose `node_id` is the cluster, not a `nodes` row), and an instance DCI whose root is itself a template-inherited DCI
- [x] run `run.sh` - must pass before task 8 (SQLite and pgsql; the task 1 `[990002]` guard became six specific fragments because binding messages legitimately name node 990002 as the DCI owner)

### Task 8: Coupled interface peer and node path check repairs

**Files:**
- Modify: `src/server/tools/nxdbmgr/check.cpp`
- Modify: `tests/nxdbmgr-check/inject-defects.sql`
- Modify: `tests/nxdbmgr-check/expected-first-run.txt`
- Modify: `tests/nxdbmgr-check/unexpected-first-run.txt`

- [x] add `CheckInterfacePeers()` clearing peer_node_id, peer_if_id, peer_proto, peer_last_updated together when either endpoint is missing from `object_properties` (shared `CheckCoupledReferences()` for both stages; the prompt names only the missing half)
- [x] add `CheckNodePathCheckResults()` resetting path_check_reason, path_check_node_id, path_check_iface_id together when either ID is missing from `object_properties`
- [x] wire both after `CheckTemplateBindings()`
- [x] inject: interface peered to a deleted node, node whose path check points at a deleted interface; negative fixture: interface 990050 peered to access point 990003 in both columns, interface 990051 with a valid peer node and peer_if_id=0, node 990002 with a valid path check node and path_check_iface_id=0
- [x] run `run.sh` - must pass before task 9 (SQLite and pgsql)

### Task 9: Event code references

**Files:**
- Modify: `src/server/tools/nxdbmgr/check.cpp`
- Modify: `tests/nxdbmgr-check/inject-defects.sql`
- Modify: `tests/nxdbmgr-check/expected-first-run.txt`

- [x] add a static `{ table, column, defaultCode, zeroAllowed }` list for the eight references and `CheckEventCodeReferences()` that verifies the default exists in `event_cfg` before offering the reset; prompt says "EPP behavior may change" (findings are grouped per bad code and the reset is `UPDATE ... SET column=default WHERE column=code`, so no per-row ID column is needed and `event_policy`'s composite key is not an issue; one stage per column named `table.column`)
- [x] wire after `CheckNodePathCheckResults()`
- [x] inject one bad code per table.column, including an `event_policy` rule with a bad `alarm_timeout_event` (baseline gained trap mapping 990053 and table threshold 990052 as negatives; assertion 169 also proves the 55 seeded rules keep their `alarm_timeout_event`)
- [x] run `run.sh` - must pass before task 10 (SQLite and pgsql)

### Task 10: Notification channel references

**Files:**
- Modify: `src/server/tools/nxdbmgr/check.cpp`
- Modify: `tests/nxdbmgr-check/inject-defects.sql`
- Modify: `tests/nxdbmgr-check/cleanup-report-only.sql`
- Modify: `tests/nxdbmgr-check/expected-first-run.txt`
- Modify: `tests/nxdbmgr-check/unexpected-first-run.txt`

- [x] add `CheckNotificationChannelReferences()` (NOTIFICATION actions only, report only)
- [x] wire after `CheckEventCodeReferences()`
- [x] inject: notification action with unknown channel; negative fixture: forward-event action 990060 with a non-channel name, notification action 990061 on a channel seeded by `init`
- [x] run `run.sh` - must pass before task 11 (SQLite and pgsql)

### Task 11: CheckEPP last-entry guard

**Files:**
- Modify: `src/server/tools/nxdbmgr/check.cpp`
- Modify: `tests/nxdbmgr-check/inject-defects.sql`
- Modify: `tests/nxdbmgr-check/cleanup-report-only.sql`
- Modify: `tests/nxdbmgr-check/expected-first-run.txt`
- Modify: `tests/nxdbmgr-check/unexpected-first-run.txt`

- [x] rewrite the source and event loops in `CheckEPP()` to select `chain_id, rule_id, object_id, exclusion` / `chain_id, rule_id, event_code`, count remaining entries per rule and list kind, report only when deleting an inclusion would empty the rule's inclusion list or deleting an exclusion would leave the rule with no source inclusions and no exclusions, otherwise delete by the full key
- [x] inject: rule with two sources one dangling (deleted), rule with one dangling source (reported only, in cleanup script), rule with a dangling exclusion plus a valid inclusion (deleted), rule whose only source filter is one dangling exclusion (reported only), rule with two events one dangling (deleted), rule with one dangling event (reported only)
- [x] inject counting and full-key cases (audit follow-up added: two all-dangling events, three events with two dangling, two dangling exclusions without inclusions): a rule with three sources of which two are dangling (both deleted, one valid remains: the remaining-count must be recomputed after each delete, not read once); a rule with two sources both dangling (first deleted, second reported only because it is now the last inclusion); the same missing object ID referenced by two rules, one where it is the last inclusion (reported only) and one where it is not (deleted), and by a rule on a second chain (deleted) - proving deletion is by `(chain_id, rule_id, object_id)` and never `WHERE object_id=X`; the same shape for events with `event_code`
- [x] `assert-after-first-run.sql`: the valid sources/events of every EPP fixture rule are still present, the report-only rows are still present, and the deleted rows are gone
- [x] run `run.sh` - must pass before task 12 (SQLite and pgsql)

### Task 12: Verify acceptance criteria (Codex audit)

Codex runs this audit independently after the task 11 commit is reviewed. It is the final
acceptance gate before the branch is merged; it does not block the per-task commits.

- [x] every relation in Technical Details has an array entry and an injected defect (Codex audit: 91 generic entries, 9 composite tables, 8 event columns all covered)
- [x] every hand-written check (a) through (l) is wired in `CheckDatabase()` in the documented order
- [x] `nxdbmgr -m check` output for new stages follows the existing stage format (`-m` does not alter check output at all)
- [x] abort/rollback test on PostgreSQL (fault: first relation's column renamed to `object_idx`; error 42703 after earlier repairs were attempted, `Database check aborted`, no later stage, all 221 table fingerprints identical before and after): Codex specifies a deliberately broken relation entry (non-existing column); Claude applies it, builds and hands over; Codex runs the injected scratch database and verifies `check` prints `Database check aborted`, never the no-errors line, and that a state assertion batch afterwards still finds every injected defect (the transaction was rolled back, nothing repaired before the fault was lost); Claude reverts the fault and rebuilds
- [x] run `tests/nxdbmgr-check/run.sh` against SQLite and `run.sh -d pgsql -H 127.0.0.1 -U alk` against the local PostgreSQL server (each run creates and drops its own `nxdbmgr_check_<random>` database)
- [x] build shows no new warnings: `make -C src/server/tools/nxdbmgr` (only the macOS linker `-bind_at_load` deprecation, present in every build)
- [x] run `nxdbmgr check` against a copy of a real customer database and review every finding for false positives. Done 2026-09-24 on a PostgreSQL restore of a customer export (schema 46.2, 9 GB, 1113 nodes, 99k DCIs), see the audit record below

**Customer copy audit record.** Work was done on clones; the restored database itself was never
modified.

- Preparation: the restore was locked by the customer's server (forced `unlock` on the clone). Master
  `nxdbmgr` could not upgrade it (GitHub #3696, 61.36 step selects 7.0-only tables), so the clone was
  upgraded with the installed 6.1 and 6.2 tools first. The export had been made without the alarm and
  event log tables, so the 70.36 step failed (GitHub #3697); the eight tables were recreated empty
  from the 6.2 `dbschema_pgsql.sql` and the upgrade finished at 70.43. A pre-check snapshot of the
  upgraded clone was taken.
- Original-state run (report only, stdin closed): 2706 findings. False positive class found in this
  copy and fixed in a4db3bd4a3: ACL rows and template root memberships referencing built-in objects
  3, 4, 5, 6 in a database whose `object_properties` has rows only for built-ins 1, 2, 7, 9. The
  EPP source loop had the same gap; it was found by review, reproduced in scratch and fixed with
  fixtures in f0d3f1b8b8, but could not fire here: this copy has no `policy_source_list` rows.
  Rerun with a4db3bd4a3: 2701 findings.
- Forced run, attempt 1 (binary a4db3bd4a3): aborted. The export carried no per-object data tables; the existing
  "Data tables" stage created 1711 of 2226 missing `idata_`/`tdata_` tables inside the check's
  single transaction and PostgreSQL failed with "out of shared memory" (max_locks_per_transaction).
  The old stage does not propagate the failure; the next set-based stage detected the poisoned
  transaction, the check rolled back and printed `Database check aborted`. Rollback evidence: the
  report-only rerun showed the identical 2701 findings, all 1113 targets still lacked their `idata_`
  table, and the later creation of every `tdata_` table succeeded, which would have failed on any
  table left behind. Pre-existing limitation, GitHub #3698.
- Prepared-clone acceptance run (binary f0d3f1b8b8): the 3339 missing table and index objects were
  created outside the check on both the clone and the snapshot with the DDL of `dci_table_creation.h` (export artifact,
  not a defect). Report only: 475 findings. Forced: 475 found, 466 corrected. Re-run: 9 findings,
  all report-only.
- Repairs, each verified as a true positive against the snapshot: 240 DCIs on 60 nodes bound to a
  template that exists in no class table (unbound); hardware inventory of 76, software inventory of
  74 and access snapshot of 68 deleted objects (355, 16551 and 950 rows deleted); pollable_objects,
  dc_targets and icmp_statistics rows of 2 deleted objects; one template mapping to a deleted object
  (existing stage); one ACL row for object 8, which is not a built-in. Row counts of all 252
  non-data tables differ between snapshot and clone only in those tables plus `DBLockFlag` in
  `config` (lock bookkeeping); the set of DCIs with template_id=0 grew by exactly 240.
- Residuals (report only, correct): 7 duplicate subnet groups in zone 0 (same address and mask,
  none marked deleted; two of them carry a /26 and /29 name while the mask column says 24) and
  `dci_delete_list` rows of 2 deleted nodes.
- Independent content comparison by the reviewer: row counts plus order-independent sums of the
  two 64-bit halves of each row's MD5 JSON digest for all 252 non-data tables, snapshot with only
  the documented repairs applied virtually versus the repaired clone, differ in nothing (only the
  four `DBLock*` config rows excluded). This covers every column, including the 240 two-column
  unbinds, all preserved rows and the residuals.
- Logs and scripts are in the session scratch directory: upgrade logs,
  `missing-tables.sql`, `create-data-tables.sql`, `check-report-only.log`,
  `check-forced-attempt1-lock-exhaustion.log`, `check-report-only-2.log`, `check-forced.log`,
  `check-rerun.log`, `rowcount-diff.txt`.

### Task 13: [Final] Update documentation
- [x] update `src/server/tools/nxdbmgr/CLAUDE.md` check.cpp row to mention the relation table, how to add an entry, and the fixture rules for the harness
- [x] update `doc/` user documentation for `nxdbmgr check` if such a page exists in this repo (otherwise note in commit message that the admin guide needs an update) - no such page in `doc/`; the admin guide section on `nxdbmgr check` needs an update for the new stages and the report-only findings
- [x] move this plan to `docs/plans/completed/`

## Post-Completion

**GitHub issues filed** (server-side, out of scope for this change):
- #3691 `Zone::deleteFromDatabase` (`zone.cpp:217-223`) deletes `shared_secrets`, `snmp_communities`, `usm_credentials`, `well_known_ports` by object ID while the `zone` column holds the zone UIN; `ssh_credentials` not deleted at all.
- #3692 Never cleaned on object delete: `dashboard_associations` (both columns), `object_ai_data`, `port_stop_list`, `cluster_resources`, `radios`, `dci_delete_list`.
- #3693 Never cleaned on user or group delete: `responsible_users`, `alarm_category_acl`, `graph_acl`, `object_tools_acl`, `dci_summary_table_acl`, `policy_chain_acl`, `trusted_devices`, `ai_saved_prompts`.
- #3694 `DCTable::deleteFromDatabase` skips `dct_threshold_instances`; no DCI delete path touches `cond_dci_map.dci_id`.

**Manual verification:**
- run against a large production database copy and time the new stages; the generic stage is one query per relation so it should be fast, but `CheckContainerCycles` loads all memberships into memory
- confirm the admin guide section on `nxdbmgr check` still describes the output accurately
