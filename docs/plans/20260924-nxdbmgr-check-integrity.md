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
- build with `make -C src/server/tools/nxdbmgr && make -C src/server/tools/nxdbmgr install`
- stage names of the eight replaced property stages ("Zone object properties", "Node object
  properties", "Interface object properties", "Network service object properties", "Cluster object
  properties", "Access point object properties", "Business services", "Business service
  prototypes", "Assets") are replaced by one stage "Object class coverage". All other existing
  stage names and message formats stay as they are.

## Testing Strategy

- **defect injection test** (`tests/nxdbmgr-check/`, manual, not in build). `run.sh`:
  1. creates a scratch database (`sqlite` default: fresh file; `pgsql`/`tsdb`: drop and recreate
     via `-C dba/pass` or an explicit `dropdb`/`createdb`), writes `nxdbmgr.conf`, runs
     `nxdbmgr init <type>` with the type given explicitly;
  2. applies `baseline.sql` (valid fixture graph), runs `nxdbmgr -f check` once so the existing
     stages create the per-object data tables, then `nxdbmgr -E check` and asserts the output
     contains `Database doesn't contain any errors` (baseline is clean);
  3. applies `inject-defects.sql`, runs `nxdbmgr -f check | tee first.log`, asserts every
     fragment in `expected-first-run.txt` is present and every fragment in
     `unexpected-first-run.txt` is absent;
  4. applies `cleanup-report-only.sql`, runs `nxdbmgr -E check`, asserts exit code 0 **and**
     `Database doesn't contain any errors` in the output (the exit code alone is not enough:
     `check` always exits 0 and `-E` only fires from a prompt);
  5. every `batch` invocation fails the run if its output contains `SQL query failed`.
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
`SQLSelect` / `SQLQuery` does this so the stages stay short). `CheckDatabase()` treats the flag
as fatal: the transaction is rolled back and the final line is `Database check aborted`, never
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

**(e) Container cycles.** Load `container_members` into memory, DFS from every container ID,
report each cycle once as the ID path. Report only.

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

**(h) Interface peers.** Rows where `peer_node_id <> 0` or `peer_if_id <> 0` and either ID is not in
`object_properties` (AP peers store the AP ID in both columns, so class tables cannot be used).
Fix per interface: `UPDATE interfaces SET peer_node_id=0, peer_if_id=0, peer_proto=0, peer_last_updated=0 WHERE id=?`.

**(i) Node path check results.** Rows where `path_check_node_id <> 0` or `path_check_iface_id <> 0`
and either is not in `object_properties` (polymorphic: may be AP or VPN connector). Fix per node:
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
  run.sh                   # usage: run.sh [-d sqlite|pgsql|tsdb] [-c nxdbmgr.conf] [-b /path/to/nxdbmgr]
  baseline.sql             # valid fixture graph: container 990001, node 990002 as its member with a
                           # full object_properties row, an interface, a DCI; must check clean
  inject-defects.sql       # one block per defect, /* label */ comments only
  cleanup-report-only.sql  # removes report-only defects so the final run is clean
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
- Create: `tests/nxdbmgr-check/cleanup-report-only.sql`
- Create: `tests/nxdbmgr-check/expected-first-run.txt`
- Create: `tests/nxdbmgr-check/unexpected-first-run.txt`
- Modify: `tests/CLAUDE.md` (layout table row)

- [ ] write `run.sh` implementing the five steps from Testing Strategy: scratch DB per driver with explicit `init <type>`, baseline apply + `-f check` + clean `-E check` assertion, inject + `-f check` + expected/unexpected greps, cleanup + `-E check` + `Database doesn't contain any errors` grep, `SQL query failed` detection on every batch, non-zero exit on any failure
- [ ] write `baseline.sql`: container 990001 and node 990002 (member of the container) with complete `object_properties` rows, one interface, one DCI; labels as `/* */` only
- [ ] seed `inject-defects.sql` with one defect an existing check already catches (threshold for non-existing DCI) and `expected-first-run.txt` with its fragment
- [ ] seed `unexpected-first-run.txt` with `[990002]` to prove the baseline node is never reported
- [ ] add the directory to the layout table in `tests/CLAUDE.md` as manual / not in build
- [ ] run `run.sh` against SQLite - must pass before task 2

### Task 2: Generic orphan relation stage with object satellite relations

**Files:**
- Modify: `src/server/tools/nxdbmgr/check.cpp`
- Modify: `tests/nxdbmgr-check/inject-defects.sql`
- Modify: `tests/nxdbmgr-check/expected-first-run.txt`
- Modify: `tests/nxdbmgr-check/unexpected-first-run.txt`

- [ ] add `s_checkAborted` with `CheckSelect()` / `CheckQuery()` wrappers that set it on any SQL failure
- [ ] add `OrphanFix`, `OrphanRelation` and `static void CheckOrphanRelation(const OrphanRelation& r)` implementing the grouped LEFT OUTER JOIN query, per-parent prompt with row count, fix by same predicate, using the wrappers
- [ ] make `CheckDatabase()` honor `s_checkAborted` and check `DBBegin` / `DBCommit` results: roll back and print `Database check aborted` instead of the no-errors line
- [ ] add `static void CheckOrphanRelations()` iterating the array and call it from `CheckDatabase()` after `CheckAssetNodeLinks()`
- [ ] add all `DeleteRow` entries with parent `object_properties.object_id` from the relation table, including the two `zeroAllowed=false` entries, `physical_links`, `ap_common` and `object_access_snapshot`
- [ ] inject one orphan row per entry (deleted-object IDs in the 999xxx range) and a row with key zero in `interface_address_list` to prove `zeroAllowed=false`
- [ ] inject negative fixtures: an `acl` row and an `object_custom_attributes` row for node 990002; add `[990002]` checks to `unexpected-first-run.txt`
- [ ] run `run.sh` - must pass before task 3

### Task 3: Class-table, DCI, reset and report-only relations

**Files:**
- Modify: `src/server/tools/nxdbmgr/check.cpp`
- Modify: `tests/nxdbmgr-check/inject-defects.sql`
- Modify: `tests/nxdbmgr-check/cleanup-report-only.sql`
- Modify: `tests/nxdbmgr-check/expected-first-run.txt`
- Modify: `tests/nxdbmgr-check/unexpected-first-run.txt`

- [ ] add class-table parent entries (radios via node-or-AP subquery, cluster_resources, dashboard_associations.dashboard_id, dc_table_columns, dct_threshold_conditions, dct_threshold_instances)
- [ ] add DCI-parent entries (dci_schedules, dci_access) using the items UNION dc_tables subquery with an alias
- [ ] add `ResetToZero` entries for nodes, interfaces.parent_iface, object_properties.drilldown_object_id, conditions.source_object, dashboards.forced_context_object_id, items/dc_tables.related_object; proxy prompts include "polling routing may change"
- [ ] add `ReportOnly` entries: nodes.zone_guid and subnets.zone_guid (parent zones.zone_guid), dci_delete_list.node_id, business_service_checks.related_object / prototype_service_id / related_dci, cond_dci_map.node_id / dci_id, scheduled_tasks.object_id, dashboard_template_instances.instance_object_id
- [ ] verify the array order is topological (objects, DCIs, dct_thresholds, threshold satellites)
- [ ] inject one defect per new entry; report-only ones also go into `cleanup-report-only.sql`; negative fixtures: a `radios` row owned by an access point 990003, a DCI on node 990002 with `related_object=990001`
- [ ] run `run.sh` - must pass before task 4

### Task 4: EPP composite-key and chain relations

**Files:**
- Modify: `src/server/tools/nxdbmgr/check.cpp`
- Modify: `tests/nxdbmgr-check/inject-defects.sql`
- Modify: `tests/nxdbmgr-check/expected-first-run.txt`
- Modify: `tests/nxdbmgr-check/unexpected-first-run.txt`

- [ ] add the EPP table name array and `static void CheckEppRelations()` joining on `(chain_id, rule_id)` against `event_policy`, deleting by both columns
- [ ] add chain-level entries to the generic array (event_policy.chain_id, policy_chain_acl.chain_id, policy_chain_call_list.target_chain_id against event_policy_chain, `zeroAllowed=false`) and alarm_category_map.category_id against alarm_categories
- [ ] call `CheckEppRelations()` after `CheckOrphanRelations()`
- [ ] inject one row per policy table with a non-existing rule, one rule on a non-existing chain, one alarm_category_map row with a bad category; negative fixture: a rule 990010 on chain 0 with valid satellite rows (must not be reported, chain 0 row exists from `init`)
- [ ] run `run.sh` - must pass before task 5

### Task 5: Object class coverage, ghost properties, duplicate IDs

**Files:**
- Modify: `src/server/tools/nxdbmgr/check.cpp`
- Modify: `tests/nxdbmgr-check/inject-defects.sql`
- Modify: `tests/nxdbmgr-check/cleanup-report-only.sql`
- Modify: `tests/nxdbmgr-check/expected-first-run.txt`
- Modify: `tests/nxdbmgr-check/unexpected-first-run.txt`

- [ ] add a static class-table list `{ table, displayName, builtinId }` matching the loader in `objects.cpp`
- [ ] add `CheckObjectClassCoverage()` looping the list through `CheckMissingObjectProperties`; remove `CheckZones`, `CheckAccessPoints`, `CheckBusinessServices`, `CheckAssets` and the properties stages inside `CheckNodes`, `CheckComponents`, `CheckClusters`
- [ ] add `CheckGhostObjectProperties()` (report only, local `s_builtinObjectIds` array)
- [ ] add `CheckDuplicateObjectIds()` (report only, matching extension pairs excluded, all other combinations reported)
- [ ] wire the three at the top of `CheckDatabase()`
- [ ] inject: a `sensors` row without properties, a properties row 999500 with no class table, ID 999501 in both `nodes` and `object_containers` class 5, ID 999502 in `object_containers` class 32 and `nodes` (must be reported); negative fixture: business service 990020 with both `object_containers` class 28 and `business_services` rows
- [ ] run `run.sh` - must pass before task 6

### Task 6: Duplicate GUIDs, container cycles, duplicate subnets

**Files:**
- Modify: `src/server/tools/nxdbmgr/check.cpp`
- Modify: `tests/nxdbmgr-check/inject-defects.sql`
- Modify: `tests/nxdbmgr-check/cleanup-report-only.sql`
- Modify: `tests/nxdbmgr-check/expected-first-run.txt`
- Modify: `tests/nxdbmgr-check/unexpected-first-run.txt`

- [ ] add `CheckDuplicateObjectGuids()` (GROUP BY guid HAVING count > 1, report only)
- [ ] add `CheckContainerCycles()` (in-memory adjacency from `container_members`, DFS, report each cycle once as an ID path)
- [ ] add `CheckDuplicateSubnets()` (GROUP BY ip_addr, ip_netmask, zone_guid, report only)
- [ ] wire them in `CheckDatabase()` per the order in Solution Overview
- [ ] inject: two properties rows sharing a GUID, containers 999600→999601→999600, two subnets with the same address and mask in zone 0; negative fixtures: same address in a different zone (990030), same address with a different mask (990031)
- [ ] run `run.sh` - must pass before task 7

### Task 7: Template, cluster and instance binding check

**Files:**
- Modify: `src/server/tools/nxdbmgr/check.cpp`
- Modify: `tests/nxdbmgr-check/inject-defects.sql`
- Modify: `tests/nxdbmgr-check/cleanup-report-only.sql`
- Modify: `tests/nxdbmgr-check/expected-first-run.txt`
- Modify: `tests/nxdbmgr-check/unexpected-first-run.txt`

- [ ] add `CheckTemplateBindings()` for `items` and `dc_tables` classifying instance / cluster / template / deleted / unexpected in the documented order; only "deleted" (template_id absent from `object_properties`) gets the unbind fix, one UPDATE resetting both columns
- [ ] wire after `CheckDCISourceNodes()`
- [ ] inject: instance DCI with missing root, cluster DCI on a node not in `cluster_members`, template DCI on a node missing from `dct_node_map`, DCI whose template ID exists nowhere; negative fixtures: instance DCI with valid root on node 990002, cluster 990040 with node 990002 as member and a cluster-applied DCI on that node, template 990041 bound via `dct_node_map` with a template DCI on the node, template 990041 also applied to cluster 990040 (template DCI whose `node_id` is the cluster, not a `nodes` row), and an instance DCI whose root is itself a template-inherited DCI
- [ ] run `run.sh` - must pass before task 8

### Task 8: Coupled interface peer and node path check repairs

**Files:**
- Modify: `src/server/tools/nxdbmgr/check.cpp`
- Modify: `tests/nxdbmgr-check/inject-defects.sql`
- Modify: `tests/nxdbmgr-check/expected-first-run.txt`
- Modify: `tests/nxdbmgr-check/unexpected-first-run.txt`

- [ ] add `CheckInterfacePeers()` clearing peer_node_id, peer_if_id, peer_proto, peer_last_updated together when either endpoint is missing from `object_properties`
- [ ] add `CheckNodePathCheckResults()` resetting path_check_reason, path_check_node_id, path_check_iface_id together when either ID is missing from `object_properties`
- [ ] wire both after `CheckTemplateBindings()`
- [ ] inject: interface peered to a deleted node, node whose path check points at a deleted interface; negative fixture: interface 990050 peered to access point 990003 in both columns
- [ ] run `run.sh` - must pass before task 9

### Task 9: Event code references

**Files:**
- Modify: `src/server/tools/nxdbmgr/check.cpp`
- Modify: `tests/nxdbmgr-check/inject-defects.sql`
- Modify: `tests/nxdbmgr-check/expected-first-run.txt`

- [ ] add a static `{ table, column, idColumn, defaultCode, zeroAllowed }` list for the eight references and `CheckEventCodeReferences()` that verifies the default exists in `event_cfg` before offering the reset; prompt says "EPP behavior may change"
- [ ] wire after `CheckNodePathCheckResults()`
- [ ] inject one bad code per table.column, including an `event_policy` rule with a bad `alarm_timeout_event`
- [ ] run `run.sh` - must pass before task 10

### Task 10: Notification channel references

**Files:**
- Modify: `src/server/tools/nxdbmgr/check.cpp`
- Modify: `tests/nxdbmgr-check/inject-defects.sql`
- Modify: `tests/nxdbmgr-check/cleanup-report-only.sql`
- Modify: `tests/nxdbmgr-check/expected-first-run.txt`
- Modify: `tests/nxdbmgr-check/unexpected-first-run.txt`

- [ ] add `CheckNotificationChannelReferences()` (NOTIFICATION actions only, report only)
- [ ] wire after `CheckEventCodeReferences()`
- [ ] inject: notification action with unknown channel; negative fixture: forward-event action 990060 with a non-channel name
- [ ] run `run.sh` - must pass before task 11

### Task 11: CheckEPP last-entry guard

**Files:**
- Modify: `src/server/tools/nxdbmgr/check.cpp`
- Modify: `tests/nxdbmgr-check/inject-defects.sql`
- Modify: `tests/nxdbmgr-check/cleanup-report-only.sql`
- Modify: `tests/nxdbmgr-check/expected-first-run.txt`
- Modify: `tests/nxdbmgr-check/unexpected-first-run.txt`

- [ ] rewrite the source and event loops in `CheckEPP()` to select `chain_id, rule_id, object_id, exclusion` / `chain_id, rule_id, event_code`, count remaining entries per rule and list kind, report only when deleting an inclusion would empty the rule's inclusion list or deleting an exclusion would leave the rule with no source inclusions and no exclusions, otherwise delete by the full key
- [ ] inject: rule with two sources one dangling (deleted), rule with one dangling source (reported only, in cleanup script), rule with a dangling exclusion plus a valid inclusion (deleted), rule whose only source filter is one dangling exclusion (reported only), rule with two events one dangling (deleted), rule with one dangling event (reported only)
- [ ] run `run.sh` - must pass before task 12

### Task 12: Verify acceptance criteria
- [ ] every relation in Technical Details has an array entry and an injected defect
- [ ] every hand-written check (a) through (l) is wired in `CheckDatabase()` in the documented order
- [ ] `nxdbmgr -m check` output for new stages follows the existing stage format
- [ ] a deliberately broken relation entry (non-existing column) makes `check` print `Database check aborted` on PostgreSQL, then revert it
- [ ] run `tests/nxdbmgr-check/run.sh` against SQLite and `run.sh -d pgsql` against the local PostgreSQL test server
- [ ] build shows no new warnings: `make -C src/server/tools/nxdbmgr`
- [ ] run `nxdbmgr check` against a copy of a real customer database and review every finding for false positives before committing

### Task 13: [Final] Update documentation
- [ ] update `src/server/tools/nxdbmgr/CLAUDE.md` check.cpp row to mention the relation table, how to add an entry, and the fixture rules for the harness
- [ ] update `doc/` user documentation for `nxdbmgr check` if such a page exists in this repo (otherwise note in commit message that the admin guide needs an update)
- [ ] move this plan to `docs/plans/completed/`

## Post-Completion

**GitHub issues filed** (server-side, out of scope for this change):
- #3691 `Zone::deleteFromDatabase` (`zone.cpp:217-223`) deletes `shared_secrets`, `snmp_communities`, `usm_credentials`, `well_known_ports` by object ID while the `zone` column holds the zone UIN; `ssh_credentials` not deleted at all.
- #3692 Never cleaned on object delete: `dashboard_associations` (both columns), `object_ai_data`, `port_stop_list`, `cluster_resources`, `radios`, `dci_delete_list`.
- #3693 Never cleaned on user or group delete: `responsible_users`, `alarm_category_acl`, `graph_acl`, `object_tools_acl`, `dci_summary_table_acl`, `policy_chain_acl`, `trusted_devices`, `ai_saved_prompts`.
- #3694 `DCTable::deleteFromDatabase` skips `dct_threshold_instances`; no DCI delete path touches `cond_dci_map.dci_id`.

**Manual verification:**
- run against a large production database copy and time the new stages; the generic stage is one query per relation so it should be fast, but `CheckContainerCycles` loads all memberships into memory
- confirm the admin guide section on `nxdbmgr check` still describes the output accurately
