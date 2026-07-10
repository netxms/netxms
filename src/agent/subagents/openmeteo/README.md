# Open-Meteo weather subagent

Ingests weather and solar-irradiance data from the [Open-Meteo API](https://open-meteo.com)
and exposes it as NetXMS metrics. Its primary output is an **hourly weather/solar
forecast curve per location** — a forecast input for GreenDC carbon-aware
scheduling (companion to the ENTSO-E grid feed) — alongside current conditions
and an optional ensemble spread.

Tracked by issue #3408.

## Prerequisites

None. The Open-Meteo API is keyless for non-commercial use; the subagent needs
only outbound HTTPS.

## Configuration

```ini
[OpenMeteo]
Location       = datacenter-fra: 50.1109, 8.6821   ; "name: lat, lon", repeatable
Location       = datacenter-hel: 60.1699, 24.9384
PollInterval   = 900                 ; seconds
ForecastDays   = 2                   ; forecast horizon, 1..16 days
RequestTimeout = 30                  ; seconds per HTTP request
EnableEnsemble = no                  ; fetch ensemble spread (extra API calls)
EnsembleModel  = icon_seamless       ; ensemble model when EnableEnsemble = yes
```

A metric instance is either a **configured location name** or a raw **`lat,lon`
pair** — the handler detects which. Raw pairs are registered and polled on
demand, so `OpenMeteo.Temperature(50.11,8.68)` works without any configuration.
A named location and an equal coordinate pair resolve to the same polled entry
(one shared HTTP fetch, deduplicated at ~11 m precision).

## Metrics

Current conditions (scalars):

| Metric | Type | Description |
|--------|------|-------------|
| `OpenMeteo.Temperature(location)` | float | Air temperature at 2 m (°C) |
| `OpenMeteo.CloudCover(location)` | float | Total cloud cover (%) |
| `OpenMeteo.ShortwaveRadiation(location)` | float | Global horizontal irradiance (W/m²) |
| `OpenMeteo.DirectRadiation(location)` | float | Direct radiation (W/m²) |
| `OpenMeteo.WindSpeed(location)` | float | Wind speed at 10 m (km/h) |
| `OpenMeteo.RelativeHumidity(location)` | float | Relative humidity at 2 m (%) |
| `OpenMeteo.DataAge(location)` | int64 | Seconds since the current observation (freshness/health) |

Tables:

- `OpenMeteo.Forecast(location)` — hourly forward curve, `TIME`-indexed (UTC),
  with temperature, cloud cover, shortwave/direct radiation, wind speed and
  humidity columns. This is the primary input for the scheduler.
- `OpenMeteo.ForecastEnsemble(location)` — per-hour min/mean/max spread of
  shortwave radiation and temperature across ensemble members (requires
  `EnableEnsemble = yes`).

List: `OpenMeteo.Locations` — configured location names (for instance discovery).

A metric for a location that has not yet been polled returns an error (not `0`);
a variable the API does not report for the location is likewise absent rather
than a synthetic zero. `OpenMeteo.DataAge` lets you threshold-alarm on a stale
feed.

## Units and horizon

Values use Open-Meteo defaults: °C, km/h, W/m², %. Times are UTC (the subagent
requests `timeformat=unixtime&timezone=GMT`). The hourly curve spans
`ForecastDays` days from the current hour.

## Assumptions and limitations

- **Deterministic forecast by default.** Forecast uncertainty is available only
  through the optional ensemble table; consumers doing stochastic optimization
  should enable it.
- **Grid-snapped coordinates.** Open-Meteo resolves each request to its nearest
  model grid cell, so nearby coordinates may return identical data.
