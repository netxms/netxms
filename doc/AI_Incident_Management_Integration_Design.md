# AI Incident Management Integration Design

## Overview

Integration between NetXMS incident management and AI capabilities, enabling AI-assisted incident analysis, AI-driven incident creation, and conversational incident handling.

## Scope

This design covers three integration areas:

1. **AI Assists with Incidents** - Analysis, root cause identification, correlation, recommendations
2. **AI Creates/Manages Incidents** - Background tasks can create incidents and add findings
3. **Conversational Incident Handling** - Users discuss incidents with AI in chat

## Context

This builds on existing infrastructure:
- Incident management system (incidents, alarms, comments, state machine)
- AI chat sessions with synchronous user interaction (`AI_Agent_User_Sync_Interaction_Design.md`)
- AI Messages for background task communication (`AI_Agent_Messages_Design.md`)
- SSH/agent execution with approval workflows (`AI_SSH_Integration_Design.md`)

---

## Part 1: AI-Assisted Incident Analysis

### Trigger Modes

| Mode | Description | Implementation |
|------|-------------|----------------|
| **On-demand** | User explicitly requests analysis | Chat command or UI action |
| **Automatic** | Rule-triggered background analysis | Event processing rule action |

### Automatic Analysis via Event Processing Rules

New EPP action type: **"Analyze incident with AI"**

```
Event Processing Rule
├── Conditions (severity, object, event type, etc.)
└── Actions
    ├── Create incident (existing)
    └── Analyze incident with AI (new)
        ├── Analysis depth: quick / standard / thorough
        └── Auto-assign: yes / no
```

The action spawns a background AI task that analyzes the incident and posts findings.

### Data Sources for Analysis

When analyzing an incident, the AI gathers context from:

| Source | Data Retrieved |
|--------|----------------|
| **Linked alarms** | All alarms associated with the incident, their states, timestamps, messages |
| **Object context** | Source node properties, status, capabilities, recent DCI values |
| **Recent events** | Event log from related objects within time window (e.g., ±1 hour) |
| **Historical incidents** | Past incidents on same object for pattern recognition |
| **Topology** | Network relationships, upstream dependencies, affected downstream objects |

### AI Functions for Incident Context

```cpp
// Get incident details with full context
// Returns: incident data, linked alarms, source object info
json get_incident_details(uint64_t incidentId);

// Get recent events for objects related to incident
// Returns: event list within time window
json get_incident_related_events(uint64_t incidentId, uint32_t timeWindowSeconds = 3600);

// Get historical incidents for pattern analysis
// Returns: past incidents on same object(s)
json get_incident_history(uint64_t objectId, uint32_t maxCount = 20);

// Get topology context for incident source
// Returns: upstream/downstream objects, dependencies
json get_incident_topology_context(uint64_t incidentId);
```

### AI Actions

| Action | Behavior | Approval Required |
|--------|----------|-------------------|
| **Post findings** | Add analysis as incident comment | No |
| **Suggest actions** | Include recommendations in comment | No |
| **Correlate alarms** | Link related alarms to incident | No (auto) |
| **Set assignee** | Assign incident to appropriate user | No (auto) |

### AI Functions for Incident Modification

```cpp
// Add comment to incident
// Returns: success/failure, comment ID
bool add_incident_comment(uint64_t incidentId, const TCHAR *text);

// Link alarm to incident
// Returns: success/failure
bool link_alarm_to_incident(uint64_t incidentId, uint64_t alarmId);

// Link multiple alarms to incident
// Returns: number of alarms linked
int link_alarms_to_incident(uint64_t incidentId, const IntegerArray<uint64_t> &alarmIds);

// Set incident assignee
// Returns: success/failure
bool assign_incident(uint64_t incidentId, uint32_t userId);

// Suggest assignee based on object responsible users
// Returns: suggested user ID, 0 if none found
uint32_t suggest_incident_assignee(uint64_t incidentId);
```

### Analysis Output Format

AI posts structured findings as incident comment:

```
## AI Analysis

**Summary:** Network connectivity issue affecting web-server-01, likely caused by upstream switch port flapping.

**Root Cause Assessment:**
- Switch port Gi0/24 showing CRC errors and frequent state changes
- 3 similar incidents on this object in past 30 days
- Upstream switch-core-01 also has active alarm for high CPU

**Correlated Alarms:**
- Linked alarm #4521 (switch-core-01: High CPU) - likely related
- Linked alarm #4518 (web-server-02: Connection timeout) - same root cause

**Recommendations:**
1. Check physical cable on switch port Gi0/24
2. Review switch-core-01 CPU usage patterns
3. Consider scheduling maintenance window for cable replacement

**Assigned to:** network-team (based on object responsibility)

---
*Analysis performed by AI at 2025-01-03 14:32:15*
```

---

## Part 2: AI Creates/Manages Incidents

### Incident Creation Triggers

Background AI tasks can create incidents in these scenarios:

| Trigger | Description |
|---------|-------------|
| **Anomaly detection** | AI detects unusual patterns during monitoring analysis |
| **Correlation** | AI identifies pattern across multiple alarms warranting unified tracking |
| **Proactive findings** | Scheduled health checks find issues before alarms fire |

### AI Functions for Incident Creation

```cpp
// Create new incident
// Returns: incident ID on success, 0 on failure
uint64_t create_incident(
   const TCHAR *title,
   uint32_t sourceObjectId,
   const TCHAR *initialComment = nullptr,
   uint64_t sourceAlarmId = 0
);

// Create incident and link multiple alarms
// Returns: incident ID on success, 0 on failure
uint64_t create_incident_from_alarms(
   const TCHAR *title,
   const IntegerArray<uint64_t> &alarmIds,
   const TCHAR *initialComment = nullptr
);
```

### Incident Creation Guidelines (System Prompt)

The AI system prompt includes guidelines for when to create incidents:

```
When to create incidents:
- You detect a pattern affecting multiple objects that should be tracked together
- Proactive analysis reveals an issue that hasn't triggered alarms yet
- Correlation shows multiple alarms stem from a single root cause

When NOT to create incidents:
- A single alarm already adequately represents the issue
- The finding is informational only (use AI Message instead)
- An existing incident already covers this situation
```

### AI Message Integration

For findings that don't warrant incidents, AI uses the existing AI Messages system:

| Finding Type | Action |
|--------------|--------|
| Critical issue requiring tracking | Create incident |
| Important finding, no tracking needed | Send AI Message (ALERT type) |
| Informational observation | Send AI Message (INFORMATIONAL type) |
| Action requiring approval | Send AI Message (APPROVAL_REQUEST type) |

---

## Part 3: Conversational Incident Handling

### Entry Points

| Entry Point | Behavior |
|-------------|----------|
| **From incident view** | "Discuss with AI" button opens chat with incident context preloaded |
| **From existing chat** | User references incident by ID: "analyze incident #1234" |

### Incident View Integration

New action in incident details view:

```
┌─────────────────────────────────────────────────────────────────────┐
│  Incident #1234: Web server connectivity issues                      │
│  ─────────────────────────────────────────────────────────────────  │
│  State: Open          Assigned: john.doe          Created: 2h ago   │
│                                                                      │
│  [Resolve] [Assign] [Link Alarm] [Discuss with AI]  ← New button    │
│                                                                      │
```

Clicking "Discuss with AI" opens chat panel with context message:

```
System: Starting conversation about Incident #1234: "Web server connectivity issues"

Context loaded:
- Source: web-server-01
- State: Open
- Linked alarms: 3
- Created: 2 hours ago

You can ask questions about this incident or request actions.
```

### Chat Context Binding

When chat is bound to an incident:

```cpp
class Chat
{
   // Existing fields...

   // Incident context (optional, set when chat is about specific incident)
   uint64_t m_boundIncidentId;  // 0 if not bound to incident

   // Methods
   void bindToIncident(uint64_t incidentId);
   uint64_t getBoundIncidentId() const;
};
```

### Conversational Capabilities

| Capability | Description | Example User Query |
|------------|-------------|-------------------|
| **Analysis** | Explain incident, root cause | "Why is this happening?" |
| **Query live data** | Pull current metrics, run diagnostics | "What's the current CPU on this server?" |
| **Add comments** | Post notes to incident | "Add a comment that I'm investigating" |
| **Execute remediation** | Run commands on related objects | "Restart the nginx service" |

### Query Live Data

AI can query current state during conversation:

```cpp
// Already exists in AI SSH integration
json execute_ssh_command(uint32_t nodeId, const TCHAR *command);
json get_dci_last_values(uint32_t nodeId, const StringList &dciNames);

// New: query specific to incident context
json get_incident_current_status(uint64_t incidentId);
```

### Remediation with Approval

Remediation actions use the existing approval flow from AI SSH Integration:

```
User: "Restart nginx on web-server-01"

AI: I'll restart the nginx service. This requires your approval.

┌─────────────────────────────────────────────────────────────────┐
│  🤖 Approval Required                                            │
│  ─────────────────────────────────────────────────────────────  │
│  Action: Restart nginx service                                   │
│  Target: web-server-01                                           │
│  Command: systemctl restart nginx                                │
│                                                                  │
│  [Approve]  [Reject]                     Expires in 4:58         │
└─────────────────────────────────────────────────────────────────┘
```

Upon approval, AI executes and logs action as incident comment:

```
## Action Executed

**Command:** systemctl restart nginx
**Target:** web-server-01
**Result:** Success
**Executed by:** AI (approved by john.doe)
**Time:** 2025-01-03 14:45:22

Output:
```
(no output - service restarted successfully)
```

---
*Action logged by AI*
```

### Incident Reference in Chat

Users can reference incidents in any chat:

```
User: "What's happening with incident #1234?"

AI: Let me look up incident #1234...

Incident #1234: "Web server connectivity issues"
- State: Open
- Source: web-server-01
- Created: 2 hours ago by alarm escalation
- Assigned to: john.doe
- Linked alarms: 3 active

Would you like me to analyze this incident in detail?
```

---

## Database Schema Changes

No new tables required. AI functions use existing incident tables:

- `incidents` - Main incident records
- `incident_alarms` - Linked alarms junction table
- `incident_comments` - Incident comments

New comment source tracking (extend existing):

```sql
-- Add AI source tracking to incident comments
ALTER TABLE incident_comments ADD COLUMN ai_generated boolean DEFAULT false;
ALTER TABLE incident_comments ADD COLUMN ai_task_id integer NULL;
```

---

## Server API Summary

### New AI Functions

| Function | Purpose |
|----------|---------|
| `get_incident_details` | Full incident context for analysis |
| `get_incident_related_events` | Events around incident timeframe |
| `get_incident_history` | Historical incidents on same object |
| `get_incident_topology_context` | Network topology for root cause |
| `add_incident_comment` | Post AI findings as comment |
| `link_alarm_to_incident` | Correlate alarm with incident |
| `link_alarms_to_incident` | Bulk alarm correlation |
| `assign_incident` | Set incident assignee |
| `suggest_incident_assignee` | Find appropriate assignee |
| `create_incident` | Create new incident from AI |
| `create_incident_from_alarms` | Create incident linking alarms |
| `get_incident_current_status` | Live status for conversation |

### Event Processing Rule Extension

New action type for automatic AI analysis:

```cpp
enum PolicyActionType
{
   // Existing...
   POLICY_ACTION_AI_ANALYZE_INCIDENT = 27
};

struct PolicyActionAIAnalyze
{
   int analysisDepth;      // 0=quick, 1=standard, 2=thorough
   bool autoAssign;        // Whether to auto-assign based on suggestion
   TCHAR customPrompt[256]; // Optional custom analysis instructions
};
```

---

## NXMC UI Changes

### Incident Details View

- Add "Discuss with AI" button to incident details toolbar
- Button opens AI chat panel with incident context

### AI Chat Panel

- Support incident context binding
- Show incident summary header when bound
- Enable incident-specific commands

### Chat Command Parsing

Recognize incident references in natural language:

```java
// Pattern matching for incident references
Pattern INCIDENT_REF = Pattern.compile(
   "incident\\s*#?(\\d+)|#(\\d+)",
   Pattern.CASE_INSENSITIVE
);
```

---

## Implementation Phases

### Phase 1: Core AI Functions
- Implement `get_incident_details`, `get_incident_related_events`
- Implement `add_incident_comment`, `link_alarm_to_incident`
- Implement `create_incident` for AI

### Phase 2: Analysis Pipeline
- Implement analysis depth levels (quick/standard/thorough)
- Add topology and history context gathering
- Create analysis output formatting

### Phase 3: Event Processing Integration
- Add AI analyze action to EPP
- Implement rule-triggered background analysis
- Add auto-assign capability

### Phase 4: Conversational UI
- Add "Discuss with AI" button to incident view
- Implement incident context binding in chat
- Add incident reference parsing

### Phase 5: Remediation Integration
- Connect to existing SSH/agent execution
- Add remediation logging to incident comments
- Implement action audit trail

---

## Security Considerations

1. **Access control** - AI functions respect user permissions for incident operations
2. **Remediation approval** - All command execution requires explicit user approval
3. **Audit logging** - All AI actions on incidents are logged with AI source flag
4. **Context boundaries** - AI only accesses data user has permission to view
5. **Comment attribution** - AI-generated comments clearly marked as such

---

## Summary

| Aspect | Decision |
|--------|----------|
| Analysis triggers | On-demand + automatic via EPP rules |
| Data sources | Alarms, object context, events, history, topology |
| AI actions | Report, suggest, correlate, assign (no state changes) |
| Action approval | Not required for correlation/assignment |
| Incident creation | Anomaly, correlation, proactive findings |
| Conversational entry | Incident view button + ID reference in chat |
| Chat capabilities | Analyze, query, comment, remediate (with approval) |
| Remediation logging | Actions logged as incident comments |
