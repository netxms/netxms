# DCI Data Aggregation — Design

Related issue: [#419](https://github.com/netxms/netxms/issues/419)

## Motivation

Provide automated rollup of DCI history into coarser time buckets (hourly, daily) for:
- **Storage reduction** — long-term raw data is expensive; aggregates compress 60×–1440×
- **Query performance** — raw queries over months/years are slow; aggregates keep graph queries fast

Prior art: Zabbix (history + trends), Graphite/statsd tiered retention, MikroTik The Dude.

## Tier Model

Three fixed tiers:

| Tier | Bucket | Typical range served |
|---|---|---|
| Raw | collection interval | minutes → hours |
| Hourly | 1 hour | days → months |
| Daily | 1 day | months → years |

**Rationale for fixed 3 tiers**:
- 15-min tier gives only 15× compression over 60s raw — marginal
- 8-hour tier gives only 8× over hourly — below threshold
- Hour/day align to calendar reporting boundaries; 15m/8h do not
- Configurable N tiers rejected: ~3× schema/code/UI complexity, breaks TSDB continuous aggregates

## Aggregated Values

Every aggregate row stores **min, max, avg, count**.

- Sum omitted — derivable as `avg × count`
- `count` enables correct weighted re-aggregation (daily-from-hourly)
- min/max enables band-graph rendering

## Row Shape

```
item_id         integer          not null
bucket_start    <timestamp>      not null    -- SQL_INT64 non-TSDB, timestamptz TSDB
min_value       double precision null
max_value       double precision null
avg_value       double precision null
sample_count    integer          not null
PRIMARY KEY (item_id, bucket_start)
```

Native `double precision` (no `varchar(255)` — string DCIs are excluded from aggregation).

## Storage Layout

### Non-TSDB backends

Per-object tables mirror existing `idata_<N>` pattern:
- `idata_1h_<N>` — hourly aggregates for object N
- `idata_1d_<N>` — daily aggregates for object N

Created lazily on first rollup for that object. Dropped alongside `idata_<N>` on object deletion.

Sizing for a large deployment (5,000 nodes × 100 DCIs × 60s collection, 1-year hourly retention):
- ~876k hourly rows per node — comfortable
- vs. a single shared `idata_1h` table of 4.38B rows, which would force per-backend partitioning

### TSDB backend

TimescaleDB rejects continuous aggregates whose source is a view (`ERROR: invalid continuous aggregate view; DETAIL: At least one hypertable should be used in the view definition`). The aggregates therefore mirror the existing storage-class split of raw `idata_sc_*` hypertables:

Per storage class `S` ∈ {default, 7, 30, 90, 180, other}:
- `idata_1h_sc_<S>` — CAGG directly over `idata_sc_<S>` with a 1-hour `time_bucket`. Numeric regex in the WHERE clause excludes string DCIs.
- `idata_1d_sc_<S>` — hierarchical CAGG over `idata_1h_sc_<S>` with a 1-day `time_bucket`. Weighted average (`sum(avg*count)/sum(count)`) keeps it equivalent to averaging raw values directly.

Two read-side views aggregate the per-class CAGGs:
- `idata_1h` — `UNION ALL` of the six hourly CAGGs
- `idata_1d` — `UNION ALL` of the six daily CAGGs

The query dispatcher (future phase) reads from the two unified views and stays storage-class-agnostic.

**Bonus from per-class CAGGs**: chunk-drop retention is now per-storage-class (one `add_retention_policy` per CAGG), so retention granularity is exactly what users effectively requested when they picked a storage class for the DCI. Per-DCI retention overrides are still not honored on TSDB.

TSDB minimum version: ≥ 2.10 (hierarchical CAGGs). The CAGG `GROUP BY` must reference the literal `time_bucket(...)` expression — a SELECT alias is rejected by the CAGG validator. Document in release notes.

### String / table DCIs

Excluded entirely:
- String DCIs — min/max/avg meaningless for text
- Table DCIs (`tdata`) — multi-row/multi-column; aggregation would be a different feature

Counters need no special handling — aggregate `idata_value` (already rate after delta calc).

### Rule B — eligibility threshold

A tier is active for a DCI only when **collection interval ≤ bucket / 2** (guarantees ≥ 2 samples per bucket on average; min/max/avg are genuinely non-degenerate):

| Collection interval | Hourly? | Daily? |
|---|---|---|
| ≤ 30 min | ✓ | ✓ |
| 30 min < interval ≤ 12 h | ✗ | ✓ |
| > 12 h | ✗ | ✗ (raw already at daily granularity) |

Applies to non-TSDB rollup. On TSDB we accept CAGG output as-is (some degenerate 1-sample buckets in return for avoiding per-DCI metadata joins in the CAGG).

## Rollup Engine

### Non-TSDB — batch rollup

Two scheduled tasks:
- `DataCollection.Aggregation.HourlyRollup` — cron `5 * * * *`
- `DataCollection.Aggregation.DailyRollup` — cron `30 0 * * *`

Per run, for each eligible DCI:
1. Read `aggregation_watermark` from `items` (null → start from `now − close_window`)
2. For each bucket in `[watermark, now − close_window)`:
   - `SELECT min, max, avg, count FROM idata_<N> WHERE item_id=? AND idata_timestamp in [bucket_start, bucket_end)`
   - If `count = 0` — skip (chart gap)
   - Else — UPSERT into `idata_1h_<N>` / `idata_1d_<N>` (PG `ON CONFLICT`, MySQL `ON DUPLICATE KEY UPDATE`, MSSQL `MERGE`, SQLite `INSERT OR REPLACE`)
3. Advance watermark; null if fully caught up

Per-object parallelism via existing data collection thread pool.

### TSDB — continuous aggregates

For each storage class `S`:

```sql
CREATE MATERIALIZED VIEW idata_1h_sc_<S> WITH (timescaledb.continuous) AS
  SELECT item_id,
         time_bucket(interval '1 hour', idata_timestamp) AS bucket_start,
         min(idata_value::double precision) AS min_value,
         max(idata_value::double precision) AS max_value,
         avg(idata_value::double precision) AS avg_value,
         count(*) AS sample_count
  FROM idata_sc_<S>
  WHERE idata_value ~ '^[-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?$'
  GROUP BY item_id, time_bucket(interval '1 hour', idata_timestamp)
  WITH NO DATA;

CREATE MATERIALIZED VIEW idata_1d_sc_<S> WITH (timescaledb.continuous) AS
  SELECT item_id,
         time_bucket(interval '1 day', bucket_start) AS bucket_start,
         min(min_value) AS min_value,
         max(max_value) AS max_value,
         sum(avg_value * sample_count) / sum(sample_count) AS avg_value,
         sum(sample_count) AS sample_count
  FROM idata_1h_sc_<S>
  GROUP BY item_id, time_bucket(interval '1 day', bucket_start)
  WITH NO DATA;
```

Refresh and retention policies are attached at server startup by `ReconcileTSDBAggregation()` when `Aggregation.Enabled = 1` (using `add_continuous_aggregate_policy` / `add_retention_policy`). The CAGGs themselves are created `WITH NO DATA` and produce nothing while the master switch is off, so they are cheap to keep around when the feature is disabled.

### Late-data correctness

**Problem**: NetXMS agents queue data during connectivity outages. A site offline for 12 hours sends chronologically-ordered cached data on reconnect. Without special handling, aggregate rows for the outage window would remain missing after rollup advances past them.

**Non-TSDB solution**: per-DCI `aggregation_watermark` column on `items`.

- Hot-path raw insert in `datacoll.cpp`: if incoming timestamp < current watermark, atomic `UPDATE items SET aggregation_watermark = LEAST(aggregation_watermark, ?) WHERE item_id = ?`
- Chronological cached data means one `UPDATE` per outage (first post-reconnect insert), not per row
- Next scheduled rollup processes from new watermark; UPSERT handles overwriting any previously-empty bucket

**TSDB solution**: native CAGG invalidation log. `start_offset` of the refresh policy (default 30 days) caps the outage window that can be recovered.

## Query Dispatch

Extend the server-side DCI history query path with two parameters:
- `tier` — `auto | raw | hourly | daily` (default `auto`)
- `function` — `avg | min | max | minmax` (default `avg`)

### Auto-select heuristic

1. String/table DCI, or DCI with no aggregates populated → **raw**
2. `(end − start) / pollingInterval ≤ MaxAutoSelectPoints` (default 5000) → **raw**
3. `(end − start) / 3600 ≤ MaxAutoSelectPoints` and range fits within hourly retention → **hourly**
4. Else → **daily**

Fallback: if the chosen tier has no data for the range (e.g., aggregation recently enabled), dispatcher transparently reads a denser tier and serves what's available. Gaps remain gaps.

### Band-graph support

`function = minmax` returns both `min_value` and `max_value` columns in a single response — one round-trip instead of two.

### API surface

- **NXCP** — extend `CMD_GET_DCI_DATA` with `VID_DCI_TIER` and `VID_DCI_AGG_FUNCTION`; the response carries `VID_DCI_TIER_USED` so clients know which tier the dispatcher actually served. Legacy clients unaffected (absent fields → auto + avg; missing response field → raw).
- **Java client** — new `NXCSession.getCollectedData()` overload with `DciTier` and `DciAggregationFunction` enums; legacy overloads delegate to `(AUTO, AVG)`. `DataSeries.getTierServed()` exposes the served tier.
- **NXSL** — `GetDCIValues(node, dciId, startTime, endTime?, rawValue?, tier?, function?)`. Tier and function are optional strings appended after the existing `rawValue` argument; same shape for `GetDCIValuesEx`. Tier ∈ `{auto, raw, hourly, daily}`, function ∈ `{avg, min, max}` — `minmax` is not exposed in NXSL (no array-of-pairs return shape).

## DCI Recalculation

NetXMS lets administrators recalculate DCI values after changing the transformation script or delta calculation type — the existing recalculation task reads `raw_value` from `idata_<N>` and rewrites `idata_value` using the new transformation. After recalculation, any previously-computed aggregates are stale because they were derived from the old `idata_value`s.

**Constraint**: raw retention is typically shorter than aggregate retention (e.g., raw 90 days, hourly aggregates 1 year). Aggregate buckets older than the earliest retained raw sample cannot be recomputed because the source data is gone.

**Approach**: recompute aggregates where possible, preserve older aggregates as-is.

### Non-TSDB

1. Run the existing recalculation (rewrites `idata_value` in `idata_<N>`)
2. After recalc completes: `UPDATE items SET aggregation_watermark = (SELECT MIN(idata_timestamp) FROM idata_<N>) WHERE item_id = ?`
3. Next scheduled rollup re-aggregates the range `[earliest_raw, now − close_window]`; UPSERT overwrites existing aggregate rows
4. Aggregates with `bucket_start < earliest_raw` remain untouched (they continue to reflect the previous transformation)

### TSDB

1. Run the existing recalculation
2. Explicit CAGG refresh over the affected range (the automatic refresh policy's `start_offset` may be shorter than raw retention):
   ```sql
   CALL refresh_continuous_aggregate('idata_1h', <earliest_raw_ts>, now());
   CALL refresh_continuous_aggregate('idata_1d', <earliest_raw_ts>, now());
   ```
3. Buckets older than `earliest_raw_ts` remain as-is

### User-facing behavior

The recalculation confirmation dialog surfaces the scope of recomputation:

> Recalculating DCI values using the updated transformation.
> Aggregates will be recomputed for the period where raw data is retained
> (from {earliest_raw_ts} to present). Older aggregates will continue to reflect
> the previous transformation and cannot be updated.

### Concurrency

Watermark reset is a single atomic `UPDATE`; if a rollup pass is running concurrently it picks up the new watermark on its next iteration. Existing serialization around the recalculation task is sufficient — no new locks required.

### Edge cases

- **Aggregation disabled for the DCI**: recalc runs normally; no aggregate-side action
- **Aggregation eligible but never enabled until now**: same as "enable on existing" — backfill via watermark init, which recalc's watermark reset effectively performs anyway

## Configuration

All new server configuration variables live under `DataCollection.Aggregation.*`:

| Variable | Type | Default | Purpose |
|---|---|---|---|
| `Enabled` | boolean | false | Master switch |
| `DefaultHourlyRetentionTime` | days | 365 | Global hourly retention |
| `DefaultDailyRetentionTime` | days | 1825 | Global daily retention |
| `HourlyCloseWindow` | seconds | 300 | Lag before rolling up a closed hour |
| `DailyCloseWindow` | seconds | 1800 | Lag before rolling up a closed day |
| `BackfillOnEnable` | boolean | true | Initialize watermarks to earliest raw timestamp |
| `MaxAutoSelectPoints` | int | 5000 | Auto-dispatch threshold |
| `TSDB.RefreshStartOffset` | days | 30 | CAGG refresh lookback (caps recoverable outage) |
| `TSDB.RefreshScheduleInterval` | seconds | 600 | CAGG refresh cadence |

## User Interface

### DCI properties (nxmc desktop + web)

New section in the **General** tab:
- **Aggregation mode**: *Default (inherit) / Enabled / Disabled*
- **Hourly retention**: *Default / Override (days)*
- **Daily retention**: *Default / Override (days)*

Info lines shown when DCI is ineligible:
- *"Aggregation not applicable: data type is STRING"*
- *"Aggregation not applicable: this is a table DCI"*
- *"Hourly aggregation not applicable: polling interval exceeds 30 minutes"*

On TSDB backend, an info note explains that per-DCI retention overrides are not honored and disable is query-level only.

### Graph widget

- **Data resolution**: *Auto / Raw / Hourly / Daily* dropdown
- **Show min/max band** checkbox — enables `function=minmax`; renders filled band with avg line
- Legend includes tier when not raw (e.g., *"Interface Load (hourly avg)"*)
- Hover tooltip shows `sample_count` when viewing aggregates

### Server configuration screen

New "Aggregation" section under Data Collection. TSDB-specific rows visible only on TSDB backend.

## Upgrade Path

Schema migration in the next available upgrade slot:

1. Add to `items`:
   - `aggregation_mode CHAR(1)` — `I` (inherit, default) / `E` (enabled) / `D` (disabled)
   - `hourly_retention INT NULL` — per-DCI override
   - `daily_retention INT NULL` — per-DCI override
   - `aggregation_watermark SQL_INT64 NULL` — non-TSDB only
2. Insert `DataCollection.Aggregation.*` config variables via `CreateConfigParam()`
3. Register the two scheduled tasks
4. On TSDB: create `idata_1h` and `idata_1d` hypertables, CAGGs, refresh and retention policies
5. Bump `DB_SCHEMA_VERSION_MINOR` in `src/server/include/netxmsdb.h`

Non-TSDB per-object aggregate tables (`idata_1h_<N>`, `idata_1d_<N>`) are **not** created during migration — they are created lazily on first rollup for that object, matching existing `idata_<N>` lifecycle.

### Enable-on-existing

When the admin flips the master switch:
- If `BackfillOnEnable = true`:
  - Non-TSDB: `UPDATE items SET aggregation_watermark = (SELECT MIN(idata_timestamp) FROM idata_<N> ...)` per eligible DCI
  - TSDB: `CALL refresh_continuous_aggregate('idata_1h', NULL, now())`, then same for `idata_1d`
- If false: watermarks stay null; aggregation starts from next bucket close

No server restart required.

## Housekeeper Integration

- **Non-TSDB retention enforcement**: new housekeeper task walks each object's aggregate tables and deletes rows older than retention. Per-DCI overrides honored via partitioned `WHERE item_id IN (...)` deletes.
- **TSDB retention enforcement**: native chunk-drop via `add_retention_policy` — no server-side code.
- **Object deletion**: the existing cleanup path is extended with `DROP TABLE IF EXISTS idata_1h_<N>, idata_1d_<N>` (non-TSDB) or `DELETE ... WHERE item_id = ?` on the aggregate hypertables (TSDB).

## Known TSDB Tradeoffs

Documented in the admin guide:
- **Per-DCI aggregation disable** is a query-time filter only — aggregates are still produced on disk for all DCIs
- **Per-DCI aggregate retention is not honored** — retention is driven by one chunk-drop policy per `idata_1h_sc_<S>` / `idata_1d_sc_<S>`. Granularity is per-storage-class, which matches the granularity users opted into when assigning a DCI to a storage class.
- **Per-DCI retention longer than the storage-class default is impossible** — same reason

Same class of tradeoff as the existing raw storage-class model, which approximates per-DCI retention via the `idata_sc_*` buckets.

## Compatibility

All schema changes are additive. NXCP gains optional fields (legacy clients ignore them). Java client and NXSL gain new overloads that preserve existing signatures. No breaking changes.

## Open Items for Implementation

- Housekeeper task naming and integration point for non-TSDB aggregate retention deletes
- Precise UI field labels and help text (en/de/cs/ru/es/pt translations)
- Whether to expose aggregate `sample_count` in the API as a distinct response column or only via tooltip
