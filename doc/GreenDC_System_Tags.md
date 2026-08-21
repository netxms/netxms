# Green DC Monitor — DCI System Tag Vocabulary

**Status:** proposal for review
**Project:** NetXMS Green DC Monitor · 1.2.1.2.i.2/1/24/A/CFLA/006
**Derived from:** `GreenDC_Data_Model_Scientific.md` (data model specification) and
`GreenDC_KPI_DataModel_v0.1.md` (KPI data model & conceptual architecture)

Slot bindings in the Green DC model are tags on ordinary DCIs, using the existing DCI
system tag property with a controlled vocabulary under the reserved `greendc.` namespace
(KPI doc §5.1). This document enumerates the concrete tag vocabulary: the EED slot tags
fixed by the data model specification, plus the additional signal tags the internal
(estimation) model needs — environmental and thermal covariates that are not EED data
points themselves but are inputs to the D1 estimation methodology.

An optional qualifier suffix separated by `/` disambiguates per-feed meters bound at one
object (`greendc.power.feed/A`), per data model spec §3.3.

---

## 1. EED slot tags (fixed by the data model specification, §3.2)

These are the v1 first-class slots. Identifiers are frozen contract; listed here for
completeness of the vocabulary.

| Tag | Quantity kind | Unit | Bindable at | Cardinality | Purpose |
|---|---|---|---|---|---|
| `greendc.eed.edc` | ENERGY | kWh | Facility; GRID_ENTRY domains | SUMMING | Total facility energy at utility boundary |
| `greendc.eed.eit` | ENERGY | kWh | Facility; PowerDomain; Rack | SUMMING | IT equipment energy |
| `greendc.eed.win` | VOLUME | m³ | Facility | SUMMING | Water intake |
| `greendc.eed.reuse` | ENERGY | kWh | Facility; CoolingZone | SUMMING | Energy reused outside boundary |
| `greendc.eed.ren` | ENERGY | kWh | Facility | SUMMING | Renewable energy |
| `greendc.power.feed` | POWER | W | PowerDomain; Rack | SUMMING | Instantaneous feed power; `/A`, `/B`… feed qualifier |
| `greendc.cool.energy` | ENERGY | kWh | Facility; CoolingZone | SUMMING | Cooling plant energy |

The full Annex II data-point list ships in the taxonomy with deferred status ([SD-2] of
the data model spec); deferred slot identifiers are assigned at taxonomy freeze and are
out of scope here.

## 2. Environmental signal tags (new — internal model inputs)

Covariates for the estimation provider: weather normalization, free-cooling/economizer
detection, thermal-balance closure. These are *state* signals — instantaneous readings
that are time-averaged, never summed or integrated to energy (see §6.1).

| Tag | Quantity kind | Unit | Bindable at | Cardinality | Purpose |
|---|---|---|---|---|---|
| `greendc.env.temp.outdoor` | TEMPERATURE | °C | Facility | SINGLE | Outside (outdoor dry-bulb) air temperature; primary weather covariate. Source may be a physical sensor or the weather subagent |
| `greendc.env.humidity.outdoor` | HUMIDITY | % | Facility | SINGLE | Outdoor relative humidity; with dry-bulb gives wet-bulb for evaporative/adiabatic cooling models |
| `greendc.env.temp.ambient` | TEMPERATURE | °C | CoolingZone | AVERAGED | Ambient (room/cold-aisle) air temperature |
| `greendc.env.humidity.ambient` | HUMIDITY | % | CoolingZone | AVERAGED | Room relative humidity |
| `greendc.env.temp.supply` | TEMPERATURE | °C | CoolingZone | AVERAGED | Supply (CRAC/CRAH discharge) air temperature |
| `greendc.env.temp.return` | TEMPERATURE | °C | CoolingZone | AVERAGED | Return air temperature; supply/return ΔT enters air-side thermal balance |
| `greendc.env.temp.inlet` | TEMPERATURE | °C | Rack | AVERAGED | Rack inlet temperature (ASHRAE intake); links rack power to zone thermal model |
| `greendc.env.temp.outlet` | TEMPERATURE | °C | Rack | AVERAGED | Rack exhaust temperature (optional; where rear-door or exhaust sensors exist) |

## 3. Cooling-plant signal tags (new — internal model inputs)

Thermal-side observability for estimating cooling energy and closing the heat balance
where electrical cooling metering is partial.

| Tag | Quantity kind | Unit | Bindable at | Cardinality | Purpose |
|---|---|---|---|---|---|
| `greendc.cool.power` | POWER | W | Facility; CoolingZone | SUMMING | Instantaneous cooling equipment electrical power (chillers, CRAC/CRAH, pumps, towers); gauge counterpart of `greendc.cool.energy` |
| `greendc.cool.load` | THERMAL_POWER | W | CoolingZone | SUMMING | Directly metered thermal load (BTU/energy meter on the loop) |
| `greendc.cool.temp.supply` | TEMPERATURE | °C | CoolingZone | AVERAGED | Chilled-water (or coolant) supply temperature |
| `greendc.cool.temp.return` | TEMPERATURE | °C | CoolingZone | AVERAGED | Chilled-water return temperature |
| `greendc.cool.flow` | FLOW | m³/h | CoolingZone | SUMMING | Coolant volumetric flow rate; flow × ΔT reconstructs thermal load where no BTU meter exists |

## 4. Power-path signal tags (new — internal model inputs)

Loss modeling along the electrical distribution lineage. `greendc.power.feed` gives the
throughput of a domain at its metering point; input/output pairs on conversion domains
(UPS) make conversion loss observable directly.

| Tag | Quantity kind | Unit | Bindable at | Cardinality | Purpose |
|---|---|---|---|---|---|
| `greendc.power.in` | POWER | W | PowerDomain | SUMMING | Power at the domain's input (upstream) side; feed qualifier applies |
| `greendc.power.out` | POWER | W | PowerDomain | SUMMING | Power at the domain's output (downstream) side; in − out = conversion/distribution loss |
| `greendc.power.gen` | POWER | W | Facility | SUMMING | On-site generation output (PV, generator); gauge counterpart of `greendc.eed.ren` |

## 5. Water and heat-reuse signal tags (new — internal model inputs)

| Tag | Quantity kind | Unit | Bindable at | Cardinality | Purpose |
|---|---|---|---|---|---|
| `greendc.water.flow` | FLOW | m³/h | Facility | SUMMING | Instantaneous water intake flow rate; gauge counterpart of `greendc.eed.win` |
| `greendc.heat.export` | THERMAL_POWER | W | Facility; CoolingZone | SUMMING | Instantaneous exported (reused) heat flow; gauge counterpart of `greendc.eed.reuse` |

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

`greendc.power.feed`/`greendc.cool.power`/`greendc.water.flow`/`greendc.heat.export`/
`greendc.power.gen` exist because many sites meter instantaneous gauges rather than
cumulative counters. The engine integrates gauges explicitly (trapezoidal over the
actual sample grid, spec §3.2); a facility binds the counter slot *or* the gauge slot
for a given flow, never both for the same meter.

### 6.4 Sources already in tree

- `greendc.env.temp.outdoor` / `greendc.env.humidity.outdoor` can be fed by the weather
  subagent (`Weather.Temperature`, `Weather.RelativeHumidity`) where no on-site sensor
  exists — such bindings are PROXY-class by nature and should be recorded as PROXIED in
  the metering profile.
- Grid carbon intensity (ENTSO-E subagent, `ENTSOE.CarbonIntensity`) belongs to the CUE
  / carbon strand, which has its own document; a `greendc.carbon.*` sub-namespace is
  reserved but deliberately not defined here.

### 6.5 Namespace and client support

- The `greendc.` prefix is reserved as a system-tag namespace with binding-time
  validation (vocabulary, level legality, unit compatibility, cardinality) per spec
  §3.3; unknown `greendc.*` tags are rejected at configuration time.
- Existing well-known system tags (`iface-inbound-bits`, …) use a hyphenated flat style;
  the dotted `greendc.` hierarchy is intentional per the KPI doc and coexists with it.
- The management console's DCI property page keeps a fixed list of well-known tags
  (`OtherOptions.java`); the Green DC vocabulary should come from the versioned taxonomy
  rather than extending that hard-coded array.
