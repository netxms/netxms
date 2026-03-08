# AI SSH Integration Design

## Overview

A new `aissh` server module providing AI agent capabilities for SSH-based data acquisition, autonomous troubleshooting, and network control across Linux/Unix servers and network devices.

## Use Cases

1. **Autonomous troubleshooting** - AI detects problems, SSHs to devices to diagnose/remediate
2. **Intelligent network control** - AI makes configuration changes based on conditions
3. **Conversational investigation** - User asks AI to investigate issues interactively via NXMC chat

## Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│  NXMC (Management Console)                                              │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │  Chat Panel (context-aware of selected node/alarm)              │   │
│  └─────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  netxmsd (Server)                                                       │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐                  │
│  │  IRIS Core   │  │   aitools    │  │    aiext     │                  │
│  │  (LLM API)   │  │   module     │  │    module    │                  │
│  └──────────────┘  └──────────────┘  └──────────────┘                  │
│         │                                                               │
│         ▼                                                               │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │  aissh module (NEW)                                              │  │
│  │  - SSH functions/skills for AI                                   │  │
│  │  - Command classification (hybrid + intent)                      │  │
│  │  - Approval workflow                                             │  │
│  │  - Audit logging                                                 │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│         │                                                               │
│         ▼                                                               │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │  Network Device Drivers (extended)                               │  │
│  │  - SSHDriverHints: prompt, enable, pagination                    │  │
│  │  - Command classification hints                                  │  │
│  │  - Output extractors (optional)                                  │  │
│  └──────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼ (Agent Protocol)
┌─────────────────────────────────────────────────────────────────────────┐
│  nxagentd (Agent)                                                       │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │  SSH Subagent (extended)                                         │  │
│  │  - Existing: SSHSession (exec mode)                              │  │
│  │  - NEW: SSHInteractiveSession (PTY + shell mode)                 │  │
│  │  - Session pool with conversation persistence                    │  │
│  └──────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼ (SSH)
┌─────────────────────────────────────────────────────────────────────────┐
│  Target Systems                                                         │
│  - Linux/Unix servers (exec mode)                                       │
│  - Network devices: Cisco, Juniper, etc. (interactive mode)             │
└─────────────────────────────────────────────────────────────────────────┘
```

## Autonomy Model

### Tiered Autonomy (Phase 1)

Interfaces designed to support context-dependent rules in Phase 5.

| Command Type | Autonomy | Examples |
|--------------|----------|----------|
| Read-only | Autonomous | show, get, display, cat, ps, df, logs |
| Write | Requires approval | restart, config changes, kill |
| Dangerous | Not available | firmware update, factory reset |

### Command Classification: Hybrid Approach

1. Check hardcoded patterns first (fast, reliable)
2. If no match → AI classifies with intent declaration
3. If uncertain → default to "requires approval"
4. Admin-configurable pattern overrides

## Approval Workflow

| Context | Model | Flow |
|---------|-------|------|
| Interactive chat | Synchronous | AI asks → user responds → execute/abort |
| Background task | Queue-based | AI queues → notification sent → async approval |

### Queue-based Approvals (Background Tasks)

- Pending storage with persistence
- User notification via configured channel
- Expiration timeout
- Approval via NXMC or API

## SSH Session Management

### Two Execution Modes

| Mode | Use Case | Implementation |
|------|----------|----------------|
| Exec | Linux/Unix servers | Existing `ssh_channel_request_exec()` |
| Interactive | Network devices | NEW: PTY + shell with prompt handling |

### SSHInteractiveSession (New Class)

```cpp
class SSHInteractiveSession
{
   bool connect(credentials, driverHints);
   bool waitForPrompt(timeout);
   bool escalatePrivilege(enablePassword);  // On-demand, when write command requires
   String executeCommand(command, timeout);
   void disablePagination();
   bool detectPromptPattern();              // Auto-learn if driver doesn't provide
};
```

### Session Persistence

Agent session pool handles session reuse; conversation activity extends idle timeout.

### Privilege Escalation

On-demand based on command type - only escalate when executing commands that require elevated privileges.

```
Command flow with on-demand escalation:

┌─────────────┐     ┌──────────────┐     ┌─────────────────┐
│ Classify    │ ──► │ Needs        │ NO  │ Execute in      │
│ command     │     │ privilege?   │ ──► │ current mode    │
└─────────────┘     └──────────────┘     └─────────────────┘
                           │ YES
                           ▼
                    ┌──────────────┐     ┌─────────────────┐
                    │ Already      │ YES │ Execute         │
                    │ escalated?   │ ──► │ command         │
                    └──────────────┘     └─────────────────┘
                           │ NO
                           ▼
                    ┌──────────────┐     ┌─────────────────┐
                    │ Escalate     │ ──► │ Execute         │
                    │ (enable)     │     │ command         │
                    └──────────────┘     └─────────────────┘
```

## Driver Extension

Extend existing `NetworkDeviceDriver` class with SSH hints:

```cpp
struct SSHDriverHints
{
   const char* promptPattern;        // "^[\\w.-]+[>#$]\\s*$"
   const char* enableCommand;        // "enable"
   const char* enablePrompt;         // "Password:"
   const char* paginationDisable;    // "terminal length 0"
   const char* exitCommand;          // "exit"
   uint32_t commandTimeout;          // Default timeout
   bool requiresPTY;                 // true for network devices
};

class NetworkDeviceDriver
{
   // Existing SNMP methods...

   // NEW: SSH support
   virtual bool hasSSHSupport() { return false; }
   virtual SSHDriverHints getSSHDriverHints();
   virtual SSHCommandType classifyCommand(const char* cmd);
   virtual json_t* parseCommandOutput(const char* cmd, const char* output);
};
```

## Output Processing Pipeline

```
SSH Output → Driver Extractor (optional) → Size Check → Summarize if large → Main Agent
```

| Stage | Action |
|-------|--------|
| 1. Driver extractor | Pattern-based extraction if driver provides it; pass-through otherwise |
| 2. Size check | If < 2KB: pass through unchanged |
| 3. Summarization | If > threshold: LLM extracts key findings, notes full output available |
| 4. Main agent | Receives processed output, can request full if needed |

### Detailed Pipeline

```
┌─────────────────────────────────────────────────────────────────┐
│  SSH Command Execution                                          │
└─────────────────────┬───────────────────────────────────────────┘
                      │ raw output
                      ▼
┌─────────────────────────────────────────────────────────────────┐
│  Stage 1: Driver Extractor (optional)                           │
│  - Pattern-based extraction if driver provides it               │
│  - Structured data extraction for known command types           │
│  - Pass-through if no extractor defined                         │
└─────────────────────┬───────────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────────┐
│  Stage 2: Size Check + Conditional Summarization                │
│  - If output < threshold (e.g., 2KB): pass through unchanged    │
│  - If output > threshold: LLM summarization                     │
│    - Extract key findings, errors, anomalies                    │
│    - Note that full output is available if needed               │
└─────────────────────┬───────────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────────┐
│  Stage 3: Main Agent                                            │
│  - Receives processed output                                    │
│  - Can request full output if summary insufficient              │
└─────────────────────────────────────────────────────────────────┘
```

## Audit Logging

### Dedicated Table: `ai_ssh_audit`

| Column | Type | Description |
|--------|------|-------------|
| id | integer | Primary key |
| timestamp | integer | Unix timestamp |
| user_id | integer | NetXMS user who initiated |
| node_id | integer | Target node object ID |
| command | varchar(2000) | Executed command |
| command_class | char(1) | R=read, W=write, D=dangerous |
| intent | varchar(500) | AI declared intent |
| approval_status | char(1) | P=pending, A=approved, R=rejected, N=not required |
| approved_by | integer | User ID who approved |
| approval_time | integer | When approved |
| exec_result | char(1) | S=success, F=failed, X=not executed |
| exec_error | varchar(500) | Error message if failed |
| session_id | integer | For correlating commands in conversation |

### SQL Schema

```sql
CREATE TABLE ai_ssh_audit
(
   id integer not null,
   timestamp integer not null,
   user_id integer not null,
   node_id integer not null,
   command varchar(2000) null,
   command_class char(1) not null,
   intent varchar(500) null,
   approval_status char(1) not null,
   approved_by integer null,
   approval_time integer null,
   exec_result char(1) not null,
   exec_error varchar(500) null,
   session_id integer null,
   PRIMARY KEY(id)
);
```

## Error Handling

| Scenario | Policy |
|----------|--------|
| Connection failure | Retry up to 2 times |
| Command failure | No automatic rollback (too risky) |
| Consecutive failures | Escalate to human after 3 failures on same target |

## Module Structure: aissh

### Functions (Always Available)

- `ssh-execute-command` - Core command execution
- `ssh-check-connection` - Test SSH connectivity

### Skills (Loaded on Demand)

- `ssh-diagnostics` - Server/device diagnostics skill
- `ssh-network-control` - Network device configuration skill

### Prompt Files

- `ssh-diagnostics.md` - Guidance for diagnostic operations
- `ssh-network-control.md` - Guidance for network device operations

## Implementation Phases

### Phase 1: Agent-User Synchronous Interaction

*Generic mechanism for AI agent to ask questions and receive user input*

- Extend IRIS core with synchronous question/response capability
- Chat protocol extension for agent-initiated questions
- NXMC UI support for answering agent questions
- Timeout handling for unanswered questions
- Question types: yes/no, multiple choice, free text, approval request

*This enables any AI function to request user input, not just SSH approvals.*

### Phase 2: SSH Interactive Sessions

*Extend SSH subagent with PTY/shell support for network devices*

- New `SSHInteractiveSession` class in agent subagent
- PTY + shell channel setup
- Prompt detection and waiting
- Command execution with prompt-based output capture
- Pagination handling
- Privilege escalation support (enable mode)
- Session pool integration for interactive sessions
- Agent protocol extension for interactive mode requests

*This is pure SSH infrastructure, no AI involvement yet.*

### Phase 3: aissh Module Foundation

*Core AI SSH module with basic functionality*

- aissh module skeleton
- Basic SSH functions: `ssh-execute-command`, `ssh-check-connection`
- Command classification (hardcoded patterns)
- Audit table (`ai_ssh_audit`) and logging
- Integration with Phase 1 for approval workflow
- Integration with Phase 2 for interactive sessions
- Basic skills: `ssh-diagnostics`

### Phase 4: Network Device Driver Extension

*Extend drivers with SSH CLI support*

- `SSHDriverHints` structure
- Driver base class extension
- Implement hints for major vendors:
  - Cisco IOS/IOS-XE
  - Juniper JunOS
  - Huawei VRP
  - MikroTik RouterOS
  - Generic Linux/Unix
- Prompt auto-detection fallback
- Optional output extractors per driver
- Driver-based command classification hints

### Phase 5: Advanced Features

*Intelligence and autonomy enhancements*

- Intent-based command classification with AI
- Output summarization pipeline (large output → LLM summary)
- Background task approval queue
- Context-dependent autonomy rules (device criticality, severity, time)
- Advanced skills: `ssh-network-control`, `ssh-troubleshooting`
- Multi-command operation plans with batch approval

### Phase Dependencies

```
┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│  Phase 1: Agent-User          Phase 2: SSH Interactive          │
│  Sync Interaction             Sessions                          │
│                                                                 │
└───────────┬───────────────────────────┬─────────────────────────┘
            │                           │
            └─────────────┬─────────────┘
                          │
                          ▼
            ┌─────────────────────────────┐
            │  Phase 3: aissh Module      │
            │  Foundation                 │
            └─────────────┬───────────────┘
                          │
                          ▼
            ┌─────────────────────────────┐
            │  Phase 4: Driver Extension  │
            └─────────────┬───────────────┘
                          │
                          ▼
            ┌─────────────────────────────┐
            │  Phase 5: Advanced Features │
            └─────────────────────────────┘
```

Phases 1 and 2 can be developed in parallel. Phases 3-5 are sequential.
