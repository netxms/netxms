# Green DC Monitor — DCI System Tag Vocabulary

**Status:** proposal for review
**Project:** NetXMS Green DC Monitor · 1.2.1.2.i.2/1/24/A/CFLA/006
**Derived from:** `GreenDC_Data_Model_Scientific.md` (data model specification) and
`GreenDC_KPI_DataModel_v0.1.md` (KPI data model & conceptual architecture)

Slot bindings in the Green DC model are tags on ordinary DCIs, using the existing DCI
system tag property with a controlled vocabulary (KPI doc §5.1). This document
enumerates the concrete tag vocabulary: the EED slot tags corresponding to the data
model specification's v1 slot set, plus the signal tags the internal (estimation) model
needs — environmental and thermal covariates that are not EED data points themselves but
are inputs to the D1 estimation methodology.

**Naming principle:** the interpretation vocabulary is generic infrastructure, not a
Green DC-branded namespace. A physical signal is what it is with or without the Green DC
modules — ambient temperature is ambient temperature — so signal tags carry no project
prefix and are usable by any consumer (dashboards, maps, other engines). Only the slots
whose meaning *is* the EED Annex II reporting semantics ("IT equipment energy inside the
reporting boundary") carry the `eed.` prefix, because the standard defines them, not
physics. This diverges from the `greendc.*` identifiers sketched in the two draft specs
(data model spec §3.2, KPI doc §5.1); those documents should be aligned to this
vocabulary at taxonomy freeze.

An optional qualifier suffix separated by `/` disambiguates per-feed meters bound at one
object (`power.feed/A`), per data model spec §3.3.

---

## 1. EED slot tags (semantics fixed by the data model specification, §3.2)

The v1 first-class slots. Semantics, units, bindable levels, and cardinality are the
spec's frozen contract; only the identifiers differ from the draft (project prefix
dropped).

| Tag | Quantity kind | Unit | Bindable at | Cardinality | Purpose |
|---|---|---|---|---|---|
| `eed.edc` | ENERGY | kWh | Facility; GRID_ENTRY domains | SUMMING | Total facility energy at utility boundary |
| `eed.eit` | ENERGY | kWh | Facility; PowerDomain; Rack | SUMMING | IT equipment energy |
| `eed.win` | VOLUME | m³ | Facility | SUMMING | Water intake |
| `eed.reuse` | ENERGY | kWh | Facility; CoolingZone | SUMMING | Energy reused outside boundary |
| `eed.ren` | ENERGY | kWh | Facility | SUMMING | Renewable energy |

The full Annex II data-point list ships in the taxonomy with deferred status ([SD-2] of
the data model spec); deferred slot identifiers are assigned at taxonomy freeze under
the same `eed.` prefix.

The spec's remaining two v1 slots — instantaneous feed power and cooling plant energy —
are generic electrical/thermal signals, not EED data points, and appear below as
`power.feed` and `cooling.energy`.

## 2. Environmental signal tags

Covariates for the estimation provider: weather normalization, free-cooling/economizer
detection, thermal-balance closure. These are *state* signals — instantaneous readings
that are time-averaged, never summed or integrated to energy (see §6.1).

| Tag | Quantity kind | Unit | Bindable at | Cardinality | Purpose |
|---|---|---|---|---|---|
| `env.temp.outdoor` | TEMPERATURE | °C | Facility | SINGLE | Outside (outdoor dry-bulb) air temperature; primary weather covariate. Source may be a physical sensor or the weather subagent |
| `env.humidity.outdoor` | HUMIDITY | % | Facility | SINGLE | Outdoor relative humidity; with dry-bulb gives wet-bulb for evaporative/adiabatic cooling models |
| `env.temp.ambient` | TEMPERATURE | °C | CoolingZone | AVERAGED | Ambient (room/cold-aisle) air temperature |
| `env.humidity.ambient` | HUMIDITY | % | CoolingZone | AVERAGED | Room relative humidity |
| `env.temp.supply` | TEMPERATURE | °C | CoolingZone | AVERAGED | Supply (CRAC/CRAH discharge) air temperature |
| `env.temp.return` | TEMPERATURE | °C | CoolingZone | AVERAGED | Return air temperature; supply/return ΔT enters air-side thermal balance |
| `env.temp.inlet` | TEMPERATURE | °C | Rack | AVERAGED | Rack inlet temperature (ASHRAE intake); links rack power to zone thermal model |
| `env.temp.outlet` | TEMPERATURE | °C | Rack | AVERAGED | Rack exhaust temperature (optional; where rear-door or exhaust sensors exist) |

## 3. Cooling signal tags

Thermal-side observability for estimating cooling energy and closing the heat balance
where electrical cooling metering is partial.

| Tag | Quantity kind | Unit | Bindable at | Cardinality | Purpose |
|---|---|---|---|---|---|
| `cooling.energy` | ENERGY | kWh | Facility; CoolingZone | SUMMING | Cooling plant energy (v1 slot per spec §3.2) |
| `cooling.power` | POWER | W | Facility; CoolingZone | SUMMING | Instantaneous cooling equipment electrical power (chillers, CRAC/CRAH, pumps, towers); gauge counterpart of `cooling.energy` |
| `cooling.load` | THERMAL_POWER | W | CoolingZone | SUMMING | Directly metered thermal load (BTU/energy meter on the loop) |
| `cooling.temp.supply` | TEMPERATURE | °C | CoolingZone | AVERAGED | Chilled-water (or coolant) supply temperature |
| `cooling.temp.return` | TEMPERATURE | °C | CoolingZone | AVERAGED | Chilled-water return temperature |
| `cooling.flow` | FLOW | m³/h | CoolingZone | SUMMING | Coolant volumetric flow rate; flow × ΔT reconstructs thermal load where no BTU meter exists |

## 4. Power-path signal tags

Loss modeling along the electrical distribution lineage. `power.feed` gives the
throughput of a domain at its metering point; input/output pairs on conversion domains
(UPS) make conversion loss observable directly.

| Tag | Quantity kind | Unit | Bindable at | Cardinality | Purpose |
|---|---|---|---|---|---|
| `power.feed` | POWER | W | PowerDomain; Rack | SUMMING | Instantaneous feed power (v1 slot per spec §3.2); `/A`, `/B`… feed qualifier |
| `power.in` | POWER | W | PowerDomain | SUMMING | Power at the domain's input (upstream) side; feed qualifier applies |
| `power.out` | POWER | W | PowerDomain | SUMMING | Power at the domain's output (downstream) side; in − out = conversion/distribution loss |
| `power.gen` | POWER | W | Facility | SUMMING | On-site generation output (PV, generator); gauge counterpart of `eed.ren` |

## 5. Water and heat-reuse signal tags

| Tag | Quantity kind | Unit | Bindable at | Cardinality | Purpose |
|---|---|---|---|---|---|
| `water.flow` | FLOW | m³/h | Facility | SUMMING | Instantaneous water intake flow rate; gauge counterpart of `eed.win` |
| `heat.export` | THERMAL_POWER | W | Facility; CoolingZone | SUMMING | Instantaneous exported (reused) heat flow; gauge counterpart of `eed.reuse` |

## 6. Implementation notes

### 6.1 New quantity kinds

The slot taxonomy in the data model spec declares ENERGY, POWER, and VOLUME. The signal
tags above require extending the quantity-kind enumeration:

- **TEMPERATURE** (°C) and **HUMIDITY** (%) — state signals. Window aggregation is
  time-weighted averaging. They must never enter energy summation or integration; the
  engine uses them as estimator covariates only.
- **FLOW** (m³/h) — gauge counterpart of VOLUME; integrates to volume over time the same
  way POWER integrates to ENERGY.
- **THERMAL_POWER** (W thermal) — integrates to thermal energy; kept distinct from
  electrical POWER so binding validation can reject an electrical meter bound to a
  thermal slot and vice versa.

### 6.2 New cardinality: AVERAGED

The spec's cardinality vocabulary is SINGLE / SUMMING. Several sensors legitimately
measure the *same* ambient quantity in one zone (two sensors in one room), and their
aggregate is a mean, not a sum. The tags above use **AVERAGED** for that case; if the
scientific team prefers, AVERAGED can collapse to SINGLE (one representative sensor per
object) at taxonomy freeze — the tag identifiers are unaffected.

### 6.3 Relationship of gauge counterparts to EED slots

`power.feed`/`cooling.power`/`water.flow`/`heat.export`/`power.gen` exist because many
sites meter instantaneous gauges rather than cumulative counters. The engine integrates
gauges explicitly (trapezoidal over the actual sample grid, spec §3.2); a facility binds
the counter slot *or* the gauge slot for a given flow, never both for the same meter.

### 6.4 Sources already in tree

- `env.temp.outdoor` / `env.humidity.outdoor` can be fed by the weather subagent
  (`Weather.Temperature`, `Weather.RelativeHumidity`) where no on-site sensor exists —
  such bindings are PROXY-class by nature and should be recorded as PROXIED in the
  metering profile.
- Grid carbon intensity (ENTSO-E subagent, `ENTSOE.CarbonIntensity`) belongs to the CUE
  / carbon strand, which has its own document; a `carbon.` tag group (e.g.
  `carbon.intensity`) is anticipated but deliberately not defined here.

### 6.5 Vocabulary governance and client support

- The vocabulary is controlled by the versioned slot taxonomy, not by a reserved
  prefix: binding-time validation (known tag, level legality, unit compatibility,
  cardinality) applies to every tag listed in the taxonomy, per spec §3.3. Tags outside
  the taxonomy remain ordinary free-form system tags, as today.
- Existing well-known system tags (`iface-inbound-bits`, …) use a hyphenated flat style;
  the dotted hierarchy here groups related signals. Both styles coexist in the same
  property.
- The management console's DCI property page keeps a fixed list of well-known tags
  (`OtherOptions.java`); the Green DC vocabulary should come from the versioned taxonomy
  rather than extending that hard-coded array.
