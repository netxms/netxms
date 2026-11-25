# ENTSO-E Transparency Platform Integration — Design Document

Status: implemented — tracked by issue #3397. Subagent at
`src/agent/subagents/entsoe/`; unit tests at `tests/agent/unit/entsoe/`.

This document describes a new agent subagent, **`entsoe`**, that ingests
electricity-grid data from the ENTSO-E Transparency Platform
(<https://transparency.entsoe.eu>) and exposes it through the standard NetXMS
metric interface. Its primary purpose is to provide **average, production-based
carbon intensity (gCO₂/kWh) per bidding zone** as a prerequisite for the GreenDC
carbon-aware scheduling component, while remaining a neutral, general-purpose
grid-data source.

## 1. Background and motivation

GreenDC's carbon-aware scheduling core (rolling-horizon stochastic MILP) needs a
carbon-intensity signal per bidding zone, plus a forecast horizon to build its
scenarios. ENTSO-E publishes the raw inputs — actual generation per production
type, wind/solar and load forecasts, and cross-border physical flows — as
time-windowed XML documents behind a token-authenticated REST API.

Those documents are awkward to consume directly as web-service DCIs: the response
is a multi-`TimeSeries` window (not a point-in-time value), production-type codes
need mapping, generation is published with a delay and revised, and carbon
intensity has to be *derived*. A dedicated subagent is NetXMS's established idiom
for wrapping an external source with its own auth, quirks, and evolving format
(cf. `prometheus`, `mqtt`, `mongodb`) and gives all of the above a single home.

Design decisions locked during brainstorming:

1. **Subagent, not server module or web-service DCIs.** External API data with a
   uniform-interface requirement is a subagent's job; a server module is reserved
   for when the server must own new object semantics, and web-service DCIs cannot
   host token/rate-limit/delay handling, code→category mapping, or CI derivation.
2. **Source-neutral internal model.** The subagent's cached representation is
   *not* ENTSO-E-shaped; only a thin adapter knows about `documentType`,
   `psrType`, EIC codes, and XML. A future second source (ElectricityMaps, a
   national TSO) is a new adapter, not a new metric contract.
3. **Data scope: actuals + forecast + cross-border flows.** Enables
   production-based CI now and a consumption-based (import-aware) CI later without
   re-plumbing.
4. **Carbon intensity is computed in the subagent** from an editable, citable
   emission-factor file. Forecast CI is *not* computed here — that dispatch
   modelling belongs to the MILP.
5. **Comprehensive built-in zone catalog.** Bidding-zone EIC codes and adjacency
   are well-known static reference data; the user names zones, the subagent
   resolves the rest.
6. **Average, production-based CI**, with import-aware CI as a documented future
   extension. Average-vs-marginal and production-vs-consumption are stated
   assumptions/limitations, not silent choices.

## 2. Architecture

A new subagent at `src/agent/subagents/entsoe/`, structured like `prometheus`:
an external source is polled on a background thread into a cached snapshot, and
metric handlers read the cache without ever touching the network.

The abstraction that absorbs API changes is a **source-neutral model**, one
instance per configured zone:

```
ZoneState {                     // refreshed per poll, guarded by a mutex
   GenerationSnapshot actual     // interval timestamp; MW per category; total; CI; renewable%; lowCarbon%
   ForecastCurve      forecast   // [target_time -> wind/solar/load MW]
   FlowSnapshot       flows      // [neighbour -> import/export/net MW]
}
```

An **ENTSO-E adapter** is the only code aware of `documentType`, `processType`,
`psrType`, EIC codes, XML, and the publication delay. It fetches every document
type for a zone and populates `ZoneState`. Every metric reads from `ZoneState`.

- **HTTP**: call libcurl directly — include the common `nxlibcurl.h` header,
  set curl options with a write callback accumulating the response into a
  `ByteStream`, and `curl_easy_perform()`. libcurl is linked transitively through
  `libnetxms`. There is no shared request helper; `prometheus/scraper.cpp` is the
  reference example to copy the boilerplate from.
- **XML**: parse with vendored **pugixml** (DOM). ENTSO-E documents use a default
  namespace, so element names are unprefixed (`TimeSeries`, `Period`, `Point`,
  `MktPSRType`, `psrType`, `quantity`) and traverse directly by name.
- **Threading**: one background poller (`ThreadCreateEx`) loops over configured
  zones on the poll interval, builds a new snapshot off-lock (HTTP + parse are
  slow), then swaps it in under a brief lock. Handlers copy out under lock. The
  poller honours a shutdown `Condition` on unload.

**Deployment**: runs on any agent with internet egress (typically the server
host's agent). Zone is a metric *argument*, so one instance serves N zones. DCIs
attach to whatever node monitors that agent — model a "Grid: <zone>" node, or
hang the DCIs on the relevant DC node.

## 3. Metric surface

All metrics take **zone** (the catalog name, e.g. `LV`) as the first argument.

### Scalar parameters (latest complete actual interval)

| Metric | Unit | Notes |
|--------|------|-------|
| `ENTSOE.CarbonIntensity(zone)` | gCO₂/kWh | primary GreenDC signal |
| `ENTSOE.RenewableShare(zone)` | % | |
| `ENTSOE.LowCarbonShare(zone)` | % | renewable + nuclear |
| `ENTSOE.TotalGeneration(zone)` | MW | |
| `ENTSOE.Generation(zone, psrType)` | MW | one production type, e.g. `B16` |
| `ENTSOE.NetImport(zone)` | MW | net cross-border, import positive |
| `ENTSOE.DataAge(zone)` | s | seconds since served interval → staleness threshold |

### Tables

| Table | Rows |
|-------|------|
| `ENTSOE.Generation(zone)` | psrType code, friendly name, category, MW, share % |
| `ENTSOE.CrossBorderFlows(zone)` | neighbour, import MW, export MW, net MW |
| `ENTSOE.Forecast(zone)` | target UTC timestamp, wind MW, solar MW, load MW — the forward curve the MILP consumes, snapshot-replaced each poll |

### Lists

| List | Purpose |
|------|---------|
| `ENTSOE.Zones` | configured zone names |
| `ENTSOE.GenerationTypes(zone)` | psrType codes present → drives instance discovery for per-type MW DCIs |

Parameters and tables are separate NXCP registries, so sharing the base name
`ENTSOE.Generation` across a scalar and a table is intentional and conventional.

Cold start or a persistently failing zone returns an NXCP error
(`SYSINFO_RC_ERROR`), never a synthetic `0`, so a missing feed is distinguishable
from genuine zero generation.

## 4. ENTSO-E API mechanics (adapter)

Endpoint `https://web-api.tp.entsoe.eu/api`, auth via `securityToken=`. Times are
UTC; `periodStart`/`periodEnd` in `yyyyMMddHHmm`.

| Data | documentType | processType | domain param |
|------|--------------|-------------|--------------|
| Actual generation per type | `A75` | `A16` (Realised) | `in_Domain=<zoneEIC>` |
| Wind & solar forecast | `A69` | `A01` (day-ahead) | `in_Domain=<zoneEIC>` |
| Load forecast | `A65` | `A01` | `outBiddingZone_Domain=<zoneEIC>` |
| Cross-border physical flow | `A11` | — | `in_Domain`/`out_Domain` per border, both directions |

Request volume is `~3 + 2×neighbours` per zone per poll — well under the ENTSO-E
400 req/min token limit. A small inter-request delay and `429` backoff are applied
regardless.

**Point → time mapping (parsing gotcha).** Each `TimeSeries` carries
`MktPSRType/psrType`; each `Period` has `timeInterval/start` and `resolution`
(`PT60M` or `PT15M` — handle both). A `Point` has 1-based `position` and
`quantity`; `timestamp = start + (position − 1) × resolution`. **ENTSO-E omits
unchanged points** — a gap means "carry forward the last value", so the adapter
must forward-fill, not treat gaps as zero.

**Publication delay.** Actuals lag ~1 h and may be revised. The adapter fetches
`[now − Lookback, now]` and serves the latest *fully elapsed* interval; its
timestamp drives `ENTSOE.DataAge`.

**Error responses.** On a bad/empty query ENTSO-E returns an
`Acknowledgement_MarketDocument` with a `Reason/text` rather than the expected
document. The adapter detects that root element, logs the reason, and retains the
last good `ZoneState` instead of blanking it.

**Pumped storage.** `B10` may include a separate consumption `TimeSeries`; the
adapter keys it out so it is not double-counted in totals or CI.

## 5. Emission factors and carbon-intensity computation

The factor set and the renewable/low-carbon classification are the assumptions
reviewers will scrutinise, so both live in an editable, citable file rather than
in code. A default file ships with the subagent (header-documenting its source,
e.g. IPCC AR5 lifecycle medians); `EmissionFactors=` overrides it.

```
# code  gCO2eq/kWh  category      # source: IPCC AR5 lifecycle medians
B04  =  490,  fossil       # Fossil Gas
B05  =  820,  fossil       # Fossil Hard coal
B02  = 1050,  fossil       # Lignite
B14  =   12,  nuclear      # Nuclear (low-carbon, not renewable)
B16  =   45,  renewable    # Solar
B19  =   11,  renewable    # Wind Onshore
B18  =   12,  renewable    # Wind Offshore
B11  =   24,  renewable    # Hydro Run-of-river
B01  =  230,  renewable    # Biomass (contested — re-tune here)
B10  =    0,  storage      # Pumped storage — excluded from CI by default
```

Computation per served interval (production-based):

```
CI             = Σ(MWᵢ × factorᵢ) / Σ MWᵢ           # over generation types → gCO2/kWh
renewableShare = Σ MW(renewable) / total
lowCarbonShare = Σ MW(renewable ∪ nuclear) / total
```

`storage` and net import are excluded from the production-based CI. Net import is
where a future consumption-based CI would blend neighbour intensities — the data
is already available via `A11`.

**Unmapped `psrType` safety.** A production-type code with no factor line uses a
configurable `DefaultFactor=` and logs a one-time warning, so a new ENTSO-E code
skews CI visibly rather than silently dropping from the denominator.

## 6. Zone catalog and configuration

Bidding-zone EIC codes and interconnection adjacency are well-known static
reference data. The subagent ships a **comprehensive built-in catalog** covering
all ENTSO-E bidding zones at true bidding-zone granularity — including split
zones and their intra-country borders (Nordic `SE1–SE4`/`NO1–NO5`/`DK1`/`DK2`,
the Italian zones, etc.) and non-EU areas ENTSO-E publishes. The catalog is
`name -> {EIC, [neighbours]}`.

The catalog is generated from ENTSO-E's published area/EIC registry via a
checked-in generator script, with the source and retrieval date in a header
comment, so a topology change is a mechanical regen. An optional external
`ZoneCatalog=` file overrides/extends it without recompiling.

Normal configuration reduces to a token and a zone name list:

```ini
[ENTSOE]
Token           = 1a2b3c4d-....
Zones           = LV, EE, LT              # names resolved against the built-in catalog
PollInterval    = 900                     # s; actuals refresh ~hourly
Lookback        = 10800                   # s; window for "latest complete interval"
EmissionFactors = {installdir}/etc/entsoe-factors.conf
DefaultFactor   = 500                     # gCO2/kWh for an unmapped psrType
```

`EIC` and `Border` for named zones are looked up from the catalog, so
`NetImport`/`CrossBorderFlows` work without extra config. The hierarchical block
is an **escape hatch**, needed only to add a zone the catalog does not yet know or
to override a border:

```ini
[ENTSOE/Zone/XX]                          # section path -> zone name "XX"
EIC    = 10Y....
Border = YY:10Y....                       # name:EIC, repeatable
```

(Section paths use the current hierarchical bracket syntax; the trailing element
is the zone name. The legacy `*Section` syntax is deprecated.)

## 7. Error handling and resilience

- **Per-document last-good retention**: the poller updates `ZoneState` per
  document, so a single failing request (e.g. `A11`) does not blank generation/CI.
  A failed poll ages the snapshot rather than clearing it.
- **Freshness as a first-class signal**: `ENTSOE.DataAge(zone)` lets an operator
  threshold-alarm on a stale feed (e.g. > 3 h), and lets the MILP refuse to act on
  stale CI. This is native monitoring-product behaviour, not a bolt-on.
- **Typed HTTP failures**, each logged distinctly via `nxlog_debug_tag("entsoe",…)`:
  timeout/connection (transient), `401` (bad/expired token — loud, actionable),
  `429` (honour `Retry-After`, exponential backoff), and the acknowledgement-doc
  case.
- **No synthetic zeros**: never-populated or persistently-failing zones return an
  NXCP error, not `0`.
- **Bounded memory**: only the latest snapshot + forecast curve per zone.
- **Non-goal (YAGNI)**: no historical backfill into DCI history — series accrue as
  collection runs; the scheduler seeds its own cold-start window if needed.

## 8. Testing

**Offline unit tests** (in `tests/`, no network, CI-safe):

- Adapter parsing against saved ENTSO-E XML fixtures: a normal `A75` document,
  a `PT15M`-resolution document, a sparse-points document (verifies forward-fill),
  a pumped-storage consumption series (verifies it is keyed out), and an
  `Acknowledgement_MarketDocument` (verifies last-good retention).
- CI computation: fixed generation mix + known factor file → exact gCO₂/kWh,
  renewable %, low-carbon %; plus an unmapped `psrType` → `DefaultFactor` applied
  and warning logged.
- Point→time mapping across `PT15M`/`PT60M`.

**Integration smoke** (manual, gated on a real token): `nxagentd -c test.conf`,
then `nxget … 'ENTSOE.CarbonIntensity(LV)'` returns a plausible value,
`ENTSOE.Generation(LV)` a sane table, `ENTSOE.DataAge(LV)` under ~2 h. Resilience:
bad token → `401` logged, metric returns error not `0`; kill connectivity →
values age, `DataAge` climbs, no crash.

## 9. Future extensions (out of scope for v1)

- **Consumption-based (import-aware) CI**, blending neighbour intensities weighted
  by `A11` physical flows — the flow data is ingested from v1 specifically to make
  this a pure computation change later.
- **Additional data sources** (ElectricityMaps, national TSO APIs) as alternative
  adapters behind the same `ZoneState` model and metric contract.
- **Marginal emissions factors**, if a defensible marginal dataset becomes
  available for the target zones.
