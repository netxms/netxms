# ENTSO-E Transparency Platform subagent

Ingests electricity-grid data from the [ENTSO-E Transparency Platform](https://transparency.entsoe.eu)
and exposes it as NetXMS metrics. Its primary output is **average, production-based
carbon intensity (gCO₂/kWh) per bidding zone** — the signal for carbon-aware
scheduling — alongside the raw generation mix, forecasts and cross-border flows.

Design document: [`doc/ENTSOE_Integration_Design.md`](../../../../doc/ENTSOE_Integration_Design.md).
Tracked by issue #3397.

## Prerequisites

- An ENTSO-E API token. Register at the Transparency Platform and request web-API
  access (email `transparency@entsoe.eu`), then create a security token in your
  account settings.

## Configuration

```ini
[ENTSOE]
Token           = your-security-token
Zones           = LV, EE, LT          ; names resolved against the built-in catalog
PollInterval    = 900                 ; seconds (actuals refresh ~hourly)
Lookback        = 10800               ; seconds; window for the latest complete interval
RequestTimeout  = 30                  ; seconds per HTTP request
EmissionFactors = /etc/netxms/entsoe-factors.conf   ; optional override
DefaultFactor   = 500                 ; gCO2/kWh for an unmapped production type
ZoneCatalog     = /etc/netxms/entsoe-zones.csv      ; optional override/extension
```

Zones named in `Zones=` are resolved against the built-in catalog (EIC code and
neighbour list). To add a zone the catalog does not know, or to override a
border, add an explicit block:

```ini
[ENTSOE/Zone/XX]
EIC    = 10Y....
Border = YY:10Y....                   ; name:EIC, repeatable; NAME alone looks up the catalog
```

## Metrics

| Metric | Type | Description |
|--------|------|-------------|
| `ENTSOE.CarbonIntensity(zone)` | float | Average production-based carbon intensity (gCO₂/kWh) |
| `ENTSOE.RenewableShare(zone)` | float | Renewable share of generation (%) |
| `ENTSOE.LowCarbonShare(zone)` | float | Renewable + nuclear share (%) |
| `ENTSOE.TotalGeneration(zone)` | float | Total generation excluding storage (MW) |
| `ENTSOE.Generation(zone, psrType)` | float | Generation for one production type, e.g. `B16` (MW) |
| `ENTSOE.NetImport(zone)` | float | Net cross-border import (MW, import positive) |
| `ENTSOE.DataAge(zone)` | int64 | Seconds since the served generation interval (freshness/health) |

Tables: `ENTSOE.Generation(zone)` (per-type breakdown), `ENTSOE.CrossBorderFlows(zone)`,
`ENTSOE.Forecast(zone)` (wind/solar/load forward curve).
Lists: `ENTSOE.Zones`, `ENTSOE.GenerationTypes(zone)` (for instance discovery).

A metric for a configured zone that has not yet been polled returns an error
(not `0`); `ENTSOE.DataAge` lets you threshold-alarm on a stale feed.

## Emission factors

Carbon intensity is computed from an emission-factor table. Compiled-in defaults
are IPCC AR5 lifecycle medians (gCO₂eq/kWh). Override with `EmissionFactors=`,
one entry per line:

```
# code = gCO2eq/kWh, category    (category: renewable | nuclear | fossil | storage | other)
B04 = 490, fossil
B16 = 45, renewable
B14 = 12, nuclear
```

`storage` production types are excluded from the carbon-intensity and share
denominators.

## Zone catalog

The built-in catalog (`catalog_data.h`) is generated from ENTSO-E's published
area/EIC registry by `tools/generate_catalog.py`. Re-run the generator after a
bidding-zone topology change and regenerate the header:

```
python3 tools/generate_catalog.py > catalog_data.h
```

An external `ZoneCatalog=` file (CSV: `NAME,EIC,neighbour1,neighbour2,...`)
overrides or extends the built-in catalog without recompiling.

## Assumptions and limitations

- **Average, not marginal** carbon intensity.
- **Production-based, not consumption-based**: for an import-heavy zone the
  actual consumed mix differs from the zone's own generation. Cross-border flow
  data is collected (`ENTSOE.NetImport` / `ENTSOE.CrossBorderFlows`) to enable a
  consumption-based figure in the future.
