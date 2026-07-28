# Green DC Monitor — Data Model Specification (Scientific Team Edition)

**Status:** draft for review
**Project:** NetXMS Green DC Monitor · 1.2.1.2.i.2/1/24/A/CFLA/006
**Audience:** Scientific Lead and estimation-methodology team (LBTU D1 / Manuscript 1)
**Relationship to other documents:**
- Supersedes the structural sections (§§2–6) of *KPI Data Model & Conceptual Architecture v0.1*,
  incorporating the review corrections agreed since (notably the dual-feed power topology).
- The engineering counterpart is
  [`GreenDC_KPI_Engine_Design.md`](GreenDC_KPI_Engine_Design.md), which maps every construct here
  onto NetXMS internals. This document deliberately contains no implementation detail; where a
  reader wants the mechanism, Appendix B gives the section mapping.

**How to read this document:** everything stated here is a *contract*. The structures are frozen
before the estimation method exists and are designed so that no choice below pre-empts the D1
methodology. Items the scientific team must still decide are marked **[SD-n]** and collected in §9.

---

## 1. Overview

The system observes a data-center facility through **metered series** bound to a **spatial
model**, computes **daily energy-component records** through a pluggable **computation provider**,
and derives **KPIs** (PUE, WUE, ERF, REF; CUE later) over arbitrary windows from those records.
Every value carries **per-sample provenance**; reported annual figures are **frozen immutably**
with full provenance and corrected only by versioned supersession.

Five layers, each specified in its own section:

| Layer | Contents | Section |
|---|---|---|
| Spatial model | Facility, PowerDomain, CoolingZone, Rack; two overlay hierarchies | §2 |
| Measurement model | slots, series, samples, bindings, quantity kinds | §3 |
| Provenance model | quality classes, uncertainty bounds, completeness, method identity | §4 |
| Computation model | the provider contract — the estimator's entire world | §5 |
| Temporal & audit model | daily records, settlement, window derivation, annual snapshots | §6–7 |

---

## 2. Spatial model

### 2.1 Objects and hierarchies

The facility is modeled as a directed acyclic graph of typed objects. Containment expresses
*membership only* — never energy flow (invariant **I-5**, §2.4).

- **Facility** — the root object and the EED reporting boundary. One Facility ⇔ one Annex II
  submission. All records, profiles and snapshots are keyed by facility.
- **PowerDomain** — a segment of the electrical distribution topology. Domains nest to express
  lineage: grid entry → UPS → PDU. Each domain has:
  - a *type* ∈ {GRID_ENTRY, GENERATOR, UPS, PDU, BUSWAY, OTHER};
  - a *feed tag* (free label, conventionally "A"/"B") identifying the feed lineage it belongs to;
  - a *rated power* (nameplate, W; 0 = undeclared) used only for plausibility checks.
- **CoolingZone** — a segment of the thermal topology. Zones nest to express plant → zone
  structure. Each zone has a *type* ∈ {PLANT, ZONE, OTHER} and a *rated capacity* (W thermal).
- **Rack** — the shared leaf. A rack has **one or more electrical parent edges — one per feed —
  and one thermal parent edge**, plus its ordinary organizational parent(s).

> **Correction to KPI doc v0.1 §4.1.** The earlier text read "one electrical parent edge"; dual
> A/B feeds are the normative case (v0.1 §4.2), so a rack attaches to the *lowest metered
> PowerDomain of each feed* — typically two PDU-level domains from different UPS lineages.

Equipment devices (UPS units, PDUs, chillers, CRAC/CRAH units, meters) are ordinary monitored
nodes attached as *members* of the domain or zone they serve. Cardinality is unconstrained: a
zone cooled by two CRAC units has two equipment members; an N+1 parallel UPS system is several
equipment members of one logical UPS domain. Equipment membership serves navigation and
diagnostics only — it carries no flow semantics.

### 2.2 Example

```
Facility F
 ├── PowerDomain "Entry-1"  (GRID_ENTRY)
 │    └── PowerDomain "UPS-A"  (UPS, feed A)     [members: UPS nodes A1, A2]
 │         ├── PowerDomain "PDU-A1" (PDU, feed A)
 │         │     ├── Rack R101      ◄── electrical edge, feed A
 │         │     └── Rack R102
 │         └── PowerDomain "PDU-A2" (PDU, feed A)
 ├── PowerDomain "Entry-2"  (GRID_ENTRY)
 │    └── PowerDomain "UPS-B"  (UPS, feed B)
 │         └── PowerDomain "PDU-B1" (PDU, feed B)
 │               ├── Rack R101      ◄── electrical edge, feed B (second parent)
 │               └── Rack R102
 └── CoolingZone "Plant"   (PLANT)               [members: chiller nodes]
      └── CoolingZone "Room-1" (ZONE)            [members: CRAC-1, CRAC-2]
            ├── Rack R101          ◄── thermal edge (third parent)
            └── Rack R102
```

### 2.3 The flow graph is not the object graph

Two distinct graphs exist and must never be conflated:

1. **The object graph** (above): membership, used for grouping, rollup boundaries, navigation,
   and binding-level validation.
2. **The flow graph**: implied by the set of metered series and their binding points (§3).
   Energy-balance closure, conservation assertions, and the estimator's reasoning operate on the
   flow graph exclusively.

Summing object-graph edges across a dual-feed rack double-counts its draw; therefore object
membership is never summed (I-5).

### 2.4 Spatial invariants

- **I-1** One Facility per reporting boundary; every record/profile/snapshot is facility-scoped.
- **I-2** PowerDomain and CoolingZone hierarchies are independent overlays; neither is
  authoritative over the other.
- **I-3** A rack has exactly one thermal parent and ≥ 1 electrical parents (one per feed).
- **I-4** Feed identity is a property of the PowerDomain (feed tag); a series inherits its feed
  from its binding point. Tag qualifiers (§3.3) cover the exceptional per-feed meter bound at
  rack level.
- **I-5** Membership edges carry no energy semantics; conservation applies to flows, not objects.

---

## 3. Measurement model

### 3.1 Series and samples

A **series** is an ordered set of timestamped samples produced by one meter/sensor channel (or by
a computation, §4.3):

- sample: *(t, v)* with *t* a UTC timestamp (millisecond resolution) and *v* a numeric point
  estimate;
- optionally, a sample carries an **attribute tuple** *A = (q, [l, u], c, m)* — quality class,
  bounds, completeness, method identity (§4);
- a sample that was expected but not produced is **absent** — the data model never stores
  placeholder zeros or carried-forward values (**I-6**, the no-silent-substitution invariant).

Each series has a declared collection interval; the *expected sample grid* of a series over a
window is derived from that interval and is the denominator for gap statistics.

### 3.2 Slots

The **slot taxonomy** is the controlled vocabulary binding series to EED semantics. It is frozen
per software release (versioned with the method registry, not editable per site). Each slot
declares:

| Property | Meaning |
|---|---|
| identifier | e.g. `greendc.eed.eit` |
| quantity kind | ENERGY (cumulative, integrated) · POWER (instantaneous) · VOLUME (cumulative) |
| canonical unit | kWh · W · m³; unit conversion is fixed at binding time |
| bindable levels | which object classes the slot may bind to (Facility / PowerDomain / CoolingZone / Rack) |
| cardinality | SINGLE (at most one series per object) or SUMMING (many series aggregated) |
| Annex II reference | the EED data point it feeds |
| tier | first-class in v1, or taxonomy-listed-deferred |

Proposed v1 first-class set — the minimum closing PUE, WUE, ERF, REF and the CUE denominator
**[SD-2]**:

| Slot | Kind | Unit | Bindable at | Cardinality |
|---|---|---|---|---|
| `greendc.eed.edc` — total facility energy at utility boundary | ENERGY | kWh | Facility; GRID_ENTRY domains | SUMMING |
| `greendc.eed.eit` — IT equipment energy | ENERGY | kWh | Facility; PowerDomain; Rack | SUMMING |
| `greendc.eed.win` — water intake | VOLUME | m³ | Facility | SUMMING |
| `greendc.eed.reuse` — energy reused outside boundary | ENERGY | kWh | Facility; CoolingZone | SUMMING |
| `greendc.eed.ren` — renewable energy | ENERGY | kWh | Facility | SUMMING |
| `greendc.power.feed` — instantaneous feed power | POWER | W | PowerDomain; Rack | SUMMING |
| `greendc.cool.energy` — cooling plant energy | ENERGY | kWh | Facility; CoolingZone | SUMMING |

The full Annex II data-point list ships in the taxonomy with deferred status, so the per-facility
gap map (§3.5) is complete even where the pilot's metering is not.

ENERGY and POWER are never interchangeable: the engine integrates POWER series to energy
explicitly (trapezoidal over the actual sample grid), and the counter-vs-gauge character of the
source is part of the binding validation.

### 3.3 Bindings

A **binding** attaches a series to *(slot, object)*:

- the slot fixes semantics and canonical unit (conversion factor recorded at binding time);
- the object fixes the position in the spatial model — and thereby the feed, via I-4;
- an optional qualifier suffix (`greendc.power.feed/A`) disambiguates the exceptional cases where
  one device meters several feeds.

Bindings are validated at configuration time (vocabulary, level legality, unit compatibility,
cardinality, quantity-kind sanity vs. the declared rated power where available). Invalid bindings
never become visible to the computation layer; there is no "discovered at computation time"
configuration failure mode.

### 3.4 Metering profile

Per facility, per slot, the declared expectation:

- state ∈ {BOUND, PROXIED, ABSENT};
- *expected binding count* for SUMMING slots — the size of the binding set that *would* exist
  under full metering.

The profile is a first-class quantified statement ("we meter 7 of 10 IT feeds"), maintained by
the operator, snapshotted into every annual freeze, and it drives provider selection (§5.4).

### 3.5 Gap map

For each slot: expected bindings vs. resolved bindings vs. per-window sample presence. The
resulting structure *is* the estimator's problem statement — which flows are observed, which are
partially observed, which are absent. If the D1 method additionally requires explicit
meter-coverage topology (which meter covers which subtree, with what share), the profile gains a
**coverage graph** section: edges *(series → covered object, share)* **[SD-1]**. The provider
contract (§5.2) reserves a field for it from day one, so confirming the requirement changes
content, not shape.

---

## 4. Provenance model

### 4.1 The attribute tuple

Provenance is a property of the **sample**, not of the series. A metered series legitimately
contains estimated samples (backfill during a meter outage) interleaved with measured ones. Each
sample may carry:

| Attribute | Definition |
|---|---|
| **Quality class** *q* | `MEASURED` · `PROXY` · `ESTIMATED` · `INTERPOLATED` · `MISSING` (§4.2) |
| **Bounds** *[l, u]* | lower/upper bound on the true value at the method's declared coverage level. Bounds, not a standard deviation — representation-neutral w.r.t. GUM propagation, Monte Carlo, or interval arithmetic, and free of any symmetric-Gaussian presumption |
| **Completeness** *c* ∈ [0, 1] | share of expected underlying inputs actually present for a computed value (§4.4) |
| **Method identity** *m* | (method id, method version) of the producer, resolvable in the method registry (§7.3) |

Defaults when the tuple is absent: a *present* sample without attributes is `MEASURED`, unbounded
(point value only), completeness 1. An *expected but absent* sample is `MISSING` by derivation —
nothing is stored (I-6).

The attribute set is fixed — deliberately not extensible per deployment — because audit-grade
semantics require a frozen vocabulary. The quality-class enumeration is append-only across
releases (a future `FORECAST` class is anticipated); existing classes never change meaning.

No cross-series correlation metadata is carried on values (settled in review round 0):
correlation handling, if the estimator requires it, lives inside the provider against its own
inputs.

### 4.2 Quality classes — precise semantics

| Class | A sample of this class asserts |
|---|---|
| `MEASURED` | value read from the physical instrument covering this flow, within its calibration |
| `PROXY` | value read from an instrument covering a *different but systematically related* flow, mapped by a declared static relation (e.g. name-plate scaling); the relation's identity is the method id |
| `ESTIMATED` | value produced by an estimation method from other observations; bounds mandatory |
| `INTERPOLATED` | value reconstructed purely from the *same series'* neighboring samples; bounds mandatory |
| `MISSING` | no defensible value exists; the numeric field is void and must not enter arithmetic |

Ordering for degradation logic (worst-of): `MEASURED` ≺ `PROXY` ≺ `ESTIMATED` ≺ `INTERPOLATED` ≺
`MISSING` **[SD-6]** — the scientific team is asked to confirm this ordering, in particular
PROXY vs. ESTIMATED precedence.

### 4.3 Provenance invariants

- **I-6 (no silent substitution)** — missing data is absent or explicitly `MISSING`; never an
  implicit zero, carry-forward, or unlabelled guess. Holds in every layer: acquisition, storage,
  computation, derivation.
- **I-7 (producer exclusivity)** — attribute tuples are written only by the producing engine;
  no downstream consumer (user, script, API client) can edit provenance. Engine-produced series
  additionally reject all other data paths (manual pushes, transformations).
- **I-8 (aggregation blanking)** — generic aggregation/consolidation machinery operates on point
  estimates only and produces outputs *without* attribute tuples. Uncertainty does not average;
  correlated errors do not cancel; a generic consolidation that emitted bounds would produce
  confidently wrong intervals. Only the KPI engine (and future engines of the same discipline)
  writes uncertainty on derived values.
- **I-9 (legacy degradation)** — every consumer unaware of attributes sees exactly the point
  estimate; the attribute tuple is strictly additive information.

### 4.4 Completeness

For a computed value over window *W* from a SUMMING slot with expected binding set *B* (declared
in the metering profile) and per-series expected sample grids:

> *c* = (observed input mass) / (expected input mass under the metering profile)

The precise weighting of "input mass" (per-series sample counts vs. energy-share weights) is a
method decision **[SD-5]**; the container stores a single scalar per computed value regardless of
the weighting chosen, so the choice does not affect the schema.

---

## 5. Computation model — the provider contract

This is the seam where the D1 methodology plugs in. Everything the estimator can see and must
produce is specified here; nothing else about the host system is observable from inside a
provider (one-way dependency, mechanically enforced).

### 5.1 Computation window

The atomic computation unit is the **facility-local calendar day** (facility timezone; DST
handled by local-day boundaries). All coarser windows are derived by summation of daily
components (§6.3) — the provider is never asked for a month or a year.

### 5.2 Provider input

For facility *F* and day *D*:

1. **Resolved slot set** — for every bound slot: the binding metadata (slot, binding object and
   its class, feed lineage, unit conversion, quantity kind, declared collection interval) and the
   sample series over *D* (with a configurable margin for integration edge effects), each sample
   carrying its attribute tuple or the documented defaults.
2. **Metering profile** — the per-slot state and expected binding counts (§3.4).
3. **Coverage graph** — present iff [SD-1] resolves to "required"; otherwise absent.

The input is a value snapshot: no live queries, no configuration access, no clock access beyond
the window definition. Consequence: identical input ⇒ the provider cannot even observe anything
that would let it deviate.

### 5.3 Provider output

The set of **energy-component values** for (F, D). Components in v1: `EIT`, `EDC`, `WIN`,
`REUSE`, `REN`. Each output value is *(v, q, [l, u], c, m)*:

- *m* must be the provider's own registered method identity;
- a component the provider cannot defensibly produce is emitted as `MISSING` (I-6) — refusal is a
  valid, expected output, not an error path;
- bounds are at the coverage level declared by the method (§7.3).

### 5.4 The two providers

| Provider | Tier | Behavior |
|---|---|---|
| **Direct computation** | open | requires full metering per the profile; sums/integrates `MEASURED` inputs; on any gap in a component's input set it degrades that component (`MISSING` or degraded class per its published rules) rather than estimating. Its refusals delimit the open tier honestly. |
| **Estimation** | commercial | accepts partial coverage; produces `ESTIMATED` components with propagated bounds per the D1 methodology. Internals out of scope here; the contract above is its complete external specification. |

Selection is per facility, recorded explicitly, and included in snapshot provenance.

### 5.5 Computation invariants

- **I-10 (determinism)** — identical input snapshot + identical method version ⇒ identical
  output. Recomputation over settled inputs is an audit operation, not a source of drift. The
  engine records an input digest with every computed record, making drift *detectable*, not
  merely forbidden.
- **I-11 (statelessness)** — providers hold no state between invocations that affects output;
  anything the method needs across days must be reconstructible from the input snapshot.
  **[SD-7]** If the D1 method requires cross-day state (e.g. a learned baseline), this invariant
  needs renegotiation *before* the schema freeze — the alternative is widening the input window,
  not adding provider state.

---

## 6. Temporal model

### 6.1 Daily component records

The persistent record is *(facility, day, component)* → *(v, q, [l, u], c, m, state)*. This is
the single stored representation; **ratios are never stored**. A mean of daily PUEs is not the
monthly PUE, and quotient uncertainty must be propagated from component uncertainties at
derivation time, never averaged.

### 6.2 Settlement lifecycle

- A record is written `PROVISIONAL` when computed near real time.
- Every provisional day inside the facility's **settlement lag** (default 5 days **[SD-3]**) is
  recomputed on each engine cycle — late meter reads, utility corrections, and fuller estimator
  inputs are absorbed by overwrite-in-place, which is safe under I-10.
- After the lag, the record transitions to `SETTLED` and is no longer touched by the scheduled
  path.
- Post-settlement correction exists but is an explicit, audited operation (visible in the audit
  trail, never autonomous). **[SD-3]** also asks: should improved estimator-input completeness be
  allowed to *request* such a recomputation, or is settlement strictly time-driven? The container
  supports both; the default is time-driven.

### 6.3 Window derivation

For any window *W* (day, month, rolling-12, EED reporting year), derived on demand:

1. **Component totals:** Σ over days of *v* per component; days with `MISSING` components
   contribute no value but *are counted* in the window's completeness denominator and force the
   window quality to at most `ESTIMATED`.
2. **Window quality:** worst-of over contributing days (ordering per §4.2).
3. **Window completeness:** day-weighted mean of *c*, with `MISSING` days entering as 0.
4. **Window bounds:** linear (perfectly-correlated) interval sum of daily bounds —
   *deliberately conservative placeholder*. **[SD-4]** The scientific team is asked to either
   endorse this or supply the aggregation rule the D1 method requires (e.g. partial independence
   assumptions). The rule is applied at derivation time inside the engine, so changing it never
   touches stored data.
5. **KPIs:** PUE = EDC⁄EIT, WUE = WIN⁄EIT, ERF = REUSE⁄EDC, REF = min(REN⁄EDC, 1). Quotient
   bounds by interval division of the summed component intervals. CUE follows in the carbon
   strand (separate document) using the same component substrate.

One derivation path serves every window — consistency across windows is structural.

---

## 7. Audit model

### 7.1 Annual frozen snapshots

At reporting-year close (facility-local year anchor + settlement lag), a **versioned, immutable
snapshot** per facility is materialized containing:

- derived annual KPI values with bounds and their coverage levels;
- component totals;
- the provenance bundle: method ids/versions in effect per day-range; per-component completeness;
  the metering-profile state at freeze; the day-level quality-class distribution (which days of
  the year were measured/estimated/missing); input digests.

The snapshot is self-contained: it answers "what did we report, from what inputs" even if daily
records are later corrected.

### 7.2 Supersession

A post-freeze correction creates snapshot version *n+1* with an explicit *supersedes* link —
never an edit. The chain answers "what did we report, from what inputs, and what changed"
mechanically. This is the audit artifact behind the EED annual submission.

### 7.3 Method registry

Every method identity *(id, version)* appearing in any stored value resolves in a registry
recording: owning engine, human-readable name, open/commercial tier, and the **declared coverage
level of its bounds** (resolves OQ-4: coverage is per-method-declared, system default 95 %).
Registration is mandatory before a provider may produce values; the registry is what keeps the
supersede chain interpretable years later.

---

## 8. What the scientific team can rely on

Guarantees the containers enforce, independent of any method choice:

1. Point estimates and provenance are inseparable at the sample level, and provenance can never
   be edited after production (I-7).
2. Nothing in the system fabricates data: every gap is visible as a gap at every layer (I-6).
3. Generic infrastructure will never manufacture uncertainty figures (I-8); every bound in the
   system traces to a registered method version.
4. The estimator's input is a complete, closed snapshot (I-10/I-11): reproducing any historical
   computation requires only the archived inputs and the method version.
5. Stored data never encodes a window-aggregation or uncertainty-propagation policy: those are
   applied at derivation time and can be revised by method decision without data migration.
6. The gap map (metering profile + resolved bindings + sample presence) is precise enough to
   serve as the formal problem statement for partial-metering estimation.

---

## 9. Decisions requested from the scientific team [SD-n]

| # | Decision | Maps to | Blocking |
|---|---|---|---|
| **SD-1** | Is meter-coverage topology (coverage graph: series → covered object, share) required beyond the flat metering profile? If yes: required contents. (= OQ-1) | §3.5, §5.2 | profile schema v0.2; pilot data checklist |
| **SD-2** | Confirm the v1 first-class slot set (§3.2) against Manuscript 1 scope and pilot metering. (= OQ-2) | §3.2 | taxonomy freeze |
| **SD-3** | Settlement lag default (proposed 5 days); strictly time-driven settlement vs. estimator-requested recomputation. (= OQ-3) | §6.2 | record-lifecycle implementation |
| **SD-4** | Window bound-aggregation rule: endorse conservative linear interval sum, or specify the method's rule. (Applied at derivation time; no storage impact.) | §6.3 | first cross-window KPI output |
| **SD-5** | Completeness weighting: sample-count-based vs. energy-share-based "input mass". (Scalar container either way.) | §4.4 | estimator gap definition |
| **SD-6** | Confirm quality-class degradation ordering, esp. PROXY ≺ ESTIMATED. | §4.2 | worst-of logic |
| **SD-7** | Does the D1 method need cross-day state? If yes, define the required input-window widening (provider state remains prohibited). | §5.5 | provider contract freeze |

Per the KPI doc's review protocol: structural elements not challenged in this round are treated
as frozen for implementation. OQ-4 (coverage-level convention) is considered resolved per §7.3
unless objected to.

---

## Appendix A — Notation summary

- Sample: *(t, v)* or *(t, v, A)* with *A = (q, [l, u], c, m)*
- Quality classes: `MEASURED` ≺ `PROXY` ≺ `ESTIMATED` ≺ `INTERPOLATED` ≺ `MISSING`
- Completeness *c* ∈ [0, 1]; coverage level: per method version, default 95 %
- Components: EIT, EDC, WIN, REUSE, REN (per facility-local day)
- KPIs: PUE = EDC⁄EIT · WUE = WIN⁄EIT · ERF = REUSE⁄EDC · REF = min(REN⁄EDC, 1)
- Invariants I-1…I-11 as defined in §§2–5

## Appendix B — Mapping to the engineering design

| This document | `GreenDC_KPI_Engine_Design.md` |
|---|---|
| §2 spatial model | §3 (object classes, containment matrix, equipment membership) |
| §3 slots/bindings/profile | §5 (catalog, tag binding, validation, metering profile) |
| §4 provenance model | §4 (core sample-attribute infrastructure) |
| §5 provider contract | §6.2 (C++ interface, provider registration, tiers) |
| §6 temporal model | §6.3–6.5 (scheduling, records, derivation) |
| §7 audit model | §6.6, §4.5 (snapshots, method registry) |
