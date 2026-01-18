# AI Log Analysis Skill Design Document

## Overview

This document describes the design for a new AI assistant skill that provides comprehensive log analysis capabilities for NetXMS. The skill enables AI-powered searching, correlation, and pattern detection across multiple log sources.

### Goals

1. Provide unified access to all NetXMS log sources (syslog, Windows events, SNMP traps, system events)
2. Enable intelligent log correlation for root cause analysis
3. Detect patterns, anomalies, and bursts in log data
4. Support troubleshooting, security investigation, and compliance auditing use cases

### Non-Goals

1. Real-time log streaming (future enhancement)
2. Machine learning-based anomaly detection (uses statistical methods instead)
3. Log ingestion or parsing configuration

---

## Architecture

### Module Location

The log analysis skill will be implemented in the existing `aitools` module (GPL licensed, open source):

```
src/server/aitools/
├── main.cpp              # Skill registration (modified)
├── aitools.h             # Function declarations (modified)
├── logs.cpp              # NEW: Log analysis functions
└── skills/
    └── log-analysis.md   # NEW: Skill documentation
```

### Database Tables

| Table | Description | Key Columns |
|-------|-------------|-------------|
| `syslog` | Syslog messages from network devices and hosts | `msg_timestamp`, `source_object_id`, `facility`, `severity`, `msg_text` |
| `win_event_log` | Windows Event Log entries | `event_timestamp`, `node_id`, `log_name`, `event_source`, `event_code`, `message` |
| `snmp_trap_log` | SNMP traps from network devices | `trap_timestamp`, `object_id`, `trap_oid`, `trap_varlist` |
| `event_log` | NetXMS internal system events | `event_timestamp`, `event_source`, `event_severity`, `event_message`, `origin` |

### Access Control

| Log Type | Required System Access |
|----------|----------------------|
| Syslog | `SYSTEM_ACCESS_VIEW_SYSLOG` |
| Windows Events | `SYSTEM_ACCESS_VIEW_EVENT_LOG` |
| SNMP Traps | `SYSTEM_ACCESS_VIEW_TRAP_LOG` |
| System Events | `SYSTEM_ACCESS_VIEW_EVENT_LOG` |

Additionally, per-object access rights (`OBJECT_ACCESS_READ`) are enforced when filtering by source object.

---

## Function Specifications

### 1. search-syslog

Search syslog messages with flexible filtering.

#### Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `object` | string | No | - | Node name or ID to filter by source |
| `time_from` | string | No | -60 min | Start time (ISO 8601 or relative minutes) |
| `time_to` | string | No | now | End time (ISO 8601 or relative) |
| `severity` | int/array | No | - | Syslog severity (0-7) or array |
| `facility` | int/array | No | - | Syslog facility (0-23) or array |
| `text_pattern` | string | No | - | Case-insensitive substring search |
| `tag` | string | No | - | Syslog tag filter |
| `limit` | int | No | 100 | Max results (max: 1000) |

#### Response Schema

```json
{
  "count": 42,
  "messages": [
    {
      "id": 123456,
      "timestamp": "2026-01-07T10:30:15Z",
      "sourceObjectId": 100,
      "sourceObjectName": "router-core-01",
      "hostname": "router-core-01",
      "facility": 23,
      "facilityName": "local7",
      "severity": 3,
      "severityName": "error",
      "tag": "LINK",
      "message": "Interface GigabitEthernet0/1 changed state to down"
    }
  ]
}
```

#### SQL Query Pattern

```sql
SELECT msg_id, msg_timestamp, source_object_id, facility, severity,
       hostname, msg_tag, msg_text
FROM syslog
WHERE msg_timestamp BETWEEN ? AND ?
  AND source_object_id = ?           -- if object specified
  AND severity IN (?, ?, ?)          -- if severity filter
  AND facility IN (?, ?)             -- if facility filter
  AND LOWER(msg_text) LIKE LOWER('%pattern%')  -- if text_pattern
  AND msg_tag = ?                    -- if tag filter
ORDER BY msg_timestamp DESC
LIMIT ?
```

---

### 2. search-windows-events

Search Windows Event Log entries collected from monitored Windows hosts.

#### Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `object` | string | No | - | Node name or ID |
| `time_from` | string | No | -60 min | Start time |
| `time_to` | string | No | now | End time |
| `log_name` | string | No | - | Log name (System, Application, Security, etc.) |
| `event_source` | string | No | - | Event source filter |
| `event_code` | int/array | No | - | Event ID(s) |
| `severity` | int/array | No | - | Severity (1=Error, 2=Warning, 4=Info, 8=AuditSuccess, 16=AuditFailure) |
| `text_pattern` | string | No | - | Text search in message |
| `limit` | int | No | 100 | Max results |

#### Response Schema

```json
{
  "count": 15,
  "events": [
    {
      "id": 789012,
      "timestamp": "2026-01-07T09:45:00Z",
      "originTimestamp": "2026-01-07T09:44:58Z",
      "nodeId": 200,
      "nodeName": "srv-dc-01",
      "logName": "Security",
      "eventSource": "Microsoft-Windows-Security-Auditing",
      "eventCode": 4625,
      "severity": 16,
      "severityName": "AuditFailure",
      "message": "An account failed to log on..."
    }
  ]
}
```

---

### 3. search-snmp-traps

Search SNMP traps received from network devices.

#### Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `object` | string | No | - | Node name or ID |
| `time_from` | string | No | -60 min | Start time |
| `time_to` | string | No | now | End time |
| `trap_oid` | string | No | - | Trap OID or OID prefix |
| `ip_address` | string | No | - | Source IP filter |
| `varbind_pattern` | string | No | - | Text search in varbinds |
| `limit` | int | No | 100 | Max results |

#### Response Schema

```json
{
  "count": 8,
  "traps": [
    {
      "id": 345678,
      "timestamp": "2026-01-07T08:15:30Z",
      "objectId": 150,
      "objectName": "switch-access-02",
      "sourceIp": "10.1.2.50",
      "trapOid": "1.3.6.1.6.3.1.1.5.3",
      "trapOidName": "linkDown",
      "varbinds": [
        {"oid": "1.3.6.1.2.1.2.2.1.1", "value": "24"},
        {"oid": "1.3.6.1.2.1.2.2.1.2", "value": "GigabitEthernet0/24"}
      ]
    }
  ]
}
```

---

### 4. search-events

Search NetXMS internal system events.

#### Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `object` | string | No | - | Source object name or ID |
| `time_from` | string | No | -60 min | Start time |
| `time_to` | string | No | now | End time |
| `event_code` | int/string | No | - | Event code or name |
| `severity` | int/array | No | - | Severity (0=Normal to 4=Critical) |
| `origin` | string | No | - | Event origin (SYSTEM, SNMP, SYSLOG, AGENT, SCRIPT) |
| `tags` | string/array | No | - | Event tags filter |
| `text_pattern` | string | No | - | Text search in message |
| `limit` | int | No | 100 | Max results |

#### Response Schema

```json
{
  "count": 25,
  "events": [
    {
      "id": 567890,
      "timestamp": "2026-01-07T10:15:00Z",
      "sourceObjectId": 100,
      "sourceObjectName": "router-core-01",
      "eventCode": 28,
      "eventName": "SYS_NODE_DOWN",
      "severity": 4,
      "severityName": "Critical",
      "origin": "SYSTEM",
      "message": "Node router-core-01 is not responding",
      "tags": ["network", "availability"]
    }
  ]
}
```

---

### 5. get-log-statistics

Get log volume statistics for capacity planning and trend analysis.

#### Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `log_type` | string | Yes | - | Log type: syslog, windows_events, snmp_traps, events |
| `object` | string | No | - | Object to filter statistics |
| `time_from` | string | No | -24h | Start time |
| `time_to` | string | No | now | End time |
| `group_by` | string | No | hour | Grouping: hour, day, source, severity |

#### Response Schema

```json
{
  "logType": "syslog",
  "timeRange": {
    "from": "2026-01-06T10:00:00Z",
    "to": "2026-01-07T10:00:00Z"
  },
  "totalCount": 15420,
  "statistics": [
    {"period": "2026-01-07T09:00:00Z", "count": 542, "errorCount": 12, "warningCount": 45},
    {"period": "2026-01-07T08:00:00Z", "count": 623, "errorCount": 8, "warningCount": 38}
  ],
  "topSources": [
    {"objectId": 100, "objectName": "router-core-01", "count": 2341},
    {"objectId": 105, "objectName": "switch-dist-01", "count": 1892}
  ],
  "severityBreakdown": {
    "emergency": 0,
    "alert": 2,
    "critical": 15,
    "error": 234,
    "warning": 1456,
    "notice": 3421,
    "info": 9876,
    "debug": 416
  }
}
```

---

### 6. correlate-logs

Cross-correlate log entries across multiple sources within a time window.

#### Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `object` | string | Yes | - | Primary object for correlation |
| `time_from` | string | Yes | - | Correlation window start |
| `time_to` | string | Yes | - | Correlation window end |
| `include_neighbors` | bool | No | true | Include L2 neighbors |
| `log_types` | array | No | all | Log types to include |
| `limit_per_source` | int | No | 50 | Max entries per source |

#### Response Schema

```json
{
  "correlationWindow": {
    "from": "2026-01-07T10:00:00Z",
    "to": "2026-01-07T10:15:00Z"
  },
  "primaryObject": {
    "id": 100,
    "name": "router-core-01"
  },
  "includedObjects": [
    {"id": 100, "name": "router-core-01", "relationship": "primary"},
    {"id": 105, "name": "switch-dist-01", "relationship": "L2_neighbor"}
  ],
  "timeline": [
    {
      "timestamp": "2026-01-07T10:05:12Z",
      "type": "syslog",
      "objectId": 100,
      "objectName": "router-core-01",
      "severity": "error",
      "message": "Interface GigabitEthernet0/1 changed state to down"
    },
    {
      "timestamp": "2026-01-07T10:05:14Z",
      "type": "snmp_trap",
      "objectId": 100,
      "objectName": "router-core-01",
      "trapOid": "linkDown",
      "message": "ifIndex=5 ifDescr=GigabitEthernet0/1"
    }
  ],
  "summary": {
    "syslogCount": 12,
    "trapCount": 3,
    "eventCount": 5,
    "windowsEventCount": 0,
    "objectsInvolved": 4
  }
}
```

---

### 7. analyze-log-patterns

Analyze log patterns for anomaly detection and trend identification.

#### Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `object` | string | No | - | Object to analyze |
| `log_type` | string | Yes | - | Log type to analyze |
| `time_from` | string | No | -24h | Analysis start |
| `time_to` | string | No | now | Analysis end |
| `pattern_type` | string | No | all | Pattern type: frequency, burst, recurring, new |

#### Response Schema

```json
{
  "analysisWindow": {
    "from": "2026-01-06T10:00:00Z",
    "to": "2026-01-07T10:00:00Z"
  },
  "baseline": {
    "meanHourlyRate": 245.5,
    "stdDeviation": 42.3
  },
  "patterns": {
    "bursts": [
      {
        "timestamp": "2026-01-07T08:30:00Z",
        "endTime": "2026-01-07T08:35:00Z",
        "durationMinutes": 5,
        "messageCount": 1243,
        "normalRate": 45.0,
        "peakRate": 248,
        "multiplier": 5.5,
        "topPatterns": [
          {"pattern": "Connection timeout to <IP>", "count": 892},
          {"pattern": "Retrying connection to <IP>", "count": 351}
        ]
      }
    ],
    "recurring": [
      {
        "pattern": "Scheduled task <WORD> completed successfully",
        "occurrences": 24,
        "intervalSeconds": 3600,
        "intervalDescription": "1 hour",
        "regularity": 0.95,
        "firstSeen": "2026-01-06T11:00:00Z",
        "lastSeen": "2026-01-07T10:00:00Z"
      }
    ],
    "newPatterns": [
      {
        "pattern": "Certificate expiration warning for <WORD>",
        "firstSeen": "2026-01-07T06:15:00Z",
        "count": 12,
        "severity": 4,
        "severityName": "warning",
        "sources": [
          {"objectId": 200, "objectName": "srv-web-01", "count": 8},
          {"objectId": 201, "objectName": "srv-web-02", "count": 4}
        ]
      }
    ],
    "volumeAnomalies": [
      {
        "objectId": 200,
        "objectName": "srv-web-01",
        "normalHourlyVolume": 120.5,
        "currentHourlyVolume": 2340.0,
        "deviation": 19.4,
        "deviationDescription": "19.4x normal"
      }
    ]
  }
}
```

---

## Pattern Analysis Algorithms

### Message Normalization

Log messages are normalized by replacing variable parts with placeholders to identify patterns:

| Variable Type | Pattern | Placeholder |
|--------------|---------|-------------|
| IPv4 Address | `\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}` | `<IP>` |
| IPv6 Address | `[0-9a-fA-F:]{2,39}` | `<IP>` |
| MAC Address | `[0-9a-fA-F]{2}(:[0-9a-fA-F]{2}){5}` | `<MAC>` |
| Timestamp | `\d{4}-\d{2}-\d{2}[T ]\d{2}:\d{2}:\d{2}` | `<TIMESTAMP>` |
| UUID | `[0-9a-fA-F]{8}-...-[0-9a-fA-F]{12}` | `<UUID>` |
| Large Numbers | `\d{4,}` | `<NUM>` |
| File Paths | `/[a-zA-Z0-9_./\-]+` | `<PATH>` |
| Email | `[\w.+-]+@[\w.-]+\.\w+` | `<EMAIL>` |
| Hex Strings | `0x[0-9a-fA-F]+` or 32+ hex chars | `<HEX>` |

### Burst Detection

Bursts are detected using Z-score analysis on time-bucketed message counts:

```
Z-score = (bucket_count - baseline_mean) / baseline_stddev
```

**Thresholds:**
- Burst threshold: Z-score >= 3.0 (3 standard deviations above mean)
- Minimum duration: 1 bucket
- Baseline period: First 75% of analysis window

**Time Bucket Sizes:**

| Window Duration | Bucket Size |
|-----------------|-------------|
| <= 1 hour | 1 minute |
| <= 6 hours | 5 minutes |
| <= 24 hours | 15 minutes |
| <= 7 days | 1 hour |
| > 7 days | 1 day |

### Recurring Pattern Detection

Recurring patterns are identified by analyzing intervals between occurrences:

1. Collect timestamps for each normalized pattern
2. Calculate intervals between consecutive occurrences
3. Match intervals against known periods (with 10% tolerance)
4. Calculate regularity score = matching_intervals / total_intervals
5. Report patterns with regularity >= 0.7 (70%)

**Expected Intervals:**
- 1 minute, 5 minutes, 10 minutes, 15 minutes, 30 minutes
- 1 hour, 2 hours, 4 hours, 6 hours, 12 hours
- 1 day, 1 week

### New Pattern Detection

New patterns are messages that first appeared in the recent portion of the analysis window:

- "New" threshold: First seen in last 25% of time window
- Minimum occurrences: 3 (to filter noise)
- Sorted by count (descending)

### Volume Anomaly Detection

Per-source volume anomalies compare recent rate to historical baseline:

```
deviation = current_hourly_rate / baseline_hourly_rate
```

**Thresholds:**
- Report if deviation >= 3.0 (3x normal)
- Or if baseline > 10/hour and deviation >= 2.0

---

## Implementation Details

### Data Structures

```cpp
/**
 * Pattern statistics tracker
 */
struct PatternStats
{
   String pattern;                           // Normalized message pattern
   uint32_t count;                           // Total occurrences
   time_t firstSeen;                         // First occurrence
   time_t lastSeen;                          // Last occurrence
   IntegerArray<time_t> timestamps;          // All timestamps (for interval analysis)
   HashMap<uint32_t, uint32_t> sourceObjects; // ObjectId -> count
   int maxSeverity;                          // Highest severity seen
};

/**
 * Time bucket for volume analysis
 */
struct TimeBucket
{
   time_t startTime;
   uint32_t totalCount;
   uint32_t errorCount;
   uint32_t warningCount;
   StringObjectMap<uint32_t> patternCounts;  // Pattern -> count
};

/**
 * Analysis context
 */
struct LogAnalysisContext
{
   time_t timeFrom;
   time_t timeTo;
   time_t bucketSize;

   StringObjectMap<PatternStats> patterns;
   ObjectArray<TimeBucket> timeBuckets;

   double baselineRate;
   double baselineStdDev;

   ObjectArray<json_t> bursts;
   ObjectArray<json_t> recurringPatterns;
   ObjectArray<json_t> newPatterns;
   ObjectArray<json_t> volumeAnomalies;
};
```

### Helper Functions

```cpp
// Parse time parameter (ISO format or relative minutes)
time_t ParseTimeParameter(json_t *arguments, const char *paramName, time_t defaultValue);

// Normalize log message for pattern extraction
String NormalizeLogMessage(const TCHAR *message);

// Get human-readable names
const char* GetSyslogFacilityName(int facility);
const char* GetSyslogSeverityName(int severity);
const char* GetWindowsEventSeverityName(int severity);
const char* GetEventSeverityName(int severity);

// Format interval as human-readable string
const char* FormatInterval(int seconds);

// Escape string for SQL LIKE pattern
String EscapeSQLString(const String& input);

// Calculate optimal bucket size
time_t CalculateOptimalBucketSize(time_t windowDuration);

// JSON error response helper
std::string JsonError(const char *message);
```

### File Structure

```
src/server/aitools/
├── aitools.h
│   └── Add function declarations:
│       - F_SearchSyslog
│       - F_SearchWindowsEvents
│       - F_SearchSnmpTraps
│       - F_SearchEvents
│       - F_GetLogStatistics
│       - F_CorrelateLogs
│       - F_AnalyzeLogPatterns
│
├── logs.cpp (NEW - ~1500 lines)
│   ├── Helper functions (normalization, parsing)
│   ├── F_SearchSyslog implementation
│   ├── F_SearchWindowsEvents implementation
│   ├── F_SearchSnmpTraps implementation
│   ├── F_SearchEvents implementation
│   ├── F_GetLogStatistics implementation
│   ├── F_CorrelateLogs implementation
│   └── F_AnalyzeLogPatterns implementation
│       ├── ProcessSyslogForPatterns
│       ├── ProcessWindowsEventsForPatterns
│       ├── ProcessSnmpTrapsForPatterns
│       ├── ProcessEventsForPatterns
│       ├── CalculateBaseline
│       ├── DetectBursts
│       ├── DetectRecurringPatterns
│       ├── DetectNewPatterns
│       └── DetectVolumeAnomalies
│
├── main.cpp
│   └── Add skill registration in CreateAssistantSkillList()
│
└── skills/
    └── log-analysis.md (NEW)
```

---

## Skill Registration

```cpp
RegisterAIAssistantSkill(
   "log-analysis",
   "Provides comprehensive log analysis capabilities for NetXMS. Use this skill to search "
   "and analyze syslog messages, Windows events, SNMP traps, and system events. Essential "
   "for troubleshooting, root cause analysis, security investigation, compliance auditing, "
   "and detecting patterns or anomalies in log data across monitored infrastructure.",
   "@log-analysis.md",
   {
      AssistantFunction(
         "search-syslog",
         "Search syslog messages with flexible filtering by time, source, severity, "
         "facility, and text content.",
         {
            { "object", "optional node name or ID to filter by source" },
            { "time_from", "start time (ISO format or relative like '-60' for 60 min ago)" },
            { "time_to", "end time (ISO format, defaults to now)" },
            { "severity", "syslog severity (0-7) or array of severities" },
            { "facility", "syslog facility (0-23) or array" },
            { "text_pattern", "case-insensitive text search pattern" },
            { "tag", "syslog tag filter" },
            { "limit", "max results (default: 100, max: 1000)" }
         },
         F_SearchSyslog),
      AssistantFunction(
         "search-windows-events",
         "Search Windows event log entries collected from monitored Windows hosts.",
         {
            { "object", "optional node name or ID" },
            { "time_from", "start time" },
            { "time_to", "end time" },
            { "log_name", "log name (System, Application, Security, etc.)" },
            { "event_source", "event source filter" },
            { "event_code", "event ID or array of event IDs" },
            { "severity", "severity (1=Error, 2=Warning, 4=Info, 8/16=Audit)" },
            { "text_pattern", "text search pattern" },
            { "limit", "max results (default: 100)" }
         },
         F_SearchWindowsEvents),
      AssistantFunction(
         "search-snmp-traps",
         "Search SNMP traps received from network devices.",
         {
            { "object", "optional node name or ID" },
            { "time_from", "start time" },
            { "time_to", "end time" },
            { "trap_oid", "trap OID or OID prefix filter" },
            { "ip_address", "source IP address filter" },
            { "varbind_pattern", "text search in varbind data" },
            { "limit", "max results (default: 100)" }
         },
         F_SearchSnmpTraps),
      AssistantFunction(
         "search-events",
         "Search NetXMS system events generated by thresholds, polling, and scripts.",
         {
            { "object", "optional source object name or ID" },
            { "time_from", "start time" },
            { "time_to", "end time" },
            { "event_code", "event code or name" },
            { "severity", "severity (0=Normal to 4=Critical)" },
            { "origin", "event origin (SYSTEM, SNMP, SYSLOG, AGENT, SCRIPT)" },
            { "tags", "event tags filter" },
            { "text_pattern", "text search in message" },
            { "limit", "max results (default: 100)" }
         },
         F_SearchEvents),
      AssistantFunction(
         "get-log-statistics",
         "Get log volume statistics for capacity planning and trend analysis.",
         {
            { "log_type", "log type: syslog, windows_events, snmp_traps, events (required)" },
            { "object", "optional object to filter" },
            { "time_from", "start time (default: 24h ago)" },
            { "time_to", "end time (default: now)" },
            { "group_by", "grouping: hour, day, source, severity (default: hour)" }
         },
         F_GetLogStatistics),
      AssistantFunction(
         "correlate-logs",
         "Cross-correlate log entries across multiple sources within a time window.",
         {
            { "object", "primary object for correlation (required)" },
            { "time_from", "correlation window start (required)" },
            { "time_to", "correlation window end (required)" },
            { "include_neighbors", "include L2 neighbors (default: true)" },
            { "log_types", "array of log types to include (default: all)" },
            { "limit_per_source", "max entries per source (default: 50)" }
         },
         F_CorrelateLogs),
      AssistantFunction(
         "analyze-log-patterns",
         "Analyze log patterns for anomaly detection, burst identification, and trends.",
         {
            { "object", "optional object to analyze" },
            { "log_type", "log type to analyze (required)" },
            { "time_from", "analysis start (default: 24h ago)" },
            { "time_to", "analysis end (default: now)" },
            { "pattern_type", "pattern type: frequency, burst, recurring, new (default: all)" }
         },
         F_AnalyzeLogPatterns)
   }
);
```

---

## Reference Data

### Syslog Severity Levels

| Level | Name | Description |
|-------|------|-------------|
| 0 | Emergency | System is unusable |
| 1 | Alert | Immediate action required |
| 2 | Critical | Critical conditions |
| 3 | Error | Error conditions |
| 4 | Warning | Warning conditions |
| 5 | Notice | Normal but significant |
| 6 | Informational | Informational messages |
| 7 | Debug | Debug-level messages |

### Syslog Facility Codes

| Code | Name | Code | Name |
|------|------|------|------|
| 0 | kern | 12 | ntp |
| 1 | user | 13 | security |
| 2 | mail | 14 | console |
| 3 | daemon | 15 | clock |
| 4 | auth | 16 | local0 |
| 5 | syslog | 17 | local1 |
| 6 | lpr | 18 | local2 |
| 7 | news | 19 | local3 |
| 8 | uucp | 20 | local4 |
| 9 | cron | 21 | local5 |
| 10 | authpriv | 22 | local6 |
| 11 | ftp | 23 | local7 |

### Windows Event Severity

| Value | Name |
|-------|------|
| 1 | Error |
| 2 | Warning |
| 4 | Information |
| 8 | Audit Success |
| 16 | Audit Failure |

### NetXMS Event Severity

| Value | Name |
|-------|------|
| 0 | Normal |
| 1 | Warning |
| 2 | Minor |
| 3 | Major |
| 4 | Critical |

### Event Origins

| Origin | Description |
|--------|-------------|
| SYSTEM | Internal NetXMS events |
| SNMP | Generated from SNMP traps |
| SYSLOG | Generated from syslog messages |
| AGENT | Events from NetXMS agents |
| SCRIPT | Events generated by NXSL scripts |
| WINDOWS_EVENT | From Windows Event Log |
| REMOTE_SERVER | Forwarded from another NetXMS server |

---

## Use Cases

### 1. Troubleshooting Network Outage

```
User: "What happened to router-core-01 around 10:05 AM today?"

AI uses:
1. correlate-logs(object="router-core-01", time_from="10:00", time_to="10:15")
   → Returns timeline of syslog, traps, and events across router and neighbors
2. Identifies: Interface down, SNMP linkDown trap, SYS_IF_DOWN event
3. Reports correlated timeline showing interface failure cascade
```

### 2. Security Investigation

```
User: "Show me failed login attempts on Windows servers in the last hour"

AI uses:
1. search-windows-events(log_name="Security", event_code=4625, time_from="-60")
   → Returns all failed logon events
2. get-log-statistics(log_type="windows_events", group_by="source")
   → Identifies which servers have most failures
3. Reports findings with affected systems and patterns
```

### 3. Detecting Log Storms

```
User: "Are there any unusual log patterns today?"

AI uses:
1. analyze-log-patterns(log_type="syslog", pattern_type="burst")
   → Identifies periods of abnormally high log volume
2. analyze-log-patterns(log_type="syslog", pattern_type="new")
   → Identifies new error patterns that started recently
3. Reports bursts, new patterns, and affected sources
```

### 4. Compliance Audit

```
User: "Get all privileged access events from domain controllers this week"

AI uses:
1. find-objects(name="DC", classes=["node"])
   → Gets list of domain controllers
2. search-windows-events(object="DC-01", log_name="Security",
      event_code=[4672,4673,4674], time_from="-10080")
   → Gets privileged access events
3. get-log-statistics(log_type="windows_events", group_by="day")
   → Gets daily summary
4. Reports audit trail with statistics
```

---

## Performance Considerations

### Query Optimization

1. **Index Usage**: All queries filter on indexed columns (timestamp, object_id)
2. **Result Limits**: Default 100, max 1000 to prevent memory issues
3. **Time-based Partitioning**: TimescaleDB hypertables optimize time-range queries

### Memory Management

1. **Pattern Caching**: Store only top N patterns if memory constrained
2. **Streaming Processing**: Process large result sets in batches
3. **Bucket Aggregation**: Aggregate data in time buckets to reduce memory

### Recommended Limits

| Analysis Type | Recommended Window | Max Records |
|--------------|-------------------|-------------|
| Search queries | <= 24 hours | 1000 |
| Correlation | <= 1 hour | 500 per source |
| Pattern analysis | 1-7 days | Aggregated |
| Statistics | <= 30 days | Aggregated |

---

## Testing Strategy

### Unit Tests

1. Message normalization (various formats)
2. Time parameter parsing (ISO, relative)
3. Z-score calculation
4. Interval detection

### Integration Tests

1. Search with various filter combinations
2. Correlation across log types
3. Pattern detection with synthetic data
4. Access control enforcement

### Performance Tests

1. Large result set handling (10K+ records)
2. Pattern analysis with high-volume logs
3. Concurrent query handling

---

## Future Enhancements

1. **Full-Text Search**: Integration with PostgreSQL full-text search or Elasticsearch
2. **Saved Searches**: Store and recall common search criteria
3. **Real-time Streaming**: WebSocket-based log streaming
4. **ML Anomaly Detection**: Machine learning models for pattern detection
5. **Export**: CSV/JSON export of search results
6. **Alert Integration**: Auto-create alarms from detected patterns
7. **Dashboard Widgets**: Log statistics visualization

---

## References

- [NetXMS Syslog Documentation](https://www.netxms.org/documentation/adminguide/syslog.html)
- [Windows Event Log Monitoring](https://www.netxms.org/documentation/adminguide/windows-event-log.html)
- [SNMP Trap Processing](https://www.netxms.org/documentation/adminguide/snmp-traps.html)
- RFC 5424 - The Syslog Protocol
- RFC 3164 - BSD Syslog Protocol
