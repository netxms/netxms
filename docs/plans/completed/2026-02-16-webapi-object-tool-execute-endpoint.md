# WebAPI Object Tool Execution Endpoint

## Overview

Add a `POST /v1/object-tools/:tool-id/execute` endpoint to the WebAPI that executes server-capable object tools (agent action, server command, server script, table tools, SSH command) directly, without requiring the client to fetch tool source and orchestrate execution. Client-only tool types (URL, local command, file download) will be rejected with an appropriate error.

## Context

- Files involved:
  - `src/server/webapi/objtools.cpp` - add execute handler
  - `src/server/webapi/main.cpp` - register new route
  - `src/server/core/objtools.cpp` - reuse existing ExecuteTableTool, add new helper functions
  - `src/server/include/nms_core.h` - declare new exported functions
  - `src/server/webapi/openapi.yaml` - document the new endpoint
- Existing server infrastructure: The server already has full execution support for all server-side tool types via NXCP handlers in session.cpp. The WebAPI needs to call the same underlying functions but return JSON instead of NXCP messages.
- Key constraint: WebAPI is synchronous request/response (no NXCP streaming). Output must be collected and returned in a single JSON response.

## Design Decisions

### Synchronous vs Async

The WebAPI will use a synchronous approach - execute the tool and return the result in the response body. This is simpler and consistent with existing endpoints like `execute-script` and `execute-agent-command`. For tools that produce output, the output is collected in memory and returned at once. A reasonable timeout will be applied.

### Tool type support matrix

| Tool Type | Supported | Output Format |
|-----------|-----------|---------------|
| action (agent action) | Yes | text (if GENERATES_OUTPUT) or no output |
| server-command | Yes | text (if GENERATES_OUTPUT) or no output |
| server-script | Yes | text (script print output + result) |
| snmp-table | Yes | table (JSON) |
| agent-list | Yes | table (JSON) |
| agent-table | Yes | table (JSON) |
| ssh-command | Yes | text (if GENERATES_OUTPUT) or no output |
| url | No (client-only) | 400 error |
| local-command | No (client-only) | 400 error |
| file-download | No (client-only) | 400 error |
| internal | No (client-only) | 400 error |

### Request/Response Format

Request:
```json
POST /v1/object-tools/:tool-id/execute
{
  "objectId": 123,
  "alarmId": 0,
  "inputFields": {"field1": "value1"}
}
```

Response (text output):
```json
{
  "type": "text",
  "output": "command output here..."
}
```

Response (table output):
```json
{
  "type": "table",
  "table": { ... }
}
```

Response (no output):
HTTP 200 with `{"type": "none"}`

## Development Approach

- **Testing approach**: Regular (code first, then tests)
- Complete each task fully before moving to the next
- Minimize changes to server core - reuse existing functions wherever possible

## Implementation Steps

### Task 1: Add new server-side helper functions for WebAPI tool execution

**Files:**
- Modify: `src/server/core/objtools.cpp`
- Modify: `src/server/include/nms_core.h`

These new functions will execute tools and return results as JSON, bypassing the NXCP message layer. They reuse the same underlying agent connections, SNMP calls, and script VMs that the existing session handlers use.

- [x] Add `ExecuteTableToolToJSON(uint32_t toolId, const shared_ptr<Node>& node)` function that executes SNMP table, agent table, or agent list tools and returns `json_t*` with the table result. Reuse GetSNMPTable/GetAgentTable/GetAgentList logic but collect results into JSON instead of NXCP message.
- [x] Add `GetObjectToolType(uint32_t toolId, int *toolType, TCHAR **toolData, uint32_t *flags)` function to load tool metadata from DB for the execute handler.
- [x] Declare the new functions in `nms_core.h`

### Task 2: Implement the execute endpoint handler

**Files:**
- Modify: `src/server/webapi/objtools.cpp`

The main handler `H_ObjectToolExecute` will:

- [x] Parse tool-id from path, objectId from request body
- [x] Load tool type and data via GetObjectToolType
- [x] Check tool ACL via existing CheckObjectToolAccess
- [x] Find target object and check OBJECT_ACCESS_CONTROL rights
- [x] Reject unsupported tool types (url, local-command, file-download, internal) with 400
- [x] For agent action: expand macros in tool data, split command line, create agent connection, execute action. If GENERATES_OUTPUT flag set, use executeCommand with callback to collect output into a StringBuffer. Return as text.
- [x] For server-command: expand macros in tool data, create ProcessExecutor, run synchronously with output capture, return text output
- [x] For server-script: expand macros in tool data, get script from library, create VM with NXSL_ServerEnv, run, collect print output via custom environment, return text output + result
- [x] For table tools (snmp-table, agent-table, agent-list): call ExecuteTableToolToJSON, return table
- [x] For SSH command: create agent connection to SSH proxy, execute, collect output, return text
- [x] Write audit log on execution
- [x] Handle alarm context and input fields for macro expansion

### Task 3: Register the route and declare the handler

**Files:**
- Modify: `src/server/webapi/main.cpp`

- [x] Add handler declaration: `int H_ObjectToolExecute(Context *context);`
- [x] Register route: `RouteBuilder("v1/object-tools/:tool-id/execute").POST(H_ObjectToolExecute).build();`

### Task 4: Update OpenAPI specification

**Files:**
- Modify: `src/server/webapi/openapi.yaml`

- [x] Add POST /v1/object-tools/{tool-id}/execute endpoint definition
- [x] Document request schema (objectId, alarmId, inputFields)
- [x] Document response schemas for text output, table output, and no output
- [x] Document error responses (400 for unsupported tool type, 403 for access denied, 404 for tool/object not found, 500 for execution error)

### Task 5: Verify and test

- [x] Verify the code compiles (if build environment available)
- [x] Review all tool type execution paths for correctness
- [x] Verify access control is enforced consistently with existing endpoints
- [x] Verify macro expansion includes alarm context and input fields
