# NetXMS Green DC Monitor — Server Integration Design

**Status:** draft for review (rev. 2 — sample-attribute layer generalized to core infrastructure)
**Project:** NetXMS Green DC Monitor · 1.2.1.2.i.2/1/24/A/CFLA/006
**Companion documents:**
- *GreenDC KPI Data Model & Conceptual Architecture v0.1* (external, "the KPI doc") — defines the
  containers and contracts this design implements
- [`doc/ENTSOE_Integration_Design.md`](../ENTSOE_Integration_Design.md) — carbon-intensity signal
  acquisition (implemented; feeds the CUE strand, out of scope here)
- [`doc/Traffic_Observer_Integration_Design.md`](../Traffic_Observer_Integration_Design.md) —
  implemented precedent for adding new object classes end-to-end
- [`doc/design/DCI_Data_Aggregation.md`](DCI_Data_Aggregation.md) — implemented precedent for
  per-sample attributes flowing NXCP → Java client → charts → WebAPI

This document maps the KPI doc's structural decisions onto the actual NetXMS codebase: which
existing mechanisms are reused, which new classes/tables/commands are introduced, and where every
change lands. Section 11 feeds corrections back into the KPI doc, including one normative fix to
the rack containment model.

---

## 1. Scope

**In scope:** Green DC object hierarchy (Facility, PowerDomain, CoolingZone, extended Rack);
uncertain-value sample attributes; slot taxonomy, binding and validation; metering profile; KPI
engine with the open/commercial provider seam; daily energy-component records; settlement
lifecycle; annual frozen snapshots; NXCP/REST/NXSL query surfaces; database schema and upgrade.

**Out of scope:** the estimation methodology itself (commercial provider internals); carbon-signal
data model (see ENTSO-E design); console UI beyond enumerating touch points (KPI doc defers UI);
BACnet acquisition (ordinary DCIs).

---

## 2. Codebase grounding — what is reused

The KPI doc's principle "fix the container, defer the science" translates here into "reuse the
container NetXMS already has wherever one exists." Every major requirement lands on a proven
mechanism:

| Requirement (KPI doc) | Existing mechanism reused | Where |
|---|---|---|
| Overlay hierarchies, multi-parent Rack (§4) | Containment is already multi-parent: `NObject::m_parentList`, edge table `container_members`, delete-detaches-when-multi-parent | `nxsrvapi.h:588`, `sql/schema.in:864`, `netobj.cpp:974` |
| New native object classes (§4.1) | Traffic Observer class-addition checklist (classes 40/41 added recently) | `doc/Traffic_Observer_Integration_Design.md` §3 |
| Slot binding as DCI interpretation tag (§5.1) | `systemTag` + `relatedObject` pair — exactly how interface-utilization interpretation tags already work | `nms_dcoll.h:348,393`, `dcitem.cpp:1377-1415` |
| Controlled vocabulary with config-time validation (§5.5) | Asset-management attribute schema validation flow (`ValidateAssetPropertyValue` → RCC + reason) | `asset_management.cpp:927-1100` |
| Subtree-wide DCI resolution by tag (§3.1) | `addChildDCTargetsToList` + `getDCObjectsByFilter` composition (AI tools already do a subtree tag scan) | `netobj.cpp:2772`, `dcowner.cpp:1270`, `aitools/operational.cpp:611-636` |
| Daily per-entity computation with settlement lag (§6.2) | DCI aggregation rollup: scheduled task, close window, watermarks, idempotent upserts | `dcagg.cpp:591-683`, `main.cpp:1754-1755` |
| Open/commercial provider seam (§3.2) | Module provider interfaces: helpdesk link (single), cloud/traffic connectors (named registry) | `hdlink.cpp:146`, `cloud_connector.cpp:68-96` |
| Commercial module owning its own schema | nxdbmgr module extension entry points (`NXM_UpgradeDB` etc.) — supported, unused in open tree | `nxdbmgr/modules.cpp:95-100` |
| Per-sample extra attributes on the wire (§2.1) | History wire format option bits + trailing per-row fields (`sampleCount` precedent) | `session.cpp:5465`, `NXCSession.java:6500-6564` |
| Point-estimate degradation for legacy readers (R1) | Values are stored as plain `varchar(255)` in `idata`; every existing reader sees only that column | `dci_table_creation.h:48-81` |
| Open-interval / settled-record lifecycle | Business-service downtime (`to_timestamp=0` = open) and housekeeper's closed-records-only retention | `bizservice.cpp:384-444`, `hk.cpp:484-499` |
| Immutable audit artifacts with supersession | No direct precedent — new tables, but versioned rows + audit log via `writeAuditLog` conventions | §6.6 |

---

## 3. Green DC object hierarchy

### 3.1 New classes

Three new native classes, next free class IDs (current max is `OBJECT_OBSERVATIONPOINT = 41`,
`nxcldefs.h:150`):

| Class | ID | Base classes | Role |
|---|---|---|---|
| `Facility` | `OBJECT_FACILITY = 42` | `DataCollectionTarget`, `ContainerBase` | EED reporting boundary; root of the Green DC subtree; owner of metering profile, settlement config, daily records, snapshots |
| `PowerDomain` | `OBJECT_POWERDOMAIN = 43` | `DataCollectionTarget`, `ContainerBase` | Node in the electrical-distribution overlay; nests to model grid entry → UPS → PDU lineage |
| `CoolingZone` | `OBJECT_COOLINGZONE = 44` | `DataCollectionTarget`, `ContainerBase` | Node in the thermal overlay; nests to model plant → zone structure |

The `DataCollectionTarget + ContainerBase` combination follows `Collector` and `Circuit`
(`nms_objects.h:5685, 5729`) — objects that both contain children and can own DCIs. Making all
three DC targets is deliberate:

- Facility-level KPI mirror DCIs (§6.7) and domain-level summing slots need a DCI owner at that
  hierarchy level.
- `showThresholdSummary()` and the threshold/event machinery come for free.
- `getInternalMetric` gives a zero-cost query surface for current KPI values (§6.7).

`Rack` (`OBJECT_RACK = 32`, `AbstractContainer`, `rack.cpp`) is **extended, not duplicated**: it
gains no new C++ state. Its role in the Green DC model is purely relational (membership edges to
PowerDomains and CoolingZones) plus DCI bindings on devices it already contains. The existing
node↔rack binding machinery (`Node::updatePhysicalContainerBinding`, `node.cpp:14949`) operates on
a rack's *children* and is unaffected by the rack acquiring additional *parents*.

### 3.2 Power topology — correction to the KPI doc

The KPI doc §4.1 states Rack is a "shared leaf with **one electrical parent edge** and one thermal
parent edge," while §4.2 makes dual A/B feeds the normative case. The §4.1 wording is wrong and is
superseded here:

> **Rack is a shared leaf with one or more electrical parent edges (one per feed) and one thermal
> parent edge.**

The electrical overlay is a **hierarchy of PowerDomains** representing the distribution lineage:

```
Facility
 ├── PowerDomain "Utility entry 1"        (type GRID_ENTRY)
 │    └── PowerDomain "UPS-A"             (type UPS)
 │         ├── PowerDomain "PDU-A1"       (type PDU)
 │         │     ├── Rack R101  ◄─────────── feed A edge
 │         │     └── Rack R102
 │         └── PowerDomain "PDU-A2" ...
 ├── PowerDomain "Utility entry 2"        (type GRID_ENTRY)
 │    └── PowerDomain "UPS-B"             (type UPS)
 │         └── PowerDomain "PDU-B1"
 │               ├── Rack R101  ◄─────────── feed B edge (same rack, second parent)
 │               └── Rack R102
 └── CoolingZone "Chiller plant"
      └── CoolingZone "CRAH zone 1"
            ├── Rack R101       ◄─────────── thermal edge (third parent)
            └── Rack R102
```

A rack links to the **lowest metered domain of each feed** (typically PDU level; UPS level where
PDUs are unmetered). PowerDomain carries:

- `domainType` enum: `GRID_ENTRY`, `GENERATOR`, `UPS`, `PDU`, `BUSWAY`, `OTHER`
- `feedTag` (short string, e.g. "A"/"B") — feed identity as a first-class object attribute
- `equipmentId` — optional object reference to the Node representing the physical device (UPS,
  metered PDU). The device node itself stays wherever the network model puts it (subnet,
  container); this reference is not a containment edge, so no cycle risk. The `ups` subagent
  (`src/agent/subagents/ups/`) and SNMP-polled PDUs supply that node's DCIs.

Refinement of KPI doc rule **R7** ("feed identity lives on DCI binding tags"): with an explicit
PowerDomain hierarchy, feed identity lives primarily on the **PowerDomain object** (`feedTag`), and
a DCI's feed is implied by which domain it is bound to (§5). A tag *qualifier* remains available
(§5.1) for the rare case of per-feed metering bound at rack level. This is strictly more expressive
than tags alone and keeps R8 intact: membership edges are never summed; energy balance operates on
the flow graph implied by bindings (KPI doc §4.2, unchanged).

CoolingZone carries `zoneType` enum: `PLANT`, `ZONE`, `OTHER`, plus the same optional
`equipmentId` (chiller/CRAH node).

### 3.3 Containment rules

`IsValidParentClass` (`objects.cpp:2110-2242`) gains:

| Parent | Accepted new children |
|---|---|
| `OBJECT_SERVICEROOT`, `OBJECT_CONTAINER`, `OBJECT_COLLECTOR` | `OBJECT_FACILITY` |
| `OBJECT_FACILITY` | `OBJECT_POWERDOMAIN`, `OBJECT_COOLINGZONE`, `OBJECT_RACK`, `OBJECT_CONTAINER`, `OBJECT_NODE`, `OBJECT_CHASSIS`, `OBJECT_SENSOR` |
| `OBJECT_POWERDOMAIN` | `OBJECT_POWERDOMAIN` (nesting), `OBJECT_RACK`, `OBJECT_NODE`, `OBJECT_SENSOR` |
| `OBJECT_COOLINGZONE` | `OBJECT_COOLINGZONE` (nesting), `OBJECT_RACK`, `OBJECT_NODE`, `OBJECT_SENSOR` |

Notes:

- One Facility per EED submission (KPI doc §4.1). Multi-facility deployments hold several Facility
  objects under Infrastructure. No enforcement of "one facility" — the boundary is per object, not
  per server.
- Nodes/Sensors are accepted under Power/Cooling domains so that meter and equipment devices can be
  *grouped* there for navigation; grouping carries no semantic weight (R8).
- Multi-parent semantics need no new mechanism: edges persist in `container_members`
  (`sql/schema.in:864`), `NetObj::deleteObject` already detaches instead of deleting when
  `getParentCount() > 1` (`netobj.cpp:974`).
- Cycle safety: the only loop check today is on the client bind path (`ChangeObjectBinding`,
  `objects.cpp:2754-2764`, returns `RCC_OBJECT_LOOP`). Green DC introduces no internal `linkObjects`
  call sites that bypass it (all Green DC hierarchy edits go through the ordinary bind/unbind or
  create paths), so the existing guarantee is preserved.

### 3.4 Class-specific persistent state

Three small tables (full schema in §9). Design follows `Rack` (`racks` table holds only
class-specific columns; everything else comes from `object_properties` / `object_containers` /
`container_members`):

- `gdc_facilities` — provider name, settlement lag (days), reporting-year start (MM-DD), coverage
  level, flags.
- `gdc_power_domains` — domain type, feed tag, equipment object id.
- `gdc_cooling_zones` — zone type, equipment object id.

All three classes are loaded via the standard `LoadObjectsFromTable<T>` calls in `LoadObjects`
(`objects.cpp:1607-1705`), registered in `object_containers` with their class id, and cached under
`AF_CACHE_DB_ON_STARTUP` alongside `racks` (`objects.cpp:1472-1552`).

### 3.5 New-class touch-point checklist

Per the verified Traffic Observer checklist plus the object-model survey; every row is mandatory:

| Layer | Location |
|---|---|
| Class IDs | `include/nxcldefs.h:111-153` (append 42–44) |
| Class-name tables (positional!) | `netobj.cpp:31-62` — append `Facility`, `PowerDomain`, `CoolingZone` at indexes 42–44 in **both** W and A tables |
| Class declarations + impls | `nms_objects.h`; new `src/server/core/gdc_objects.cpp` (+ `Makefile.am`) |
| NXCP create factory | `ClientSession::createObject` switch, `session.cpp:6727-6916` |
| JSON create factory | `CreateObjectFromJSON` switch, `objects.cpp:2950-3146` |
| Index insert/remove, class→index map | `objects.cpp:298`, `:489`, `:1120` |
| Startup load + DB cache list | `objects.cpp:1607-1705`, `:1472-1552` |
| Parent/child matrix | `IsValidParentClass`, `objects.cpp:2110-2242` (§3.3) |
| HA sync: load rank, instance factory, relation ownership | `hasync.cpp:75-113`, `:119-157`, `:165-191` (rank: Facility with racks/containers, domains after) |
| NXSL classes + object-query constants | `nms_script.h`, `nxsl_classes.cpp` (`NXSL_FacilityClass` etc., modeled on `NXSL_ObservationPointClass:7768`); `object_queries.cpp:~549` (`FACILITY`, `POWERDOMAIN`, `COOLINGZONE` constants) |
| Java class IDs + object classes + factory | `AbstractObject.java:78-121`; new `Facility.java`, `PowerDomain.java`, `CoolingZone.java`; `NXCSession.java:1701` and `:7269` switches |
| nxmc: icons, subtree filter, create menu, property pages, selection filters | `ObjectIcons.java:80`, `ObjectBrowser.java:555-633` (INFRASTRUCTURE list) and `:665` (move validity), `ObjectCreateMenuManager.java`, `ObjectPropertiesManager.java`, `ObjectSelectionFilterFactory.java` |
| WebAPI | automatic via `toJson`/`CreateObjectFromJSON`; class-name enums in `openapi.yaml:~11540`, `~12150` |
| nxdbmgr | schema (§9); `g_tables[]` regenerates from `sql/schema.in` automatically (`create_table_list.pl`) |

---

## 4. Per-sample attribute infrastructure (core)

The attribute vocabulary defined by the KPI doc §2.1 — quality class, bounds, completeness,
method identity — contains nothing GreenDC-specific: it is the generic vocabulary of any engine
that derives, estimates, fills or degrades sample data. This section therefore specifies **core
DCI infrastructure**, with the GreenDC KPI engine as its first consumer. Types live in
`nms_dcoll.h`, tables carry no `gdc_` prefix, and `greendc.h` retains only slots, components and
the provider contract.

### 4.1 Structural decision: companion attribute storage, not a new data type

The KPI doc §2 calls for "a new first-class DCI value type." Two implementation strategies were
evaluated against the codebase:

**(a) New `DCI_DT_*` data type.** Rejected. The data-type enum (`nms_common.h:963-974`) is shared
by agent, server, drivers and tools; the type drives ~10 switch sites in the value pipeline (delta
calculation `dcitem.cpp:1737-1909`, conversion `:1698`, NXSL mapping `:2012`, wire encoding
`session.cpp:5505-5556`, …). Client-version compatibility is *not* a factor — the release policy
couples client and server at major.minor, so same-release clients always know the format — but two
decisive objections remain. First, provenance is a property of the sample, not the series (KPI doc
principle 2): the normative case is a mixed series — metered samples interleaved with estimator
backfill — and a series-level type cannot represent it. Second, the transformation pipeline is a
string round-trip (`vm->createValue(value.getString())`, `dcitem.cpp:1920`), so attributes could
not survive it in-band anyway.

**(b) Point estimate stays in the normal pipeline; attributes ride beside it.** Chosen. The point
estimate is an ordinary `DCI_DT_FLOAT` value flowing through the untouched collection/storage
pipeline; per-sample attributes live in a companion table keyed `(item_id, sample_timestamp)` and
are written **only** by producers that actually have them. This makes the KPI doc's semantic rules
structural properties rather than conventions:

- **R1 (legacy degradation)** — every existing consumer (idata readers, NXSL, thresholds, PDS
  drivers, Grafana SQL, export) reads the same `idata_value varchar(255)` it always did.
  Nothing degrades because nothing changed for them.
- **R2 (aggregation blanking)** — built-in aggregation (`dcagg.cpp`, TSDB continuous aggregates,
  `PrepareAggregatedDataSelect`, cluster aggregation) never touches the attribute table, so
  derived values *cannot* carry uncertainty. Blanking is enforced by construction, not by code
  discipline.
- **R3 (producer exclusivity)** — the attribute table has exactly two writers: the KPI engine and
  the internal attributed-write API (§4.4). There is no client-facing mutation path.

A DCI is marked as attribute-bearing with a new flag bit `DCF_HAS_SAMPLE_ATTRIBUTES 0x400000`
(next free bit after `DCF_ADD_INSTANCE_OID_COLUMN 0x200000`, `nxcldefs.h:1062`). The flag lets
read paths skip the attribute join for the overwhelming majority of DCIs.

### 4.2 Attribute record

Fixed, compact, frozen vocabulary (KPI doc §2.1 — deliberately not a property bag):

```cpp
// src/server/include/nms_dcoll.h — core, not GreenDC-specific
enum class SampleQualityClass : int16_t
{
   MEASURED = 0, PROXY = 1, ESTIMATED = 2, INTERPOLATED = 3, MISSING = 4
   // Append-only registry: future engines may add classes (e.g. FORECAST).
   // Existing values never change meaning — audit-grade frozen semantics.
};

struct SampleAttributes
{
   SampleQualityClass qualityClass;
   double lowerBound;        // at the declared coverage level
   double upperBound;
   double completeness;      // 0.0 .. 1.0
   uint32_t methodId;        // FK into computation_methods
   uint32_t methodVersion;
};
```

Coverage level is **not** stored per sample: it is declared per method version in the method
registry (§4.5), resolving OQ-4 in favor of "per-method declared" with a system default
(`GreenDC.DefaultCoverageLevel = 95`).

`MISSING` as a *stored* class appears only on computed records (a component the engine computed
nothing for). For collected series, a missing sample is the **absence of an idata row** — the
engine derives `MISSING` from the expected-sample grid at computation time. No placeholder rows,
honoring R5 (no silent substitution) without fabricating data.

### 4.3 Storage

One global table (not per-object — volume is meter-scale, not interface-counter-scale):

```
dci_sample_attributes
   item_id           integer      not null
   sample_timestamp  SQL_INT64    not null   -- ms, same epoch as idata_timestamp
   quality           integer      not null
   bound_low         SQL_DOUBLE   null
   bound_high        SQL_DOUBLE   null
   completeness      SQL_DOUBLE   null
   method_id         integer      not null
   method_version    integer      not null
   PRIMARY KEY(item_id, sample_timestamp)
```

On TSDB builds the table becomes a hypertable with the same chunking as `idata_sc_*`
(`schema.in:1216-1221` pattern). Retention: deleted in lockstep with the owning DCI's history —
a companion `DELETE` in `DCItem::cleanDCIData`'s housekeeper path and in DCI deletion, plus
`drop_chunks` on TSDB (`hk.cpp:328-349` pattern).

Write path: attributed writes go through a single internal function rather than threading extra
payload through `DELAYED_IDATA_INSERT` (`dbwrite.cpp:42-49`):

```cpp
void NXCORE_EXPORTABLE WriteAttributedSample(uint32_t nodeId, uint32_t dciId,
      Timestamp timestamp, double value, const SampleAttributes& attributes);
```

It pushes the point estimate through the normal `processNewDCValue` path (so cache, thresholds and
PDS fan-out behave identically — same entry point used by push DCIs, `session.cpp:11275`) and
queues the attribute row on the standard delayed-write queue. Determinism (R4) makes replays
idempotent: attribute upsert is `INSERT … ON CONFLICT DO UPDATE` / `MERGE`
(`BuildAggregateUpsert` pattern, `dcagg.cpp:119-173`).

### 4.4 Producers and write-side enforcement

| Producer | Point estimate | Attributes |
|---|---|---|
| Ordinary collector (agent/SNMP/web service) on a slot-bound DCI | normal pipeline | none stored — implicit `MEASURED`, no bounds, completeness 1.0 |
| Collection failure | no idata row (existing behavior) | none — a consuming engine derives `MISSING` from the gap |
| Computation engine (GreenDC mirror DCIs, §6.7; future engines) | via `WriteAttributedSample` | explicit, supplied by the engine |
| Commercial estimator (via GreenDC provider contract) | via engine only — providers never write storage (R6) | carried on provider output, written by engine |

Engine-written series get a dedicated **data origin** `DS_COMPUTED`. The data-origin enum is the
established discriminator that already changes collection behavior (push origins are not polled);
a `DS_COMPUTED` DCI additionally rejects `CMD_PUSH_DCI_DATA` and transformation scripts —
`WriteAttributedSample` is its only feed. This enforces producer exclusivity (R3) at the source
instead of by convention, while keeping the object model unchanged: no third `DCObject` variation
(a series-level type cannot express per-sample provenance; see §13).

Prospective second consumers already visible in the tree — the reason this layer is core rather
than GreenDC-owned:

- the vestigial prediction-engine seam (`items.npe_name`, `PredictionEngine` forward declaration
  in `nxmodule.h`) — forecast series need exactly bounds + method version, and would motivate a
  `FORECAST` quality class;
- per-sample anomaly marking — `raw_dci_values.anomaly_detected` is a per-DCI-latest quality bit
  that a future revision could carry per sample;
- cluster aggregation with missing members (`DCF_AGGREGATE_WITH_ERRORS`) — the completeness
  fraction is the quantified form of that flag.

### 4.5 Method registry

Core table shared by all engines; methods are namespaced by the owning engine:

```
computation_methods
   id             integer       not null   -- IDG-allocated
   engine         varchar(15)   not null   -- 'GREENDC'; future engine identifiers
   name           varchar(63)   not null   -- e.g. 'direct-computation', 'nxee-estimator'
   version        integer       not null
   tier           char(1)       not null   -- 'O' open / 'C' commercial
   coverage_level integer       not null   -- percent, e.g. 95
   description    varchar(255)  null
   PRIMARY KEY(id, version)
```

Providers self-register their method identity at load; the GreenDC engine refuses to run a
provider whose (id, version) is not registered — the registry is what makes the supersede chain
(§6.6) resolvable years later.

### 4.6 Wire protocol and client surfaces

History reads: a new option bit in the `CMD_DCI_DATA` binary header flags word. Bits `0x0001`
(raw), `0x0002` (avg/min/max) and `0x0008` (sample count) are taken (`session.cpp:5452-5490`);
`0x0004` is avoided as historically ambiguous — allocate **`0x0010` = per-row trailing attribute
block**: `int16 quality`, `double lowerBound`, `double upperBound`, `double completeness`,
`int32 methodId`, `int32 methodVersion`, appended **after** all existing per-row fields (parser
constraint: unknown trailing bytes desynchronize `parseDataRows`, `NXCSession.java:6500-6575`, so
the bit is strictly opt-in).

The server sets the bit only when the client asked for attributes via a new flag field in the
`CMD_GET_DCI_DATA` request. Under the release policy, client and server are version-coupled at
major.minor, so this opt-in is **not** a compatibility gate — it is an efficiency choice: the
attribute block adds ~34 bytes per row and most queries do not want it.

Client model: `DciDataRow` gains `qualityClass`, `lowerBound`, `upperBound`, `completeness`,
`methodId`, `methodVersion` following the `sampleCount` side-channel precedent
(`DciDataRow.java:35`, `setSampleCount` "used by NXCP parser"); `getValue()` semantics unchanged.

REST: `v1/objects/:object-id/data-collection/:dci-id/history` (`webapi/datacoll.cpp:75`) gains
`attributes=true`; row objects then carry
`"quality"`, `"bounds": {"low":…, "high":…, "coverageLevel":…}`, `"completeness"`, `"method"`.
`openapi.yaml` updated (`webapi/CLAUDE.md` requirement).

NXSL: `NXSL_DataPointClass` currently wraps `std::pair<time_t, String>`
(`nxsl_classes.cpp:7235-7258`); payload becomes a small struct with optional attributes, and
`getAttr` adds read-only `quality`, `lowerBound`, `upperBound`, `completeness`, `methodId`,
`methodVersion` (null when absent). `GetDCIValuesEx` gains an `attributes:` named-parameter fetch
mode. Read-only surface only — R3 holds because NXSL DCI objects are `DCObjectInfo` snapshots and
scripts have no write path to tags or attributes.

Charts (deferred with UI, but the seam exists): `LineChart` already renders min/max companion
series from `DciDataRow` (`LineChart.java:926-980` swt, `:880` rwt); bounds map onto the same
band machinery when the UI strand picks this up.

### 4.7 Scope rules for the core layer

Two doctrines follow from making this layer core rather than GreenDC-private:

1. **Only computation engines write attributes; generic consolidation never does.** R2 was a
   GreenDC rule; it is now server doctrine. Built-in rollups (`dcagg.cpp`, TSDB continuous
   aggregates, on-the-fly bucketing, cluster aggregation) operate on point estimates and never
   emit attribute rows. Uncertainty does not average and correlated errors do not cancel — a
   built-in consolidation that emitted bounds would produce confidently wrong intervals, worse
   than none.
2. **The data plane is generic; control planes are per-engine.** Sample attributes, quality
   vocabulary, method registry, `WriteAttributedSample`, `DS_COMPUTED`, and the wire/REST/NXSL
   exposure are core. Slot taxonomies, record schemas, settlement lifecycles, and provider
   contracts are owned by each engine (GreenDC's in §§5–6). No generic "engine framework" is
   introduced while there is a single engine: if a second engine materializes, extracting a
   shared skeleton is a mechanical refactor, whereas guessing its shape now is speculative
   abstraction.

---

## 5. Slot taxonomy and EED binding

### 5.1 Binding mechanism

A slot binding is the pair already used by the interface-utilization machinery
(`dcitem.cpp:1377-1415`): **`systemTag` carries the slot, `relatedObject` carries the hierarchy
binding point.**

- `systemTag` = `greendc.<slot>[/<qualifier>]`, e.g. `greendc.eed.eit`,
  `greendc.power.feed/A`. Fits `MAX_DCI_TAG_LENGTH = 64` (`nxcldefs.h:37`); the qualifier is free
  text for feed/leg disambiguation where a single device meters both feeds (§3.2 makes this the
  exception, not the rule).
- `relatedObject` = object id of the Facility / PowerDomain / CoolingZone / Rack the series
  describes. `m_relatedObject` (`nms_dcoll.h:393`) is an established, instance-discovery-aware
  back-reference (`dctarget.cpp:2951-2972` re-syncs it on discovery polls) — meter DCIs created by
  instance discovery inherit bindings correctly with zero new machinery.

Any existing deployment becomes Green DC-aware by tagging what it already collects (KPI doc §5.1)
— both fields are ordinary DCI properties settable through `CMD_MODIFY_NODE_DCI`, WebAPI DCI
update, and template import.

The `systemTag` UI combo (`OtherOptions.java:43`) currently hard-codes the seven interface tags;
it grows a Green DC section populated from the slot catalog (§5.2). Server-side, `getSystemTag()`
is lock-free (`nms_dcoll.h:478`) which suits the resolver's read-mostly access pattern.

### 5.2 Slot catalog

The catalog is a **compile-time table in server core** (`gdc_slots.cpp`), not a DB-configurable
schema. Rationale: audit-grade semantics require a frozen vocabulary versioned with the method
(KPI doc §2); the asset-management precedent (`am_attributes`) is admin-editable by design, which
is exactly what a regulatory taxonomy must not be. The catalog is exported to clients via a new
read command (§7.1) so UIs never hard-code it.

Each entry:

```cpp
struct GdcSlotDefinition
{
   const wchar_t *tag;              // "greendc.eed.eit"
   GdcQuantityKind quantityKind;    // ENERGY | POWER | VOLUME
   const wchar_t *canonicalUnit;    // "kWh", "W", "m3"
   uint32_t bindableClasses;        // bitmask over OBJECT_FACILITY/POWERDOMAIN/COOLINGZONE/RACK
   GdcSlotCardinality cardinality;  // SINGLE | SUMMING
   const wchar_t *annexIIRef;       // EED Annex II data point reference
   bool firstClass;                 // v1 first-class vs. taxonomy-listed-deferred (OQ-2)
};
```

Proposed v1 first-class set (input to OQ-2 — the minimum closing PUE, WUE, ERF, REF and the CUE
energy denominator):

| Slot | Kind | Unit | Bindable at | Cardinality |
|---|---|---|---|---|
| `greendc.eed.edc` — total DC energy at utility boundary | ENERGY | kWh | Facility; PowerDomain(GRID_ENTRY) | SUMMING |
| `greendc.eed.eit` — IT equipment energy | ENERGY | kWh | Facility; PowerDomain; Rack | SUMMING |
| `greendc.eed.win` — water intake | VOLUME | m³ | Facility | SUMMING |
| `greendc.eed.reuse` — energy reused outside boundary | ENERGY | kWh | Facility; CoolingZone | SUMMING |
| `greendc.eed.ren` — renewable energy | ENERGY | kWh | Facility | SUMMING |
| `greendc.power.feed` — instantaneous feed power | POWER | W | PowerDomain; Rack | SUMMING |
| `greendc.cool.energy` — cooling plant energy | ENERGY | kWh | Facility; CoolingZone | SUMMING |

Taxonomy-listed but deferred (full Annex II enumeration ships in the catalog with
`firstClass=false`): backup-generator fuel, battery capacity, floor area, rated IT capacity,
temperature set points, `greendc.grid.ci` (carbon intensity — acquisition already exists via the
`entsoe` subagent; the CUE computation strand binds it later per its own document).

The engine integrates POWER slots to energy explicitly (trapezoidal over the sample grid); a slot's
quantity kind plus the DCI's `deltaCalculation` (`nxcldefs.h:1203-1206`) decide counter vs. gauge
handling — a counter and an instantaneous reading are never interchangeable (KPI doc §5.2).

### 5.3 Binding-time validation

Modeled on `ValidateAssetPropertyValue` (`asset_management.cpp:927-1100`): a
`ValidateGreenDCBinding(dci, tag, relatedObject)` returning `(RCC, reason)` — called from the DCI
modification paths (`ClientSession::modifyNodeDCI`, the WebAPI DCI update, template apply)
whenever the new `systemTag` starts with `greendc.`. Checks, all at configuration time (KPI doc
§5.5):

1. **Vocabulary** — slot exists in the catalog (unknown → `RCC_UNKNOWN_ATTRIBUTE`-style error
   with reason).
2. **Level legality** — `relatedObject` set, resolves, and its class is in `bindableClasses`.
3. **Unit compatibility** — DCI's configured unit (`m_unitName`) convertible to the canonical
   unit; the conversion factor is fixed at binding time.
4. **Cardinality** — for SINGLE slots, no other DCI bound to the same (slot, object).
5. **Quantity-kind sanity** — POWER slots on gauge-style DCIs, ENERGY slots on
   counter/gauge-with-delta DCIs.

Rejected bindings never enter the resolver's view; there is no "discovered at computation time"
failure mode for configuration errors.

### 5.4 Binding resolution

`ResolveFacilityBindings(facility)` composes existing primitives — the same shape as the AI tools'
subtree tag scan (`aitools/operational.cpp:611-636`):

1. Collect DC targets in the facility subtree: `facility->addChildDCTargetsToList(…)`
   (`netobj.cpp:2772`) — this naturally crosses PowerDomain/CoolingZone/Rack membership edges,
   and multi-parent dedup is already built in.
2. Per target: `getDCObjectsByFilter` (`dcowner.cpp:1270`) with prefix match on
   `getSystemTag()`.
3. Attach each hit to its binding point via `getRelatedObject()`, verify the binding point is
   still inside this facility's subtree (`isParent` check, `operational.cpp:138-157` idiom).

Resolution runs per computation window and is cached on the Facility between configuration
changes. Matching is exact on the slot segment (no glob) — consistent semantics, unlike today's
four different tag-matching flavors (noted in the research survey).

### 5.5 Metering profile

Per facility, the declared expectation against which completeness is computed (KPI doc §5.3/5.4):

```
gdc_metering_profile
   facility_id       integer       not null
   slot              varchar(63)   not null
   state             char(1)       not null   -- 'B' bound / 'P' proxied / 'A' absent
   expected_bindings integer       not null   -- expected binding-set size for SUMMING slots
   comments          varchar(255)  null
   PRIMARY KEY(facility_id, slot)
```

The profile enumerates **all** catalog slots per facility (rows created on Facility creation,
default `A`bsent); "we meter 70 % of IT load" becomes `expected_bindings` vs. resolved-bindings —
a first-class quantified statement. The profile drives provider selection (§6.2) and is snapshotted
into every annual freeze (§6.6).

**OQ-1 reservation:** if the estimator requires meter-coverage topology, it arrives as a v0.2
extension table `gdc_meter_coverage(facility_id, dci_id, covers_object_id, share)` — a coverage
edge list over the same object ids, populated per facility. The provider request struct (§6.2)
carries an optional pointer to it from day one so the contract does not change shape later.

---

## 6. KPI engine

### 6.1 Placement

Open-tier engine lives in **server core** (GPL): new files `src/server/core/gdc_objects.cpp`,
`gdc_slots.cpp`, `gdc_engine.cpp`, `gdc_snapshot.cpp`, public header
`src/server/include/greendc.h` (exported types + registration API, `NXCORE_EXPORTABLE`). A server
module was considered and rejected for the open tier: the engine owns new object classes, core
schema, and session commands — the module escape hatches (`pfCreateObject`,
`pfIsValidParentClass`, `OBJECT_CUSTOM` ids) exist but would make the open tier a second-class
citizen in every UI and API surface. Modules are the right vehicle for the *commercial provider*,
which owns none of those things.

### 6.2 Provider contract — the open/commercial seam

Follows the named-registry connector pattern (`cloud_connector.cpp:68-96`), registered from a
module's `pfInitialize` like `RegisterHelpDeskLink` (`hdlink.cpp:146-159`):

```cpp
// src/server/include/greendc.h
struct GdcComputationRequest
{
   uint32_t facilityId;                         // identity only — no object access (R6)
   time_t dayStartUtc;                          // computation window
   int32_t timezoneOffset;                      // facility-local day boundary
   StructArray<GdcSlotSeries> slotSeries;       // resolved bindings, samples with attributes
   StructArray<GdcMeteringProfileEntry> profile;
   const GdcCoverageGraph *coverageGraph;       // nullptr until OQ-1 lands
};

struct GdcComponentValue
{
   wchar_t component[16];                       // 'EIT', 'EDC', 'WIN', 'REUSE', 'REN', ...
   double value;
   SampleAttributes attributes;                 // core type (§4.2): quality, bounds, completeness, method id+ver
};

class NXCORE_EXPORTABLE GreenDCComputationProvider
{
public:
   virtual const wchar_t *getName() const = 0;              // 'DIRECT' reserved for built-in
   virtual uint32_t getMethodId() const = 0;
   virtual uint32_t getMethodVersion() const = 0;
   virtual bool computeDailyComponents(const GdcComputationRequest& request,
         StructArray<GdcComponentValue> *output) = 0;
};

bool NXCORE_EXPORTABLE RegisterGreenDCComputationProvider(GreenDCComputationProvider *provider);
```

- **R6 (one-way dependency)** is mechanical: the request is plain data — no `NetObj`, no config
  reads, no session. Core never frees provider objects (`Ownership::False` registry convention,
  `cloud_connector.cpp:31`).
- **Selection** is per facility: `gdc_facilities.provider` names the provider; default `DIRECT`.
  Metering-profile-driven auto-selection (KPI doc §3.2) is a UI/assistant concern layered on top —
  the stored choice stays explicit and auditable.
- **Direct-computation provider (open, built-in):** registered by core itself under `DIRECT`.
  Emits components only where the slot's resolved bindings are complete and `MEASURED`; on gaps it
  emits the component as `MISSING`/degraded — the honest boundary of the open tier (KPI doc §3.2).
  Never estimates.
- **Estimation provider (commercial):** ships as a `.nxm` module; registers under its own name;
  owns its own tables through the nxdbmgr module-extension entry points (`NXM_UpgradeDB` /
  `NXM_GetSchemaPrefix` etc., `nxdbmgr/modules.cpp:95-100` — supported today, unused in open
  tree); enforces its own licensing via `RegisterLicenseProblem` (`main.cpp:325-335`) and announces
  itself with `RegisterComponent(L"GREENDC-EST")` (`main.cpp:286`) so UIs can gate estimation
  configuration (`NXCSession.isServerComponentRegistered`, `ObjectsPerspective.java:299-304`
  pattern).

**R4 (determinism)** is a provider obligation stated in the contract header and testable: the
engine logs an input digest (hash over slot series + profile + method id/version) with every
record write, so recomputation drift is detectable, not just forbidden.

### 6.3 Computation flow and scheduling

Registered exactly like the aggregation rollup (`main.cpp:1725,1755`):

```
RegisterSchedulerTaskHandler(L"GreenDC.DailyComputation", GdcDailyComputation, 0);
AddUniqueRecurrentScheduledTask(L"GreenDC.DailyComputation", L"45 0 * * *", ...);
```

Per cycle, for each Facility (iteration via `g_idxObjectById.getObjects` filtered on class — the
`CollectAllDCTargets` idiom, `dcagg.cpp:508-520`):

1. Determine the recompute horizon: every facility-local day from the oldest `PROVISIONAL` record
   (bounded by `GreenDC.MaxRecomputeDays`, default 45) through yesterday.
2. For each day: resolve bindings (§5.4), assemble input series from `idata` +
   `dci_sample_attributes` (LEFT JOIN), integrate POWER slots, build the request.
3. Invoke the facility's provider. Refusals/partial output become `MISSING`/degraded component
   rows — never absent rows, so the day is always accounted for (R5).
4. Upsert daily records (`ON CONFLICT`/`MERGE`, `BuildAggregateUpsert` pattern) with
   `state='P'` (PROVISIONAL). Pre-settlement overwrite-in-place is safe under R4 (KPI doc §6.2).
5. Days older than the facility's settlement lag transition `P`→`S` (SETTLED); an
   `EVENT_GDC_DAY_SETTLED` fires per transition. Settled rows are never overwritten by the
   scheduled path.
6. `IsShutdownInProgress()` checks on both loop levels; `ThrottleHousekeeper`-style backpressure
   is unnecessary (row volume is days × components), but the run logs
   `facilities processed / days recomputed / duration` at debug level 4 (`dcagg.cpp:681` style).

Settlement lag default **5 days** (`GreenDC.DefaultSettlementLag`, per-facility override in
`gdc_facilities`). The DCI-aggregation close window (1800 s) is the wrong scale here — utility
corrections and late meter reads arrive on day scale (OQ-3 input). Estimator-triggered
re-settlement (OQ-3's second half) is supported as an explicit, audited operation:
`RecomputeGreenDCDay(facilityId, day, force)` exported for console command / REST use; forcing
recomputation of a SETTLED day requires the new `SYSTEM_ACCESS_GDC_MANAGE` right and writes an
audit record — it does not silently change history, it produces a visible correction.

### 6.4 Daily component records — store components, never ratios

```
gdc_daily_components
   facility_id     integer      not null
   record_date     integer      not null   -- facility-local day, YYYYMMDD
   component       varchar(15)  not null   -- EIT / EDC / WIN / REUSE / REN / ...
   value           SQL_DOUBLE   null       -- null when quality = MISSING
   quality         integer      not null
   bound_low       SQL_DOUBLE   null
   bound_high      SQL_DOUBLE   null
   completeness    SQL_DOUBLE   null
   method_id       integer      not null
   method_version  integer      not null
   input_digest    varchar(63)  null       -- R4 drift detection
   state           char(1)      not null   -- 'P' provisional / 'S' settled
   computed_at     integer      not null
   PRIMARY KEY(facility_id, record_date, component)
```

All downstream consumption — window derivation, dashboards, snapshots, REST — reads this table,
never the provider (KPI doc §3.1 step 5). No retention: daily component records are the regulatory
substrate and are kept for the life of the facility (deleted only with the Facility object,
`deleteFromDatabase`).

### 6.5 Window derivation

`GdcDeriveKpi(facility, kpi, from, to)` — one code path for every window (day, month, rolling-12,
EED reporting year):

1. Sum each component over the window; window completeness = expected-day-count weighted mean;
   window quality class = worst contributing class; `MISSING` days count against completeness and
   force the window class to at most `ESTIMATED` — never skipped silently.
2. Component bound combination: **linear (correlated) interval sum** — conservative, method-neutral
   with respect to GUM/Monte Carlo/interval methods (KPI doc §2.1 chose bounds for exactly this
   neutrality). Quotient bounds for ratios: interval division of the summed component intervals,
   at derivation time, never averaged (KPI doc §6.1).
3. KPI set: PUE = EDC/EIT, WUE = WIN/EIT, ERF = REUSE/EDC, REF = min(REN/EDC, 1), CUE deferred to
   the carbon strand.

Daily ratios are never stored — mean-of-daily-PUE is not monthly PUE; the single stored
representation is the component record (KPI doc §6.1, unchanged).

### 6.6 Annual frozen snapshots

```
gdc_annual_snapshots
   id              integer      not null   -- IDG_GREENDC_SNAPSHOT
   facility_id     integer      not null
   reporting_year  integer      not null
   version         integer      not null   -- 1..n per (facility, year)
   supersedes      integer      null       -- previous snapshot id in the chain
   frozen_at       integer      not null
   frozen_by       integer      not null   -- user id
   content         SQL_TEXT     not null   -- JSON provenance bundle
   PRIMARY KEY(id)
   UNIQUE(facility_id, reporting_year, version)
```

`content` is the complete, self-contained provenance bundle (KPI doc §6.3): derived annual KPI
values with bounds and coverage levels; component totals; method ids/versions used per day-range;
per-component completeness; metering-profile state at freeze; day-level quality-class distribution
(365-entry class vector); input digests. Self-containment is deliberate — the snapshot must answer
"what did we report, from what inputs" even if daily records are later corrected.

Rows are **insert-only at the storage layer's honor and the server's enforcement**: no NXCP/REST
mutation path exists; a post-freeze correction creates version n+1 with `supersedes` set, requires
`SYSTEM_ACCESS_GDC_MANAGE`, and writes an audit record (`writeAuditLogWithValues` convention).
Freeze is triggered by a scheduled year-close task (facility reporting-year anchor + settlement
lag) and manually via command. `EVENT_GDC_SNAPSHOT_FROZEN` / `EVENT_GDC_SNAPSHOT_SUPERSEDED` fire
accordingly.

### 6.7 Facility internal metrics and mirror DCIs

Two consumption tiers, both reading `gdc_daily_components`:

1. **Internal metrics** (free with `DataCollectionTarget`): `Facility::getInternalMetric`
   (`dctarget.cpp:1469` override pattern) serves `GreenDC.PUE(rolling12)`,
   `GreenDC.Component(EIT,month)` etc. as point estimates — instantly usable in ordinary DCIs,
   thresholds, dashboards, with zero new client code (R1 semantics by construction).
2. **Mirror DCIs** (optional, per facility flag): the engine materializes daily component series as
   `DS_COMPUTED` DCIs on the Facility via `WriteAttributedSample` (§4.3–4.4), giving
   attribute-aware history charts and NXSL access through the standard surfaces.

---

## 7. Query surfaces

### 7.1 NXCP commands

Next free code is `0x0225` (`nms_cscp.h:748`); allocate a contiguous block, register in
`ClientSession::processRequest` switch, `NXCPMessageCodeName()` (`libnetxms/nxcp.cpp`), and
`NXCPCodes.java` per `CLAUDE.md` rules; reuse existing `VID_*` where semantics match:

| Command | Code | Purpose |
|---|---|---|
| `CMD_GET_GDC_SLOT_CATALOG` | `0x0225` | slot taxonomy for UIs (tag, kind, unit, levels, cardinality, first-class flag) |
| `CMD_GET_GDC_COMPONENTS` | `0x0226` | daily component records for a facility + window |
| `CMD_GET_GDC_KPI` | `0x0227` | derived KPI values (window param), with bounds/completeness/quality |
| `CMD_GET_GDC_METERING_PROFILE` | `0x0228` | profile rows + live resolved-binding counts (gap map) |
| `CMD_UPDATE_GDC_METERING_PROFILE` | `0x0229` | set state / expected counts |
| `CMD_GET_GDC_SNAPSHOTS` | `0x022A` | snapshot list + content |
| `CMD_FREEZE_GDC_SNAPSHOT` | `0x022B` | manual freeze / supersede |
| `CMD_RECOMPUTE_GDC_DAY` | `0x022C` | audited re-settlement trigger (§6.3) |

New access right `SYSTEM_ACCESS_GDC_MANAGE` guards `0x0229`–`0x022C`; reads require
`OBJECT_ACCESS_READ` on the Facility.

### 7.2 REST (webapi module)

Modeled on the history endpoint (`webapi/datacoll.cpp:75`) and registered via `RouteBuilder`
(`webapi/main.cpp` `InitModule`); `openapi.yaml` updated in the same change:

```
GET  v1/greendc/slot-catalog
GET  v1/objects/:object-id/greendc/components?from=&to=
GET  v1/objects/:object-id/greendc/kpi?window=day|month|rolling12|year&at=
GET  v1/objects/:object-id/greendc/metering-profile
PUT  v1/objects/:object-id/greendc/metering-profile
GET  v1/objects/:object-id/greendc/snapshots[?year=]
POST v1/objects/:object-id/greendc/snapshots        (freeze)
POST v1/objects/:object-id/greendc/recompute        (audited)
```

JSON rows carry the full attribute set; this is the primary integration surface for Grafana
(`v1/grafana/*` precedent shows metadata endpoints; KPI history goes through the generic
endpoints above).

### 7.3 NXSL

Registered via `RegisterDCIFunctions`-style table in `gdc_engine.cpp`:

- `GetFacilityKpi(facility, kpi, from, to)` → object with `value`, `lowerBound`, `upperBound`,
  `completeness`, `quality` (read-only class, `DCObjectInfo`-snapshot style).
- `GetFacilityComponents(facility, from, to)` → array of component record objects.
- NXSL classes `Facility` / `PowerDomain` / `CoolingZone` expose `domainType`, `feedTag`,
  `equipment` (resolved object), `provider`, `settlementLag`.
- Object-query constants `FACILITY`, `POWERDOMAIN`, `COOLINGZONE` (`object_queries.cpp:~549`).

---

## 8. Events, configuration, observability

**Events** (next free codes in the 0–499 system range, `doc/internal/event_code_ranges.txt`;
templates via `CreateEventTemplate` in the upgrade proc):

| Event | Fires on |
|---|---|
| `SYS_GDC_DATA_GAP` | expected binding produced no samples for a computation day (per slot, on the Facility) |
| `SYS_GDC_DAY_SETTLED` | P→S transition |
| `SYS_GDC_COMPONENT_DEGRADED` | settled component quality below MEASURED (parameters: component, quality, completeness) |
| `SYS_GDC_SNAPSHOT_FROZEN` / `SYS_GDC_SNAPSHOT_SUPERSEDED` | §6.6 |
| `SYS_GDC_BINDING_INVALID` | a previously valid binding fails re-validation (object deleted, unit changed) |

**Config parameters** (via `CreateConfigParam` + `sql/setup.in` row):
`GreenDC.DefaultSettlementLag` (5 days), `GreenDC.MaxRecomputeDays` (45),
`GreenDC.DefaultCoverageLevel` (95).

**Debug tags** (added to `doc/internal/debug_tags.txt`): `gdc.engine`, `gdc.binding`,
`gdc.snapshot`.

**Audit:** profile updates, forced recomputation, freeze/supersede — all through the standard
audit log with old/new values.

---

## 9. Database schema summary and upgrade

New tables (all in `sql/schema.in`; `nxdbmgr` table list regenerates automatically):

| Table | Section | Notes |
|---|---|---|
| `gdc_facilities` | §3.4 | per-object row, class table pattern (`racks` precedent) |
| `gdc_power_domains` | §3.4 | |
| `gdc_cooling_zones` | §3.4 | |
| `gdc_metering_profile` | §5.5 | |
| `dci_sample_attributes` | §4.3 | **core table**, not GreenDC-owned; TSDB hypertable on TSDB builds; retention tied to DCI history |
| `computation_methods` | §4.5 | **core table**; methods namespaced by owning engine |
| `gdc_daily_components` | §6.4 | no retention (regulatory substrate) |
| `gdc_annual_snapshots` | §6.6 | insert-only |
| `gdc_meter_coverage` | §5.5 | reserved, lands with OQ-1 resolution (v0.2) |

Upgrade: one procedure at the head of the current major version's upgrade file
(`upgrade_v70.cpp` `H_UpgradeFromV12` shape: `CreateTable` × n, `CreateConfigParam` × 3,
`CreateEventTemplate` × 6, `SetMinorSchemaVersion`), new entry atop `s_dbUpgradeMap[]`, version
bump in `include/netxmsdb.h`. New ID generator `IDG_GREENDC_SNAPSHOT = 40` (`nms_core.h:85`
follows `IDG_NETCONF_QUERY = 39`). Objects requiring a `MODIFY_*` persistence bit reuse the
class-specific alias space at `0x80000000` (`nms_objects.h:940-943`) for their class tables.

---

## 10. Console (nxmc) touch points

UI design is deferred (KPI doc scope), but the class plumbing in §3.5 is not optional — without
it the console cannot render the objects at all. Beyond that plumbing, the anticipated (not yet
designed) UI surface, listed so the schema/API above is checked against it:

- Facility summary view (KPI tiles from `CMD_GET_GDC_KPI`, gap map from the metering profile,
  quality-class calendar from component records).
- Metering-profile editor (slot catalog from `CMD_GET_GDC_SLOT_CATALOG`).
- Snapshot browser with supersede-chain display.
- DCI "Interpretation" combo (`OtherOptions.java:43`) extended with the Green DC slot section;
  binding validation errors surfaced from the server RCC + reason.
- Uncertainty bands on history charts via the existing `LineChart` min/max band machinery
  (`LineChart.java:926-980`) — swt and rwt variants both.
- Estimation-provider configuration gated on `isServerComponentRegistered("GREENDC-EST")`.

---

## 11. Feedback to the KPI data model v0.1

Corrections and inputs for the next revision, per its §8 review instructions:

1. **§4.1 Rack containment (normative correction).** "One electrical parent edge" contradicts
   §4.2's normative dual feeds. Replace with "one or more electrical parent edges (one per feed)
   and one thermal parent edge." The electrical overlay is an explicit PowerDomain hierarchy
   (grid entry → UPS → PDU); racks attach to the lowest metered domain of each feed. Feed identity
   moves primarily to the PowerDomain (`feedTag`); DCI tag qualifiers remain for per-feed metering
   bound at rack level. R8 unchanged.
2. **§4.1 "containment is already a DAG" (nuance).** NetXMS supports multi-parent membership
   natively, but cycle prevention exists only on the client bind path (`RCC_OBJECT_LOOP` check).
   Adequate for this design; worth stating precisely in the data model.
3. **§2 "new first-class DCI value type" (implementation strategy).** Realized as
   point-estimate-in-pipeline + companion attribute storage + opt-in wire extension, not a new
   wire/data type — R1/R2/R3 become structural guarantees instead of conventions (§4.1 above).
   Furthermore, the attribute vocabulary is engine-neutral, so it ships as core DCI
   infrastructure with the GreenDC engine as its first consumer (§4). No change to the contract,
   only to its realization.
4. **OQ-1** — the provider request struct carries an optional coverage-graph pointer from day one
   and the `gdc_meter_coverage` table is reserved; confirming "yes" costs a table + populate flow,
   not a contract change.
5. **OQ-2** — proposed v1 first-class slot set in §5.2 (PUE/WUE/ERF/REF inputs + feed power +
   cooling energy); everything else taxonomy-listed with `firstClass=false`.
6. **OQ-3** — settlement default 5 days, per-facility override; time-driven settlement with
   estimator/operator-triggered recomputation as an explicit, audited operation rather than an
   autonomous provider capability (keeps R6 one-way).
7. **OQ-4** — coverage level per method version, declared in the method registry and frozen into
   snapshots; system default 95 %.
8. **OQ-5** — the metering profile plus binding validation make metering density measurable per
   facility before pilot commitment (resolved-bindings vs. expected per slot); calibration-record
   availability remains an organizational prerequisite outside this design.

---

## 12. Implementation phasing

| Phase | Content | Depends on |
|---|---|---|
| **1. Hierarchy** | Classes 42–44, schema, containment matrix, Java/nxmc plumbing (§3.5), NXSL classes | — |
| **2. Binding** | Slot catalog, validation, resolver, metering profile, `CMD_GET_GDC_SLOT_CATALOG` / profile commands | 1 |
| **3. Engine (open tier)** | Method registry, DIRECT provider, daily computation task, component records, settlement, KPI derivation, internal metrics, events | 2 |
| **4. Query surfaces** | Remaining NXCP commands, REST endpoints, NXSL functions | 3 |
| **5. Attributes on the wire** | `dci_sample_attributes`, `WriteAttributedSample`, `DS_COMPUTED` origin, mirror DCIs, wire option bit, Java/REST/NXSL row attributes | 3 (parallel to 4) |
| **6. Snapshots** | Freeze task, supersede chain, snapshot commands/REST | 3 |
| **7. Commercial seam hardening** | Provider registration soak test with a stub `.nxm`, nxdbmgr module-schema walkthrough, component registration | 3 |

Phases 1–4 constitute a complete open-tier vertical slice (tag what you collect → daily records →
PUE over any window) and map to the EI-7 KPI engine milestone; 5–7 complete the audit and
commercial contracts.

---

## 13. Decision log

| Decision | Alternatives rejected | Why |
|---|---|---|
| Attributes beside the pipeline, not a new `DCI_DT` | new data type; widened idata columns; attributes encoded in value string; third `DCObject` variation | provenance is per-sample — a mixed measured/backfilled series is the normative case and no series-level type can express it; idata is the hottest schema in the product (per-object tables × N + 6 TSDB hypertables); string encoding breaks DB-level readers; a third DCO type forks the entire item/table machinery (client factory, editors, storage, thresholds, PDS) for values that are still scalars. Client-version compatibility is *not* the driver — release policy couples client and server at major.minor |
| Sample-attribute layer is core infrastructure, not GreenDC-private | `gdc_`-prefixed types and tables | vocabulary (quality, bounds, completeness, method identity) is engine-neutral; second consumers already visible in-tree (prediction-engine seam, per-sample anomaly marking, cluster-aggregation completeness); avoids a rename migration later |
| `DS_COMPUTED` data origin for engine-written series | third `DCObject` type; convention-only R3 | data origins already gate collection behavior (push origins are not polled); enforces producer exclusivity at the source with zero new branches in storage, wire, thresholds or client parsing |
| No generic computation-engine framework | speculative shared skeleton for future engines | single engine today; control planes differ per engine in record shape and lifecycle; extract shared code when a second engine exists |
| Compile-time slot catalog | asset-schema-style DB catalog | regulatory vocabulary must version with code/method, not with admin edits; catalog still exported to clients via command |
| `systemTag` + `relatedObject` binding | new binding table; userTag | reuses interpretation-tag machinery verbatim (`iface-*` precedent incl. instance-discovery re-sync); `userTag` stays free for operators |
| Binding at PDU/UPS domain objects for feed identity | feed encoded only in tags (KPI doc R7) | user-confirmed topology need (main entries → UPS → PDU); objects give rollup boundaries and equipment linkage; tags alone can't model lineage |
| Core engine + module providers | whole feature as module | engine owns object classes, schema, session commands — module escape hatches would demote the open tier; connectors/hdlink precedent covers the provider seam exactly |
| Daily records in dedicated table, KPIs derived | KPIs as stored DCIs; ratios stored | ratios don't aggregate; idata can't carry the attribute set; regulatory no-retention requirement conflicts with DCI retention machinery |
| Facility-local day + `YYYYMMDD` key | UTC epoch day | EED reporting is calendar-local; avoids DST double-count/gap at day boundaries |
| Settlement 5 days, P→S state column | aggregation-style close window (seconds); open-interval sentinel | corrections arrive on day scale; explicit state beats sentinel for audit queries (`business_service_downtime` sentinel considered and rejected — records here are per-day, not intervals) |
