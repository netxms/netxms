# DCI Anomaly Detection

## Overview

Anomaly detection tells when a collected value of a single-value DCI does not look like the values
that DCI normally has at that time. It needs no thresholds from the administrator: the server learns
a baseline from the DCI's own history and compares every new value with it.

The design goals are:

- **Nothing expensive on the poll path.** A value is checked against an in-memory model; there is no
  database access and no model fitting under the DCI lock.
- **Works without an LLM.** All detection is statistical. An LLM, when configured, only suggests
  settings.
- **Explainable result.** Every reported anomaly has a type, a score and the range that was expected.

Anomaly detection is enabled per DCI. It produces an event only through a threshold with function
`anomaly` (`F_ANOMALY`); without such threshold the result is visible in the last values view, in
the WebAPI and to AI tools, but nothing is raised.

Only numeric DCIs are supported. Collected values must be stored (DCIs with storage disabled cannot
be trained).

## Architecture

```
  history (idata / hourly aggregates)
            |
            v
  +-------------------+     background, once a day and on request
  | training          |     anomaly_profile.cpp
  | AnomalyModel::train
  +-------------------+
            |  AnomalyModel (immutable, shared_ptr)
            v
  +-------------------+     on each collected value, under DCI lock
  | scoring           |     DCItem::checkForAnomaly -> ScoreAnomalySample
  | AnomalyDetectorState
  +-------------------+
            |  AnomalyScore (type, score, expected range)
            v
   threshold with function "anomaly", last values, WebAPI, AI tools
```

| Part | Location |
|---|---|
| Engine (model, training, scoring), no server dependencies | `src/server/core/anomaly_engine.cpp`, `src/server/include/nxcore_anomaly.h` |
| Loading of history, training tasks, LLM tuning, replay | `src/server/core/anomaly_profile.cpp` |
| Per-DCI settings, model, detector state, persistence | `src/server/core/dcitem.cpp` |
| Unit tests | `tests/test-libnxcore/anomaly.cpp` |

Scoring depends on the model only through `ExpectedValue` (centre, scale, granularity, validity)
returned by `AnomalyModel::getExpectedValue()`, and settings record identifiers of the source of
expected values (`provider.id`, currently always `slot`) and of the level shift detector
(`detector.id`, currently always `cusum`). This is preparation for other sources of expected values
(for example models driven by other DCIs); there is no registry of providers yet.

## Baseline model

### Slots

The model has 168 slots: hour of day by day of week, **in server local time**. Time zone of the
monitored node is not considered. A value is always assigned to a slot by its own timestamp, not by
the time when it is processed.

Each slot has an expected level (centre). Days are the unit of evidence: values within one hour of
one day are not independent, so a slot centre is only as good as the number of distinct days behind
it. A centre is taken from the most specific granularity that has at least 6 days of data:

1. same hour, same day of week - only for days of week that were found to differ, see below;
2. same hour, weekdays or weekend;
3. same hour, all days;
4. all data.

A day of week gets its own baseline only when training shows that it actually differs from the
weekday/weekend level: at least 3 of its hours (out of at least 12 comparable ones) deviate by more
than 1.5 between-day standard deviations. Otherwise four or five instances of each weekday in the
training window would give a noisier baseline than the pooled one.

### Data used for training

- **Level** (slot centres, decision on individual days of week, spread used by level shift
  detection) is trained from hourly averages over up to 84 days. Hourly aggregates
  (see `DCI_Data_Aggregation.md`) are used when they are available for the DCI and cover at least
  the same period as collected values; otherwise hourly averages are calculated in memory from
  collected values. Averages are taken over hours of server local time, the same hours that slots
  cover. Hourly aggregates are aligned to UTC hours, so they are not used when the server time zone
  offset is not a whole number of hours (India, Nepal and similar): they would mix values of two
  adjacent slots.
- **Spread** of a single value around the centre, and of change between consecutive values, is
  trained from collected values over up to 30 days. Hourly averages hide variation within the hour
  and cannot provide it. Spread is pooled by hour and day type, then by hour, then globally, until
  there are at least 30 values.
- **The most recent 7 days are not used.** Otherwise a level shift that is in progress would be
  absorbed by the baseline before it can be reported. The lag is reduced while it would leave less
  than the minimum number of training days.
- Data older than the DCI's **baseline start** time is never used (see *Starting new baseline*).

All statistics are robust: centre is a median, spread is median absolute deviation scaled to
standard deviation (x 1.4826). Spread has a floor (relative to the level, and not less than
resolution of the series), so a flat series does not make every change an anomaly.

### Mode

A model works either on the collected value or on its rate of change per second. With mode `auto`
the rate of change is selected for strongly monotone series (95% of hourly changes have the same
sign), such as counters collected without delta calculation or used disk space. Expected range is
reported in the same terms as the model works in.

### Learning state

A model trained on fewer distinct days than `minTrainingDays` (7 by default) is *learning*: values
are checked only against hard bounds (which do not need a model), and no other anomalies are
reported. This is the state of every DCI right after anomaly detection is enabled on a new DCI,
and after new baseline is started.

A DCI can stay in learning state because its model cannot be trained, or have a model trained on
less data than intended. Such problems are reported as *training issues* in the last values
(NXCP), in WebAPI (`trainingIssues` in `anomalyDetection` and in `anomaly` of the last value), and
in the debug log:

| Issue | Meaning |
|---|---|
| `storageDisabled` | Collected values are not stored; the model cannot be trained and training is not attempted |
| `dataReadError` | Collected values could not be read from the database at last training |
| `sparseSamples` | `sampleSaveInterval` is greater than 1, spread is trained from part of collected values |
| `shortRetention` | Retention time is shorter than 37 days, less data than intended is available for spread |

The console shows the first two in the anomaly column of last values instead of "Learning".
The DCI properties dialog (also for template DCIs) shows a warning next to the *Detect anomalies*
option when storage is disabled, retention time is shorter than 37 days, or not every value is
stored, using storage settings as currently selected on the *General* page. Default retention time
(`DataCollection.DefaultDCIRetentionTime`) is 40 days on new installations for this reason;
upgrade does not change the configured value.

## Detection

A value is checked in this order; the first match gives the anomaly type.

| Type | Check |
|---|---|
| `range` | Value is outside of hard bounds (if set), or its robust z-score against the slot, `(x - centre) / scale`, exceeds `sensitivity`. |
| `step` | Change from the previous value, compared with typical change between consecutive values, exceeds `sensitivity`. Catches a sharp jump that stays inside a wide slot range. A change that follows the expected change of level between two slots (start of business hours, scheduled job) is not reported: when two values belong to different slots, the change has to be unusual both by itself and after subtracting the difference between slot centres (the latter is compared with spread of such changes across slot boundaries, also learned from collected values). Only in value mode, and only when the interval between values is not much longer than usual. |
| `drift` | Sustained shift of level that stays inside the range of a single value (see below). |

All checks honour `direction` (both, up only, down only).

### Level shift (drift)

Deviations from slot centre are averaged per hour. In rate mode the hourly deviation is calculated
from the rate of change between mean values of two consecutive hours, the same way hourly rates
are calculated for training; mean of rates between individual values would be dominated by noise
of the first and the last value of the hour. When the hour changes, the hourly deviation is
standardized and fed into a two-sided cumulative sum (CUSUM):

```
S+ = max(0, S+ + z - k)        S- = max(0, S- - z - k)
```

Drift is reported while either sum is above `h`. The time when the sum last left zero is kept as the
estimated onset of the shift.

- `z` is limited to +/-4, so one extreme spike does not register as drift.
- The sums cannot grow more than 2 above `h`, so the indication clears within hours after the level
  returns to normal.
- The scale used for standardization is the larger of two estimates: spread of hourly mean
  deviation, and spread of *daily* mean deviation scaled to one hour. For independent hourly
  deviations both are the same. On real data some days are simply busier than others, deviations of
  consecutive hours are correlated, and the first estimate alone makes ordinary busy days look like
  level shifts.

Defaults are `k = 1`, `h = 5`: shift of level by two standard deviations of hourly mean is reported
in about 6 hours, by three - in 2 to 3 hours.

### Scores

| Type | Score |
|---|---|
| `range` | z-score of the value |
| `step` | z-score of the change |
| `drift` | cumulative sum (negative for downward shift) |

## Settings

Per-DCI settings are set by the user and are never changed by training.

| Setting | Default | Meaning |
|---|---|---|
| `sensitivity` | 3.5 | Largest deviation, in standard deviations, that is still normal. Lower value is more sensitive. |
| `direction` | `both` | Direction of deviations that are reported. |
| `mode` | `auto` | `value`, `rate`, or selected by training. |
| `minTrainingDays` | 7 | Days of data needed before values are checked. |
| `hardBounds.min`, `hardBounds.max` | not set | Limits the value cannot cross under normal conditions. |
| `detector.k` | 1.0 | Slack of level shift detector. |
| `detector.h` | 5.0 | Decision threshold of level shift detector. |

Two DCI flags control the feature:

| Flag | Value | Meaning |
|---|---|---|
| `DCF_DETECT_ANOMALIES` | 0x0001 | Anomaly detection is enabled |
| `DCF_ANOMALY_LLM_TUNING` | 0x0200 | Settings are tuned by LLM; implies the first flag |

With default sensitivity and normally distributed noise about one value in 2000 is reported by the
range check alone. Sample count of the anomaly threshold (number of consecutive anomalous values
needed to activate it) is the intended way to ignore single-value deviations.

### LLM tuning

When LLM tuning is enabled for a DCI and an AI provider is configured, after a model is trained the
server sends a summary of training data and of the model (distribution of values, levels by hour,
spread, mode) together
with DCI description, units and AI hint (`items.ai_hint`) to the LLM and asks for sensitivity,
direction, mode and hard bounds. The answer is validated (sensitivity is limited to 2 .. 8) and
stored with the model as `tuning`.

- While the flag is set, tuned sensitivity, direction and hard bounds take precedence over
  configured ones; tuned mode is used only if configured mode is `auto`. Hard bounds are replaced
  one by one: a configured bound stays in effect if LLM did not suggest one for that side, and
  suggested bounds are ignored if they contradict a configured one.
- Tuning is requested in background after a model is trained, one request at a time and outside
  of training queues. If LLM suggests another mode, the model is trained again.
- Tuning is reused for 30 days, and requested again on regeneration requested by user.
- Configured settings are not modified. Turning the flag off returns to them.
- Without a provider, or if the request fails, detection runs on configured settings.

## Training schedule

- Scheduled task `System.RegenerateAnomalyProfiles` (daily, 12:00 by default) trains models of all
  DCIs with anomaly detection enabled. Targets are collected first; training runs on the main
  thread pool with at most 4 models trained in parallel.
- Training reads collected values of the training range (30 days) and, when available, hourly
  aggregates. Values collected within the training lag (last 7 days) are read only when there is
  less history than `minTrainingDays`, as they are not used otherwise.
- Scheduled training replaces the model and keeps level shift detector state, unless the mode of
  the model has changed.
- If a new model cannot be trained (no data, database error), the existing one is kept.
- A DCI requests training of its own model when a value is collected and the DCI has no model, the
  model is older than two days (scheduled training did not update it: detection was just enabled,
  or the server was down), or the model was trained for a detection mode other than the configured
  one (the model records the mode training was requested with). So enabling anomaly detection or
  changing the mode takes effect on the next collected value and does not wait for scheduled
  training, whatever way the configuration was changed. Baseline start is not changed. Such
  requests are made at most once per hour per DCI.
- When anomaly detection is disabled for a DCI, its model and level shift detector state are
  dropped on the next collected value, so they are not reused if detection is enabled again later.

### Starting new baseline

Regeneration requested by user (console menu item *Regenerate anomaly profile*, NXCP command
`CMD_REGENERATE_ANOMALY_PROFILE`, WebAPI `POST .../regenerate-anomaly-profile`, NXSL method
`DCI::generateAnomalyProfile()`) is how an operator accepts a new level as normal:

- baseline start time is set to the onset of level shift currently being reported, or to current
  time if there is none;
- existing model and detector state are discarded, and a new model is trained from data collected
  after baseline start;
- until enough days are collected, the DCI is in learning state.

If the DCI has no model yet, baseline start is not moved.

## Persistence and high availability

| Column | Content |
|---|---|
| `items.anomaly_settings` | Settings as JSON; NULL when all are default |
| `items.anomaly_baseline_start` | Baseline start time |
| `items.anomaly_profile` | Trained model (format version 2) with `tuning` |
| `items.anomaly_profile_timestamp` | Time of last training |
| `raw_dci_values.anomaly_state` | Level shift detector sums and their onset times, as JSON |

Detector state is written when an hourly bucket closes, only if drift sums have changed (most of
the time they stay at zero), and when new baseline is started. Sum of the hour in progress is not saved, so a restart can delay a drift
report by up to an hour and cannot cause a false one.

In an HA cluster both nodes share the database and no replication channel is involved. A node that
becomes active reloads models, baseline start times and detector state of all DCIs in one pass
(`ReloadAnomalyProfiles()`, called from the activation thread after threshold states are reloaded).
DCIs that have no profile in the database have theirs cleared in memory.

## Interfaces

### Events

Activation and repeat events of an anomaly threshold have additional named parameters:
`anomalyType`, `anomalyScore`, `anomalyExpectedLow`, `anomalyExpectedHigh`, `anomalySlot`,
`anomalyDirection`, `anomalyMode`.

### NXCP

- `VID_ANOMALY_SETTINGS` in DCI configuration: settings as JSON document.
- Last value summary record, fixed offsets from record base, present only when anomaly detection is
  enabled: 39 - anomaly type, 40 - score, 41 and 42 - expected range (40 to 42 only if the value was
  checked), 43 - learning, 44 - model works on rate of change.
- `CMD_REGENERATE_ANOMALY_PROFILE` (`VID_OBJECT_ID`, `VID_DCI_ID`); requires modify access to the object.

### WebAPI

- DCI configuration has `anomalyDetection` object: settings, `enabled`, `llmTuning`, and read-only
  `baselineStart`, `tuning` and `model`. Updates are merged into current settings.
- Current value has `anomaly` object: `learning`, `type`, `mode`, `score`, `expectedLow`,
  `expectedHigh`, `slot`.
- `POST /v1/objects/{object-id}/data-collection/configuration/{dci-id}/regenerate-anomaly-profile`.

See schemas `DCIAnomalyDetection` and `DCIAnomalyState` in `src/server/webapi/openapi.yaml`.

### AI tools

`create-metric` and `edit-metric` accept `anomalyDetection` = `none`, `enabled` or `tuned`;
`get-metric-details` reports it together with settings. The operational overview tool reports type,
score and expected range for DCIs with anomalies.

### Management console

- *Thresholds* page of DCI properties: enable detection, enable LLM tuning, sensitivity, direction,
  mode. Other settings are available through WebAPI only.
- Last values view: column *Anomaly detection* (learning, anomaly type, expected range).
- *Regenerate anomaly profile* in *Actions* submenu of data collection views.

### Configuration export

Settings are exported as `anomalySettings`. In configuration files and databases created before
this design, the LLM flag alone enabled anomaly detection; import and database upgrade set the
"enabled" flag for such DCIs.

## Replay

NXSL function `DetectAnomalies(object, dciId, timeFrom, [timeTo], [labels])` and method
`DCI::detectAnomalies(timeFrom, [timeTo], [labels])` replay recorded history through anomaly
detection, to see what would have been reported with current settings of the DCI.

- For each day of the period a model is trained as scheduled training would have trained it at the
  start of that day - same selection of data, same lag - and values of that day are checked with
  it. Detector state is carried from day to day.
- Current settings of the DCI are used, including LLM tuning as it is now. Baseline start time is
  ignored. Anomaly detection does not have to be enabled for the DCI.
- Nothing is changed in the DCI.
- Replayed period cannot be longer than 366 days.

Result is a hash map:

| Element | Content |
|---|---|
| `samples`, `checkedSamples`, `anomalousSamples`, `trainings` | Totals |
| `days` | Per day: `start`, `samples`, `anomalousSamples`, `learning` |
| `episodes` | Sequences of consecutive anomalous values: `start`, `end`, `type` (of first value), `samples`, and `score`, `value`, `expectedLow`, `expectedHigh`, `slot` of the value with highest score; `labelled` |
| `episodesByType` | Number of episodes by type |
| `evaluation` | Only when `labels` are given, see below |

`labels` is an array of time ranges (`[start, end]`, UNIX timestamps) when anomalies are expected -
known incidents. With labels the result has `evaluation`:

| Element | Content |
|---|---|
| `events` | Per labelled range: `detected`, and `delay` in seconds from its start to the first anomalous value |
| `recall` | Share of labelled ranges that were detected |
| `precision` | Share of episodes that overlap a labelled range |
| `detectionDelay` | Average delay for detected ranges |
| `falseAlarms`, `falseAlarmsByType` | Episodes outside of labelled ranges |
| `falseAlarmsPerWeek` | False alarms per week of checked data |

Example:

```
r = DetectAnomalies($node, 931, time() - 86400 * 28, null, [[1794312000, 1794313800]]);
println(r.evaluation.recall .. " " .. r.evaluation.falseAlarmsPerWeek);
for(e : r.episodes)
   println(strftime("%d.%m %H:%M", e.start) .. " " .. e.type .. " " .. e.score .. " " .. e.slot);
```

## Isolation forest

`ad.cpp` keeps a univariate isolation forest (`FitIsolationForest`, `PredictIsolationForest`). It
is not used for checking collected values; it is retained for multivariate detection over several
DCIs of one object.

## Limitations

- Slots are in server local time.
- A DCI with fewer than 7 days of history reports only values outside of hard bounds.
- With `sampleSaveInterval` greater than 1 only part of collected values is stored, and spread is
  trained from that part.
- When retention time is shorter than 37 days, less data than intended is available for spread.
- Holidays and other calendar exceptions are not known to the model.
- Each DCI is analyzed on its own.
