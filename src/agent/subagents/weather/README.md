# Weather subagent

Ingests weather and solar-irradiance data from public weather APIs and exposes it
as NetXMS metrics. Its primary output is an **hourly weather/solar forecast curve
per location** — a forecast input for GreenDC carbon-aware scheduling (companion
to the ENTSO-E grid feed) — alongside current conditions and an optional
ensemble spread.

Tracked by issues #3408 and #3570.

## Providers

| Name | Service | Notes |
|------|---------|-------|
| `openmeteo` | [Open-Meteo](https://open-meteo.com) | Default. Solar irradiance and ensemble spread. Keyless tier is non-commercial only; set `ApiKey` for a commercial subscription. |
| `metno` | [MET Norway Locationforecast 2.0](https://api.met.no/weatherapi/locationforecast/2.0/documentation) | Open data, no key. Authoritative for Nordic locations. **No solar radiation and no ensemble spread.** Requires an identifying `UserAgent`. |
| `brightsky` | [Bright Sky](https://brightsky.dev) | Free JSON API over DWD open data (MOSMIX forecasts + station observations). **Germany only**, no key. Global irradiance, **no direct radiation and no ensemble spread.** Current conditions are real station observations. |

The provider is selected **per location**, because their capabilities differ:
locations feeding irradiance-driven decisions must use `openmeteo` or (in
Germany) `brightsky`, while Nordic locations that only need
temperature/wind/cloud/humidity/precipitation can use `metno`.

MET Norway data is licensed under [CC-BY 4.0](https://creativecommons.org/licenses/by/4.0/);
attribution to the Norwegian Meteorological Institute is required when data
derived from it is published. DWD data served through Bright Sky is likewise
[CC-BY 4.0](https://creativecommons.org/licenses/by/4.0/) with attribution to
Deutscher Wetterdienst; Bright Sky itself is a community-run public instance
(self-hostable) rather than a DWD service.

## Prerequisites

Outbound HTTPS. No API key is needed for the Open-Meteo free tier, MET Norway
or Bright Sky; a commercial Open-Meteo subscription supplies a key.

## Configuration

```ini
[Weather]
Provider       = openmeteo           ; default provider for locations that do not name one
ApiKey         =                     ; Open-Meteo commercial subscription key (empty = free tier)
UserAgent      = AcmeNetXMS/1.0 ops@acme.example   ; mandatory when any location uses metno
Location       = datacenter-fra: 50.1109, 8.6821   ; "name: lat, lon", repeatable
Location       = datacenter-hel: 60.1699, 24.9384
PollInterval   = 900                 ; seconds
ForecastDays   = 2                   ; forecast horizon, 1..16 days
RequestTimeout = 30                  ; seconds per HTTP request
EnableEnsemble = no                  ; fetch ensemble spread (extra API calls, Open-Meteo only)
EnsembleModel  = icon_seamless       ; ensemble model when EnableEnsemble = yes

[Weather/Location/datacenter-osl]
Latitude  = 59.9139
Longitude = 10.7522
Provider  = metno

[Weather/Location/datacenter-fra]
Latitude  = 50.1109
Longitude = 8.6821
Provider  = brightsky
```

`Location` lines and `[Weather/Location/NAME]` blocks can be mixed; only the
block form can select a provider. Blocks are applied after the `Location` lines,
so a block redefines a name that appeared in both.

When `ApiKey` is set, Open-Meteo requests go to the commercial hosts
(`customer-api.open-meteo.com`, `customer-ensemble-api.open-meteo.com`) and carry
the key.

A metric instance is either a **configured location name** or a raw **`lat,lon`
pair** — the handler detects which. Raw pairs are registered and polled on
demand against the default provider, so `Weather.Temperature(50.11,8.68)` works
without any configuration. A named location and an equal coordinate pair on the
same provider resolve to the same polled entry (one shared HTTP fetch,
deduplicated at ~11 m precision).

## Metrics

Current conditions (scalars):

| Metric | Type | Description |
|--------|------|-------------|
| `Weather.Temperature(location)` | float | Air temperature at 2 m (°C) |
| `Weather.CloudCover(location)` | float | Total cloud cover (%) |
| `Weather.ShortwaveRadiation(location)` | float | Global horizontal irradiance (W/m²), Open-Meteo and Bright Sky |
| `Weather.DirectRadiation(location)` | float | Direct radiation (W/m²), Open-Meteo only |
| `Weather.WindSpeed(location)` | float | Wind speed at 10 m (km/h) |
| `Weather.RelativeHumidity(location)` | float | Relative humidity at 2 m (%) |
| `Weather.Precipitation(location)` | float | Precipitation over the current hour (mm) |
| `Weather.DataAge(location)` | int64 | Seconds since the current observation (freshness/health) |
| `Weather.Provider(location)` | string | Provider serving this location |

Tables:

- `Weather.Forecast(location)` — hourly forward curve, `TIME`-indexed (UTC),
  with temperature, cloud cover, shortwave/direct radiation, wind speed,
  humidity and precipitation columns. This is the primary input for the
  scheduler.
- `Weather.ForecastEnsemble(location)` — per-hour min/mean/max spread of
  shortwave radiation and temperature across ensemble members (requires
  `EnableEnsemble = yes` and an Open-Meteo location).

List: `Weather.Locations` — configured location names (for instance discovery).

A metric for a location that has not yet been polled returns an error (not `0`);
a variable the provider does not report for the location is likewise absent
rather than a synthetic zero — so `Weather.ShortwaveRadiation` on a `metno`
location always errors. `Weather.DataAge` lets you threshold-alarm on a stale
feed.

## Units and horizon

Values are normalized by the provider adapters: °C, km/h, W/m², %, mm (MET
Norway reports wind in m/s and is converted; Bright Sky reports solar energy
per hour in kWh/m² and is converted to mean irradiance). Times are UTC. The
hourly curve spans `ForecastDays` days from the current hour; MET Norway
returns hourly resolution for roughly the first two days and coarser steps
beyond that. Bright Sky forecast records carry no relative humidity, so that
column is present only for the observed (past) hours of a `brightsky` curve.

## Request caching

Responses carry `Expires` and `Last-Modified`. The subagent stores both per
location and per request kind: a location whose response is still within its
`Expires` window is not re-requested at all, and a refresh past that window is
conditional (`If-Modified-Since`), so an unchanged forecast costs a 304 instead
of a full body. This is mandated by MET Norway's terms of service and is good
practice against Open-Meteo as well. Bright Sky sends neither header, so every
poll of a `brightsky` location is a full request; its request window is pinned
to 00:00 UTC of the current day so the URL itself is stable within a day.

## Assumptions and limitations

- **Deterministic forecast by default.** Forecast uncertainty is available only
  through the optional ensemble table; consumers doing stochastic optimization
  should enable it and use an Open-Meteo location.
- **Grid-snapped coordinates.** Open-Meteo and MET Norway resolve each request
  to their nearest model grid cell, so nearby coordinates may return identical
  data. Bright Sky resolves to the nearest MOSMIX station and nearest observing
  stations within 50 km, and returns an error for coordinates outside Germany.
  Coordinates are sent with four decimals, MET Norway's documented maximum.
- **Observations only from Bright Sky.** Open-Meteo and MET Norway serve
  forecasts, so their "current" snapshot is the forecast value for the present
  hour. Bright Sky fills past hours from DWD station measurements once DWD
  has published them (the current hour is usually still the MOSMIX value until
  its observation lands), so the snapshot is an observation for all but the
  most recent hour. Forecast records carry no relative humidity, so
  `Weather.RelativeHumidity` on a `brightsky` location errors whenever the
  snapshot is still a forecast value.
