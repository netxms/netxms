# AI Agent Messages Design

## Overview

A system for AI agents running in background contexts (scheduled tasks, event processing rules, background operations) to communicate findings, reports, and approval requests to users through a dedicated UI.

This complements the existing synchronous question/answer mechanism in interactive chat sessions (see `AI_Agent_User_Sync_Interaction_Design.md`).

## Context

AI agents in NetXMS can run in several contexts:
- **Interactive chats**: User-initiated, synchronous Q&A supported
- **Background tasks**: Autonomous execution, no blocking interaction
- **Scheduled tasks**: Periodic AI operations
- **Event processing rules**: AI triggered by system events

Background/scheduled/event-triggered tasks need a way to communicate results without blocking. This design provides that mechanism.

## Message Types

| Type | Purpose | Expiration | Approval Flow |
|------|---------|------------|---------------|
| `INFORMATIONAL` | Findings, reports, analysis results | Auto-expire (configurable) | None |
| `ALERT` | Anomalies, warnings needing attention | Auto-expire | None |
| `APPROVAL_REQUEST` | Deferred decision from background task | Auto-expire (treated as rejected) | Contains task spawn instructions |

## Approval Request Flow

Background tasks cannot block waiting for user approval. Instead, they create an approval request message with instructions for a new task to spawn upon approval, then complete normally.

```
Background Task                    AI Messages Table              User Action
     │                                   │                            │
     │ Creates approval message          │                            │
     │ with spawn_task_instructions      │                            │
     │──────────────────────────────────>│                            │
     │                                   │                            │
     │ Task completes (no waiting)       │                            │
     │                                   │                            │
     │                                   │<───────── User approves ───│
     │                                   │                            │
     │                                   │ Spawn new task from        │
     │                                   │ stored instructions        │
     │                                   │────────────────────────────>
```

If the approval expires without user action, no task is spawned (treated as rejection).

## Targeting Model

Messages can be targeted to:
- **Initiator-only**: Default for personal tasks (reports, one-off queries)
- **Initiator + responsible users**: For operational/monitoring tasks

Targeting mode is specified when the task is created or scheduled.

## Notification Channels

- Task-level control over whether output goes to notification channels
- Approval requests may additionally notify via user's preferred channel (future enhancement)

## Database Schema

```sql
CREATE TABLE ai_messages (
   id integer not null,                    -- Primary key
   message_guid varchar(36) not null,      -- For external references
   message_type integer not null,          -- 0=informational, 1=alert, 2=approval_request
   creation_time integer not null,         -- Unix timestamp
   expiration_time integer not null,       -- Unix timestamp, auto-delete after
   source_task_id integer null,            -- Optional: originating task ID
   source_task_type varchar(63) null,      -- e.g., "scheduled", "event_rule", "background"
   related_object_id integer null,         -- Link to NetXMS object (node, etc.)
   title varchar(255) not null,            -- Short summary for list display
   message_text text not null,             -- Full message content
   spawn_task_data text null,              -- JSON: instructions to spawn task on approval
   status integer not null default 0,      -- 0=pending, 1=read, 2=approved, 3=rejected, 4=expired
   PRIMARY KEY(id)
);

CREATE TABLE ai_message_recipients (
   message_id integer not null,
   user_id integer not null,
   is_initiator boolean not null default false,  -- true if this user initiated the task
   read_time integer null,                       -- When user first viewed it
   PRIMARY KEY(message_id, user_id)
);
```

### Design Decisions

- **Separate recipients table**: Allows multiple recipients per message, tracks read status per user
- **spawn_task_data as JSON**: Flexible storage for task creation parameters without schema changes
- **status on message, not per-recipient**: Approval/rejection is a single action affecting the message itself
- **related_object_id**: Single object link for now; can extend to junction table if multiple objects needed

## Server API

### C++ API for AI Tasks

```cpp
// Create informational message
// Returns message ID on success, 0 on failure
uint32_t create_ai_message(
   const TCHAR *title,
   const TCHAR *text,
   uint32_t relatedObjectId = 0,         // Optional object link
   uint32_t expirationSeconds = 604800,  // Default 7 days
   bool includeResponsibleUsers = false  // Add responsible users as recipients
);

// Create approval request (background task version of askConfirmation)
// Task finishes after calling this - no blocking
uint32_t create_approval_request(
   const TCHAR *title,
   const TCHAR *text,
   const TCHAR *spawnTaskData,           // JSON with task creation params
   uint32_t relatedObjectId = 0,
   uint32_t expirationSeconds = 86400,   // Default 24h for approvals
   bool includeResponsibleUsers = false
);
```

### NXCP Commands

| Command | Direction | Purpose |
|---------|-----------|---------|
| `CMD_GET_AI_MESSAGES` | C→S | Get messages for current user (with filters) |
| `CMD_REQUEST_COMPLETED` | S→C | Response with message list (matched by request ID) |
| `CMD_AI_MESSAGE_UPDATE` | S→C | Push notification when new message arrives or status changes |
| `CMD_SET_AI_MESSAGE_STATUS` | C→S | Mark read, approve, reject, dismiss |
| `CMD_DELETE_AI_MESSAGE` | C→S | Explicit delete (admin only) |

### Server Components

- **AIMessageManager**: Singleton managing message lifecycle, expiration cleanup
- **Background thread**: Periodic cleanup of expired messages
- **Session notification**: Push `CMD_AI_MESSAGE_UPDATE` to connected recipients when new message created

## Approval Task Spawning

### spawn_task_data JSON Structure

```json
{
   "taskType": "ai_chat",
   "systemPrompt": "You are executing an approved operation...",
   "userPrompt": "Reload nginx service on node web-01 and verify it started correctly",
   "contextObjectId": 142,
   "timeout": 300,
   "notifyOnCompletion": true,
   "notifyChannels": ["email"]
}
```

### Task Types

| taskType | Description |
|----------|-------------|
| `ai_chat` | Spawn a new AI chat/task with given prompts |
| `nxsl_script` | Execute an NXSL script (script name + parameters) |
| `action` | Execute a predefined server action |

Starting with `ai_chat` is sufficient; others can be added later.

### Security

- Spawned task runs with permissions of **approving user** (not original task creator)
- spawn_task_data is created by server-side AI code, not user input (trusted)

## NXMC UI

### Implementation Status

The AI Messages UI is implemented as a Tools perspective view (accessible alongside AI Assistant).

### AI Messages View (Two-Pane Layout)

The view uses a master-detail layout similar to email clients:

```
┌──────────────────────────────────────────────────────────────────────────┐
│  AI Messages                                     [Filter] [🔍 Search]    │
├──────────────────────────────────────────────────────────────────────────┤
│  ID  │ Type    │ Status  │ Title           │ Related  │ Created │ Exp.  │
│──────┼─────────┼─────────┼─────────────────┼──────────┼─────────┼───────│
│  1   │ Info    │ Pending │ Analysis done   │ web-01   │ 10:30   │ 17:30 │  ← Bold = unread
│  2   │ Approval│ Pending │ Restart nginx   │ db-srv   │ 10:15   │ 10:15 │
│  3   │ Alert   │ Read    │ CPU anomaly     │ app-02   │ 09:45   │ 16:45 │
├──────────────────────────────────────────────────────────────────────────┤
│  Restart nginx on db-srv                          [✓ Approve] [✗ Reject]│
│  ────────────────────────────────────────────────────────────────────── │
│                                                                          │
│  Background task detected nginx configuration change.                    │
│                                                                          │
│  **Recommended action:** Reload nginx service                            │
│                                                                          │
│  ```                                                                     │
│  systemctl reload nginx                                                  │
│  ```                                                                     │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘
  Upper pane: Message list (60%)
  Lower pane: Message content with markdown rendering (40%)
```

### UI Features

| Feature | Description |
|---------|-------------|
| **Two-pane layout** | SashForm with 60/40 vertical split |
| **Bold font for unread** | Messages with PENDING status displayed in bold |
| **Markdown rendering** | Message content rendered via MarkdownViewer |
| **Approve/Reject buttons** | In header of content pane, with icons; visible for PENDING and READ approval requests |
| **Auto-mark-as-read** | Messages automatically marked as read after 30 seconds of viewing |
| **Real-time updates** | SessionListener handles AI_MESSAGE_CHANGED notifications |
| **Text filtering** | Filter by title or message text |
| **Export to CSV** | Standard table export functionality |

### Message List Columns

| Column | Content |
|--------|---------|
| ID | Message ID |
| Type | Info, Alert, or Approval |
| Status | Pending, Read, Approved, Rejected, Expired |
| Title | Message title |
| Related Object | Linked NetXMS object name |
| Created | Creation timestamp |
| Expires | Expiration timestamp |

### Content Panel

- **Header area**: Title (bold header font) + Approve/Reject buttons (right-aligned)
- **Separator line**: Visual divider between header and content
- **Content area**: MarkdownViewer for rich text display
- **Button visibility**: Approve/Reject buttons shown only for approval requests in PENDING or READ status

### Auto-Mark-As-Read

Messages are automatically marked as read when displayed in the preview pane:
- Timer starts when a PENDING message is selected
- After 30 seconds of continuous viewing, message is marked as READ
- Timer is cancelled if user selects a different message
- Configurable via `AUTO_READ_DELAY_MS` constant (currently 30000ms)

### Context Menu Actions

| Action | Description |
|--------|-------------|
| Mark as Read | Manually mark selected message(s) as read |
| Approve | Approve selected approval request (spawns task) |
| Reject | Reject selected approval request |
| Delete | Delete selected message(s) |

### Toolbar Actions

- Approve (with icon)
- Reject (with icon)
- Delete (with icon)

### Future Enhancements

- Inbox-style popup badge in main toolbar (not yet implemented)
- Notification channel integration for approvals
- Advanced filtering by type, status, time range, related object

## Configuration

### Server Configuration

```ini
[AI]
# Default expiration for informational messages (seconds, default 7 days)
MessageExpirationTime = 604800

# Default expiration for approval requests (seconds, default 24 hours)
ApprovalExpirationTime = 86400

# How often to run cleanup of expired messages (seconds, default 1 hour)
MessageCleanupInterval = 3600

# Maximum messages per user (0 = unlimited)
MaxMessagesPerUser = 1000
```

### Expiration Cleanup

Background thread in AIMessageManager periodically:
1. Deletes expired messages from database
2. Cleans up orphaned recipient records
3. Notifies connected clients about removed messages

### Client-side Handling

- Client receives `CMD_AI_MESSAGE_UPDATE` with deletion flag for expired messages
- UI removes message from list, shows brief "Message expired" if user was viewing it
- Pending approval that expires: UI shows "Expired - no action taken"

## Implementation

### Server Files

| Location | File | Purpose |
|----------|------|---------|
| `src/server/include/` | `ai_messages.h` | AIMessage struct, AIMessageManager class |
| `src/server/core/` | `ai_messages.cpp` | Implementation, cleanup thread |
| `src/server/tools/nxdbmgr/` | `upgrade_v60.cpp` | Schema upgrade (v60.21) |
| `sql/` | `schema.in` | Table definitions for ai_messages, ai_message_recipients |

### Java Client Library Files

| Location | File | Purpose |
|----------|------|---------|
| `.../client/constants/` | `AiMessageStatus.java` | Status enum (PENDING, READ, APPROVED, REJECTED, EXPIRED) |
| `.../client/constants/` | `AiMessageType.java` | Type enum (INFORMATIONAL, ALERT, APPROVAL_REQUEST) |
| `.../client/ai/` | `AiMessage.java` | Message model class |

### NXMC UI Files

| Location | File | Purpose |
|----------|------|---------|
| `.../modules/ai/views/` | `AiMessagesView.java` | Main two-pane view |
| `.../modules/ai/widgets/` | `AiMessageList.java` | Message list widget with table, actions, session listener |
| `.../modules/ai/widgets/` | `AiMessageContentPanel.java` | Content panel with markdown viewer, approve/reject buttons |
| `.../modules/ai/widgets/helpers/` | `AiMessageListLabelProvider.java` | Table label provider (bold for unread) |
| `.../modules/ai/widgets/helpers/` | `AiMessageListComparator.java` | Column sorting |
| `.../modules/ai/widgets/helpers/` | `AiMessageListFilter.java` | Text filtering |
| `.../modules/ai/` | `AiMessagesToolDescriptor.java` | Tools perspective registration |
| `icons/tool-views/` | `ai-messages.png` | View icon |

### Modified Files

| File | Changes |
|------|---------|
| `include/netxmsdb.h` | DB schema version bump to 60.21 |
| `include/nms_cscp.h` | NXCP command IDs and field IDs |
| `src/server/include/nms_core.h` | Include ai_messages.h |
| `src/server/core/main.cpp` | Initialize AIMessageManager |
| `src/server/core/Makefile.am` | Add ai_messages.cpp |
| `src/server/core/session.cpp` | Handle new NXCP commands |
| `src/server/core/iris.cpp` | Register `create-ai-message`, `create-approval-request` AI functions |
| `src/java-common/.../NXCPCodes.java` | CMD_* and VID_* constants |
| `src/client/java/.../SessionNotification.java` | AI_MESSAGE_CHANGED constant |
| `src/client/java/.../NXCSession.java` | Message handler + API methods (getAiMessages, markAiMessageAsRead, approveAiMessage, rejectAiMessage, deleteAiMessage) |
| `META-INF/services/...ToolDescriptor` | Register AiMessagesToolDescriptor |

### Implementation Phases (Completed)

**Phase 1: Database Schema** ✓
- Database schema in schema.in
- Upgrade procedure in upgrade_v60.cpp
- Version bump to 60.21

**Phase 2: Server Core** ✓
- AIMessage struct and AIMessageManager
- NXCP command handlers in session.cpp
- Background cleanup thread

**Phase 3: AI Integration** ✓
- `create-ai-message` function for AI agents
- `create-approval-request` function
- Task spawning on approval via RegisterAITask()

**Phase 4: Java Client Library** ✓
- AiMessage class with NXCP parsing
- AiMessageStatus and AiMessageType enums
- NXCSession methods for all operations
- Push notification handling

**Phase 5: NXMC UI** ✓
- Two-pane AiMessagesView
- Message list with bold unread, sorting, filtering
- Markdown content panel with approve/reject buttons
- Auto-mark-as-read after 30 seconds
- Tools perspective integration

**Future Phases**
- Inbox-style popup badge in main toolbar
- Notification channel integration for approvals
- User preferences for notification behavior
- Advanced filtering options

## Summary

| Aspect | Implementation |
|--------|----------------|
| Message types | Informational, Alert, Approval Request |
| Persistence | Database (ai_messages, ai_message_recipients tables) |
| Expiration | Auto-expire with configurable timeouts, cleanup thread |
| Approval model | Store spawn instructions in JSON; approval spawns new AI task via RegisterAITask() |
| Task permissions | Spawned task runs as approving user |
| Recipients | Initiator only, or initiator + responsible users (per-task) |
| UI | Two-pane Tools perspective view with markdown content, auto-mark-as-read |
| Auto-read | Messages marked as read after 30 seconds of viewing |
| Real-time | Push notifications via SessionListener for live updates |
