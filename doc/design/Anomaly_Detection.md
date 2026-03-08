# AI-Powered Anomaly Detection for NetXMS

Server-integrated anomaly detection using LLM-generated profiles for intelligent, metric-aware alerting.

## Overview

This system combines statistical analysis with LLM intelligence to create efficient, customized anomaly detection:

- **C++ computes statistics** from historical DCI data
- **LLM tunes detection parameters** based on metric semantics and patterns
- **C++ runtime engine** evaluates every sample without LLM calls

The result is anomaly detection that understands context ("CPU spikes during backup are normal") while running at full collection speed.

## Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                      PROFILE GENERATION (scheduled/on-demand)           │
│                                                                         │
│  ┌──────────────┐    ┌──────────────────┐    ┌───────────────────────┐ │
│  │ Historical   │───▶│ C++ Statistics   │───▶│ LLM (via existing     │ │
│  │ DCI Data     │    │ Calculator       │    │ AI infrastructure)    │ │
│  └──────────────┘    └──────────────────┘    └───────────┬───────────┘ │
│                                                          │             │
│                                                          ▼             │
│  ┌──────────────┐    ┌──────────────────┐    ┌───────────────────────┐ │
│  │ anomaly_     │◀───│ Merge stats +    │◀───│ Tuning parameters     │ │
│  │ profile JSON │    │ LLM parameters   │    │ (JSON response)       │ │
│  └──────────────┘    └──────────────────┘    └───────────────────────┘ │
└─────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────┐
│                      RUNTIME DETECTION (every DCI poll)                 │
│                                                                         │
│  ┌──────────────┐    ┌──────────────────┐    ┌───────────────────────┐ │
│  │ New Value    │───▶│ C++ Anomaly      │───▶│ m_anomalyDetected +   │ │
│  │              │    │ Detector         │    │ source indicator      │ │
│  └──────────────┘    │ (profile-based)  │    └───────────┬───────────┘ │
│                      └──────────────────┘                │             │
│                                              ┌───────────▼───────────┐ │
│                                              │ F_ANOMALY threshold   │ │
│                                              │ → Event generation    │ │
│                                              └───────────────────────┘ │
└─────────────────────────────────────────────────────────────────────────┘
```

## Detection Methods

The profile-based detector implements five detection methods, evaluated in order (short-circuit on first anomaly):

### 1. Hard Bounds

Absolute limits that should never be exceeded.

```
if value < min OR value > max → ANOMALY
```

### 2. Seasonal Deviation

Time-aware statistical thresholds accounting for daily and weekly patterns.

```
hour = currentHour()
baseline = hourlyBaselines[hour]
weekdayFactor = weekdayFactors[dayOfWeek()]
deviation = |value - baseline.mean|
threshold = sensitivity × baseline.stddev × weekdayFactor

if deviation > threshold → ANOMALY
```

### 3. Rate of Change

Detects sudden spikes or drops between consecutive samples.

```
delta = |currentValue - previousValue|
elapsedMinutes = (currentTime - previousTime) / 60

if delta > maxRatePerMinute × elapsedMinutes → ANOMALY
```

### 4. Sustained High

Detects prolonged periods above a threshold.

```
if value > threshold for consecutive durationMinutes → ANOMALY
```

### 5. Sudden Drop

Detects unexpected drops from recent baseline.

```
recentAverage = average(recent N samples)
dropPercent = (recentAverage - value) / recentAverage × 100

if dropPercent > threshold → ANOMALY
```

## Profile JSON Structure

Stored in `items.anomaly_profile` column:

```json
{
  "version": 1,
  "generatedAt": 1706000000,
  "dataRange": {
    "start": 1703000000,
    "end": 1706000000
  },

  "hourlyBaselines": [
    { "hour": 0, "mean": 12.5, "stddev": 3.2 },
    { "hour": 1, "mean": 11.8, "stddev": 2.9 },
    ...
  ],

  "weekdayFactors": {
    "monday": 1.0,
    "tuesday": 1.0,
    "wednesday": 1.0,
    "thursday": 1.0,
    "friday": 1.0,
    "saturday": 0.7,
    "sunday": 0.6
  },

  "hardBounds": {
    "enabled": true,
    "min": 0,
    "max": 100
  },

  "seasonalDetection": {
    "enabled": true,
    "sensitivity": 2.5,
    "useWeekdayFactors": true
  },

  "changeDetection": {
    "enabled": true,
    "maxRatePerMinute": 15.0
  },

  "sustainedHighDetection": {
    "enabled": true,
    "threshold": 85,
    "durationMinutes": 10
  },

  "suddenDropDetection": {
    "enabled": false
  }
}
```

## AI Hints

Administrators can provide context to guide profile generation:

### Node-Level Hints

Apply to all DCIs on a node:

- "Development server - expect irregular patterns"
- "Batch processing server - high CPU at 2-4am is normal"
- "Maintenance window: Sunday 3-5am"

### DCI-Level Hints

Apply to specific metrics:

- "Spikes during hourly backup jobs are expected"
- "Critical metric - prefer false positives over missed anomalies"
- "Counter resets at midnight are normal"

Hints are stored in `nodes.ai_hint` and `items.ai_hint` columns.

## Detection Flags

Two independent flags in `items.flags`:

| Flag | Description |
|------|-------------|
| `DCF_DETECT_ANOMALIES_IFOREST` | Isolation forest detection (existing) |
| `DCF_DETECT_ANOMALIES_AI` | AI profile-based detection (new) |

Both can be enabled simultaneously. Anomaly triggers if either detector fires. Events include source indicator.

## Profile Generation

### Workflow

1. **Collect historical data** - Query 1-3 months from idata tables
2. **Compute statistics in C++:**
   - Hourly means and standard deviations
   - Weekday vs weekend patterns
   - Min/max/percentiles (p1, p5, p50, p95, p99)
   - Rate of change statistics
   - Sample outliers for context
3. **Build LLM prompt** - Statistics + metric context + AI hints
4. **LLM returns tuning parameters** - Which checks to enable, sensitivity values
5. **Merge and store** - Combine computed baselines with LLM parameters

### Scheduling

Profile generation uses the existing scheduled task mechanism:

- **System task**: `System.RegenerateAnomalyProfiles`
- **Default schedule**: Weekly (configurable by admin)
- **Retry logic**: On LLM failure, schedule one-time retry with counter
- **Cooldown**: Skip recently regenerated DCIs

### On-Demand Generation

- Console context menu: "Generate Anomaly Profile"
- NXSL function: `GenerateAnomalyProfile(dci)`
- Execution: Asynchronous (non-blocking)

### Requirements

Profile generation requires:

- Minimum 2 weeks of historical data
- AI profile detection flag enabled on DCI
- LLM service configured and available

DCIs without a profile have AI anomaly detection disabled until first successful generation.

## Database Schema

### Items Table - New Columns

| Column | Type | Description |
|--------|------|-------------|
| `anomaly_profile` | TEXT | JSON profile |
| `anomaly_profile_timestamp` | INTEGER | Generation timestamp |
| `ai_hint` | VARCHAR(2000) | DCI-level AI hint |

### Nodes Table - New Column

| Column | Type | Description |
|--------|------|-------------|
| `ai_hint` | VARCHAR(2000) | Node-level AI hint |

### Items Flags - New Bit

| Bit | Constant | Description |
|-----|----------|-------------|
| (new) | `DCF_DETECT_ANOMALIES_AI` | Enable AI profile detection |

## LLM Integration

Uses existing AI assistant infrastructure (`ai_core.cpp`):

- Same LLM service URL and authentication
- Same thread pool for background execution
- Prompt template in `share/prompts/` or embedded

### Prompt Structure

```
You are configuring anomaly detection for a network monitoring metric.

METRIC INFORMATION:
- Name: {dci_name}
- Description: {dci_description}
- Unit: {unit}
- Polling interval: {interval} seconds
- Node: {node_name} ({node_type})

NODE CONTEXT: {node_ai_hint}
METRIC CONTEXT: {dci_ai_hint}

COMPUTED STATISTICS ({data_days} days):
{statistics_json}

Return anomaly detection parameters as JSON matching this schema:
{response_schema}

Guidelines:
- Consider the metric type and expected behavior
- Account for visible time-of-day and weekly patterns
- Incorporate any provided context hints
- Prefer fewer false positives for production stability
```

### LLM Response Format

```json
{
  "hardBounds": { "enabled": true, "min": 0, "max": 100 },
  "seasonalDetection": { "enabled": true, "sensitivity": 2.5, "useWeekdayFactors": true },
  "changeDetection": { "enabled": true, "maxRatePerMinute": 15.0 },
  "sustainedHighDetection": { "enabled": true, "threshold": 85, "durationMinutes": 10 },
  "suddenDropDetection": { "enabled": false },
  "reasoning": "Brief explanation of parameter choices..."
}
```

## Console UI

### DCI Properties Dialog

- Checkbox: "Enable AI-based anomaly detection"
- Text field: "AI Hint" (optional)
- Status display: Profile timestamp or "Not generated"

### Node Properties Dialog

- Text field: "AI Hint" (optional)

### DCI Context Menu

- Action: "Generate Anomaly Profile" (async)

## Error Handling

| Scenario | Behavior |
|----------|----------|
| LLM unreachable | Log warning, schedule retry |
| Invalid LLM response | Log error, keep existing profile |
| Insufficient history | Skip DCI, mark as "pending" |
| Unreasonable values | Apply sanity bounds, log warning |
| No profile exists | AI detection disabled until generated |

## Performance Considerations

### Runtime Detection

- Profile parsed once on load, cached in memory
- No database queries during detection
- No LLM calls during detection
- Short-circuit evaluation (stop on first anomaly)
- Typical execution: < 1ms per sample

### Profile Generation

- Batch processing with rate limiting
- Skip DCIs in cooldown period
- Background thread pool execution
- Non-blocking for console operations

## Comparison: Isolation Forest vs AI Profile

| Aspect | Isolation Forest | AI Profile |
|--------|------------------|------------|
| Training | Automatic | LLM-guided |
| Time awareness | No | Yes (hourly/weekly) |
| Semantic context | No | Yes (via hints) |
| Customization | Limited | Per-metric tuning |
| CPU overhead | Higher | Lower |
| Interpretability | Low (score) | High (explicit rules) |

Both methods can be enabled simultaneously for defense in depth.

## Future Enhancements

- **Multi-metric correlation**: Detect anomalies across related metrics
- **Feedback loop**: Learn from operator actions (acknowledged vs dismissed)
- **Automated tuning**: Adjust sensitivity based on false positive feedback
- **Profile sharing**: Apply one profile to multiple similar DCIs
- **Anomaly explanation**: LLM-generated explanations when anomalies fire
