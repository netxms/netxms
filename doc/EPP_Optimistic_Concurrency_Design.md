# Event Processing Policy: Optimistic Concurrency Design

## Overview

This document describes the transition from exclusive locking to optimistic concurrency control for Event Processing Policy (EPP) editing in NetXMS.

### Problem Statement

The current EPP editing model uses exclusive locking:
- When a user opens the EPP editor, a server-side lock is acquired
- No other users can edit EPP while the lock is held
- Lock is released when editor is closed or session disconnects

This causes operational issues:
- Users who open EPP just to view it still hold the lock
- Forgotten open editors block other administrators
- Only workaround is forcibly disconnecting the blocking session
- Model is incompatible with stateless Web API

### Solution

Replace exclusive locking with optimistic concurrency control:
- No locks acquired when opening EPP
- Multiple users can view and edit simultaneously
- Conflicts detected at save time
- Server performs automatic merge when possible
- User resolves conflicts via UI when automatic merge fails

## Design Decisions

| Aspect | Decision | Rationale |
|--------|----------|-----------|
| Concurrency model | Optimistic (no locks) | Eliminates blocking, Web API friendly |
| Conflict granularity | Whole rule | Simpler UI, sufficient for rare conflicts |
| Reorder conflicts | Last-write-wins | Simplifies merge logic, acceptable for rare edits |
| Versioning | Policy version + per-rule version | Enables precise conflict detection |
| Merge execution | Server-side | Single source of truth |
| Conflict resolution | Client-side dialog | User decides on conflicts |

## Data Model Changes

### Schema Modifications (Implemented in 60.29)

```sql
-- Per-rule modification tracking (add columns to event_policy table)
ALTER TABLE event_policy ADD modified_by_guid VARCHAR(36);  -- GUID of user who last modified
ALTER TABLE event_policy ADD modified_by_name VARCHAR(63);  -- User's name at modification time
ALTER TABLE event_policy ADD modification_time INTEGER;     -- Unix timestamp of last modification
```

All columns are nullable - existing rules have NULL values until modified.

### In-Memory Versioning

Policy and rule versions are tracked in memory only (not persisted):

**Policy version:**
- Maintained in-memory by the server
- Reset to 0 on server start
- Incremented on any EPP modification (add, delete, modify, reorder)
- Used as base version for conflict detection

**Rule version:**
- Maintained in-memory per rule
- Initialized to 1 when rules are loaded
- Incremented when rule content changes
- NOT incremented on reorder (position change only)
- Used to detect per-rule conflicts

This design was chosen because:
- Versions are consistent at server start (all clients must reload anyway)
- Simpler schema - no need to maintain version counters in database
- Avoids complexity of version recovery after crash

### User Identification

User identification uses both GUID and name:
- `modified_by_guid` - Stable identifier for user lookups
- `modified_by_name` - Preserved for display when user is deleted

### Rule Identification

- Rules identified by `rule_guid` field (stable across reorders)
- `rule_id` is display/execution order only

## Server-Side Merge Algorithm

### Input

Client submits:
```
{
  baseVersion: integer,        // EPP version when client loaded policy
  rules: [
    {
      guid: string | null,     // null for new rules
      version: integer,        // rule version when loaded (ignored for new rules)
      ruleNumber: integer,     // desired position
      ...rule fields...
    },
    ...
  ]
}
```

### Algorithm

```
function mergeEPP(clientBaseVersion, clientRules):
    serverPolicy = loadCurrentEPP()

    // Fast path: no concurrent changes
    if clientBaseVersion == serverPolicy.version:
        applyChanges(clientRules)
        return Success(newVersion)

    // Concurrent changes detected - perform merge
    conflicts = []
    mergedRules = []

    // Build lookup of server rules by GUID
    serverRuleMap = map(serverPolicy.rules, r => r.guid -> r)
    clientRuleGuids = set(clientRules.filter(r => r.guid).map(r => r.guid))

    // Process each client rule
    for clientRule in clientRules:
        if clientRule.guid is null:
            // New rule - always accept
            mergedRules.add(clientRule)
            continue

        serverRule = serverRuleMap.get(clientRule.guid)

        if serverRule is null:
            // Rule was deleted on server while client edited
            // Treat as conflict if client modified it
            if clientRule.modified:
                conflicts.add(DeleteConflict(clientRule))
            // Otherwise: client's version deleted it too, or it's a true conflict
            continue

        if serverRule.version == clientRule.version:
            // Server rule unchanged since client loaded
            // Accept client's version (whether modified or not)
            mergedRules.add(clientRule)
        else:
            // Server rule was modified
            if clientRule.modified:
                // Both modified - CONFLICT
                conflicts.add(ModifyConflict(clientRule, serverRule))
            else:
                // Only server modified - keep server version
                mergedRules.add(serverRule)

    // Check for rules deleted by client but modified on server
    for serverRule in serverPolicy.rules:
        if serverRule.guid not in clientRuleGuids:
            // Client deleted this rule
            originalVersion = getOriginalRuleVersion(clientBaseVersion, serverRule.guid)
            if serverRule.version > originalVersion:
                // Rule was modified after client loaded - conflict
                conflicts.add(DeleteConflict(serverRule))
            // Otherwise: safe to delete

    if conflicts.isEmpty():
        // Apply merged rules with client's ordering (last-write-wins for order)
        applyChanges(mergedRules)
        return Success(newVersion)
    else:
        return Conflict(conflicts, serverPolicy.version)
```

### Conflict Types

**ModifyConflict:**
- Same rule modified by both client and server
- Resolution: keep mine / keep theirs / edit manually

**DeleteConflict:**
- Client deleted a rule that server modified, OR
- Client modified a rule that server deleted
- Resolution: keep deleted / restore with version

## API Design

### NXCP Protocol (Java Console)

**Current messages (to be deprecated):**
```
CMD_OPEN_EPP         - acquires lock, returns policy
CMD_SAVE_EPP         - saves policy
CMD_CLOSE_EPP        - releases lock
```

**New messages:**
```
CMD_GET_EPP
  Request: (empty)
  Response:
    - version: INTEGER
    - rules: ARRAY of rule data (including per-rule version)

CMD_SAVE_EPP_WITH_MERGE
  Request:
    - baseVersion: INTEGER
    - rules: ARRAY of rule data
  Response (success):
    - success: BOOLEAN (true)
    - newVersion: INTEGER
  Response (conflict):
    - success: BOOLEAN (false)
    - serverVersion: INTEGER
    - conflicts: ARRAY of:
      - ruleGuid: STRING
      - conflictType: INTEGER (modify=1, delete=2)
      - clientRule: rule data (if applicable)
      - serverRule: rule data (if applicable)
      - serverModifiedBy: STRING
      - serverModifiedTime: INTEGER
```

### Java Client API

```java
public class NXCSession {
    /**
     * Get event processing policy without locking.
     * @return policy with version information
     */
    public EventProcessingPolicy getEventProcessingPolicy() throws Exception;

    /**
     * Save event processing policy with optimistic concurrency.
     * @param baseVersion version when policy was loaded
     * @param policy modified policy
     * @return save result (success or conflicts)
     */
    public EPPSaveResult saveEventProcessingPolicy(int baseVersion,
                                                    EventProcessingPolicy policy) throws Exception;

    // Deprecated - for backward compatibility during transition
    @Deprecated
    public EventProcessingPolicy openEventProcessingPolicy() throws Exception;
    @Deprecated
    public void closeEventProcessingPolicy() throws Exception;
}

public class EPPSaveResult {
    private boolean success;
    private int newVersion;
    private List<EPPConflict> conflicts;

    public boolean isSuccess();
    public int getNewVersion();
    public List<EPPConflict> getConflicts();
}

public class EPPConflict {
    public enum Type { MODIFY, DELETE }

    private Type type;
    private String ruleGuid;
    private EventProcessingPolicyRule clientRule;
    private EventProcessingPolicyRule serverRule;
    private String serverModifiedBy;
    private long serverModifiedTime;
}
```

### Web API

**Get policy:**
```
GET /v1/epp

Response 200:
{
  "version": 42,
  "rules": [
    {
      "guid": "550e8400-e29b-41d4-a716-446655440000",
      "version": 3,
      "ruleNumber": 1,
      "comments": "Handle critical events",
      "events": [1, 2, 3],
      "sources": [100, 200],
      "flags": 1,
      "alarmMessage": "%m",
      "alarmSeverity": 4,
      "actions": [10, 20],
      ...
    },
    ...
  ]
}
```

**Save policy:**
```
PUT /v1/epp
Content-Type: application/json

{
  "baseVersion": 42,
  "rules": [...]
}

Response 200 (success):
{
  "version": 43
}

Response 409 (conflict):
{
  "conflicts": [
    {
      "type": "modify",
      "ruleGuid": "550e8400-e29b-41d4-a716-446655440000",
      "clientRule": {...},
      "serverRule": {...},
      "serverModifiedBy": "admin",
      "serverModifiedTime": 1706234567
    }
  ],
  "serverVersion": 45
}
```

**Save single rule:**
```
PUT /v1/epp/rules/{guid}
Content-Type: application/json

{
  "baseVersion": 42,
  "ruleVersion": 3,
  "comments": "Updated comment",
  ...
}

Response 200:
{
  "version": 43,
  "ruleVersion": 4
}

Response 409:
{
  "conflict": {
    "clientRule": {...},
    "serverRule": {...},
    "serverModifiedBy": "admin",
    "serverModifiedTime": 1706234567
  }
}
```

**Create rule:**
```
POST /v1/epp/rules
Content-Type: application/json

{
  "baseVersion": 42,
  "afterRuleGuid": "550e8400-e29b-41d4-a716-446655440000",  // null for first position
  "comments": "New rule",
  ...
}

Response 201:
{
  "guid": "661f9511-f30c-52e5-b827-557766551111",
  "version": 43,
  "ruleVersion": 1
}
```

**Delete rule:**
```
DELETE /v1/epp/rules/{guid}?baseVersion=42&ruleVersion=3

Response 200:
{
  "version": 43
}

Response 409:
{
  "conflict": "Rule was modified by admin at 2024-01-25 10:30:00"
}
```

## Client UI Changes

### Editor State Machine

```
                                    ┌──────────────────┐
                                    │                  │
         ┌──────────────────────────│     Closed       │
         │                          │                  │
         │                          └────────┬─────────┘
         │                                   │
         │                              open (GET /epp)
         │                                   │
         │                                   ▼
         │                          ┌──────────────────┐
         │     close                │                  │
         └──────────────────────────│     Viewing      │◄──────────────────┐
                                    │   (read-only)    │                   │
                                    └────────┬─────────┘                   │
                                             │                             │
                                        first edit                         │
                                             │                             │
                                             ▼                             │
                                    ┌──────────────────┐                   │
                                    │                  │                   │
                                    │     Editing      │                   │
                                    │   (modified)     │                   │
                                    │                  │                   │
                                    └────────┬─────────┘                   │
                                             │                             │
                                           save                            │
                                             │                             │
                        ┌────────────────────┴────────────────────┐        │
                        │                                         │        │
                        ▼                                         ▼        │
               ┌──────────────────┐                      ┌──────────────────┐
               │                  │                      │                  │
               │  Save Success    │──────────────────────│    Conflict      │
               │                  │     resolved         │    Resolution    │
               └──────────────────┘                      │                  │
                        │                                └──────────────────┘
                        │                                         │
                        │                                    cancel│
                        │                                         │
                        └─────────────────────────────────────────┘
                                             │
                                             ▼
                                       back to Editing
```

### Conflict Resolution Dialog

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  Save Conflict                                                         [X]  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Rule "Handle critical events" was modified by john@workstation-5           │
│  3 minutes ago while you were editing.                                      │
│                                                                             │
│  ┌─────────────────────────────────┐ ┌─────────────────────────────────┐   │
│  │ Your Version                    │ │ Server Version                  │   │
│  ├─────────────────────────────────┤ ├─────────────────────────────────┤   │
│  │ Events: SYS_NODE_DOWN           │ │ Events: SYS_NODE_DOWN,          │   │
│  │                                 │ │         SYS_SERVICE_DOWN        │   │
│  │ Sources: All                    │ │ Sources: All                    │   │
│  │                                 │ │                                 │   │
│  │ Actions:                        │ │ Actions:                        │   │
│  │  - Log to file                  │ │  - Log to file                  │   │
│  │                                 │ │  - Send email                   │   │
│  │                                 │ │                                 │   │
│  │ Alarm: Generate, Critical      │ │ Alarm: Generate, Critical       │   │
│  └─────────────────────────────────┘ └─────────────────────────────────┘   │
│                                                                             │
│  How do you want to resolve this conflict?                                  │
│                                                                             │
│  ○ Keep my version (discard server changes)                                 │
│  ○ Keep server version (discard my changes)                                 │
│  ○ Edit manually (opens rule editor pre-filled with server version)         │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ Conflict 1 of 2                              [Apply to All Similar] │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│                                    [Previous]  [Next]  [Resolve]  [Cancel]  │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Removed UI Elements

- Lock status indicator in EPP editor
- "Locked by..." error dialog when opening
- "Force unlock" action in session manager
- Lock-related entries in server console session list

## Implementation Phases

### Phase 1: Database Schema ✓ COMPLETED

**Schema Version:** 60.28 → 60.29

**Files Modified:**
- `include/netxmsdb.h` - Bump `DB_SCHEMA_VERSION_MINOR` to 29
- `src/server/tools/nxdbmgr/upgrade_v60.cpp` - Add `H_UpgradeFromV28()` upgrade function
- `sql/schema.in` - Add columns to `event_policy` table definition

**Changes Implemented:**
- Added `modified_by_guid VARCHAR(36) NULL` to `event_policy`
- Added `modified_by_name VARCHAR(63) NULL` to `event_policy`
- Added `modification_time INTEGER NULL` to `event_policy`
- Database upgrade procedure using `SQLBatch()` for cross-database compatibility

**Design Decisions:**
- No `version` column - versioning tracked in-memory at runtime
- No `EPPVersion` config param - policy version tracked in-memory
- No creation tracking columns - modification info serves as creation info for new rules
- User GUID + name stored together for deleted user fallback display

### Phase 2: Server Core

**Files:**
- `src/server/core/epp.cpp`
- `src/server/core/session.cpp`
- `src/server/include/nms_core.h`

**Tasks:**
- Implement merge algorithm
- Add in-memory version tracking (policy version + per-rule versions)
- Populate `modified_by_guid`, `modified_by_name`, `modification_time` on save
- New NXCP message handlers
- Keep old lock-based handlers working (deprecation period)

### Phase 3: Java Client Library

**Files:**
- `src/client/java/netxms-client/src/main/java/org/netxms/client/NXCSession.java`
- `src/client/java/netxms-client/src/main/java/org/netxms/client/events/EventProcessingPolicy.java`
- `src/client/java/netxms-client/src/main/java/org/netxms/client/events/EventProcessingPolicyRule.java`
- New: `EPPSaveResult.java`, `EPPConflict.java`

**Tasks:**
- Add version fields to policy and rule classes
- Implement new `getEventProcessingPolicy()` method
- Implement new `saveEventProcessingPolicy()` with merge result
- Deprecate old lock-based methods

### Phase 4: Java Console UI

**Files:**
- `src/client/nxmc/java/src/main/java/org/netxms/nxmc/modules/events/views/EventProcessingPolicyEditor.java`
- New: `EPPConflictDialog.java`

**Tasks:**
- Remove lock acquisition on open
- Track base version and rule versions
- Detect modifications for each rule
- Implement conflict resolution dialog
- Handle save result (success vs conflict)

### Phase 5: Web API

**Files:**
- `src/server/webapi/epp.cpp` (new)
- `src/server/webapi/webapi.h`

**Tasks:**
- Implement GET/PUT /v1/epp endpoints
- Implement per-rule CRUD endpoints
- JSON serialization for rules and conflicts

### Phase 6: Cleanup

**Files:**
- `src/server/core/locks.cpp`
- `src/server/include/nms_locks.h`

**Tasks:**
- Remove `LockEPP()`, `UnlockEPP()` functions
- Remove deprecated NXCP handlers
- Remove deprecated Java client methods
- Update documentation

## Migration & Compatibility

### Backward Compatibility

During transition period:
- Old console versions continue using lock-based protocol
- Server supports both old and new protocols
- New console detects server capability and uses appropriate protocol

### Version Detection

Add server capability flag:
```
CAPABILITY_EPP_OPTIMISTIC_CONCURRENCY = 0x00001000
```

Java client checks capability before using new API.

## Testing Considerations

### Unit Tests

- Merge algorithm with various conflict scenarios
- Version increment logic
- Rule identification by GUID across reorders

### Integration Tests

- Concurrent edits with no conflict (different rules)
- Concurrent edits with conflict (same rule)
- Delete/modify conflicts
- Reorder during concurrent edit
- Network failure during save

### Manual Test Scenarios

1. User A and B both open EPP
2. User A modifies rule 5, saves - should succeed
3. User B modifies rule 10, saves - should auto-merge
4. User B modifies rule 5, saves - should show conflict dialog
5. User opens EPP, views only, closes - no impact on others
6. Web API modifies rule while console has EPP open

## Security Considerations

- All existing permission checks remain in place
- User must have EPP modify permission to save
- Audit log records all EPP changes with user attribution
- Version field prevents replay attacks (old version rejected)

## Future Enhancements

Potential future improvements (out of scope for initial implementation):

- **Field-level conflict resolution**: Show diff at field level, not whole rule
- **Real-time notifications**: Notify users when EPP changes while they have it open
- **Collaborative editing indicators**: Show which rules others are currently editing
- **Change history**: View previous versions of rules with diff
- **Conflict auto-resolution policies**: Admin-configurable rules for automatic resolution
