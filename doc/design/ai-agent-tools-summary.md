# NetXMS Agent AI Tools - Development Summary

## Concept Overview

A dedicated subagent that exposes a set of tools specifically designed for AI assistant consumption. The tools are self-describing - the agent provides both execution capability and JSON schemas that map directly to LLM tool definitions.

## Architecture

### Agent Interface

Two primary endpoints:

1. **AI.GetTools()** - Returns JSON schema describing all available tools in standard LLM tool format
2. **AI.Execute(tool_call_json)** - Executes a tool and returns structured result

### Self-Describing Tools

The agent advertises its capabilities in a format directly consumable by LLMs:

```json
{
  "schema_version": "1.0",
  "tools": [
    {
      "name": "tool_name",
      "category": "category",
      "description": "Description for LLM to understand when/how to use",
      "parameters": {
        "type": "object",
        "properties": { ... },
        "required": [ ... ]
      }
    }
  ]
}
```

### Server-Side Flow

1. Server calls `AI.GetTools()` on target node
2. Caches result, merges with server-side tools (database queries, topology, etc.)
3. Combined tool list passed to LLM
4. LLM generates tool calls → server routes to appropriate executor (agent or local)
5. Results returned to LLM for interpretation

### Benefits

- **Self-describing**: Server doesn't hardcode tool knowledge, queries agent dynamically
- **Versionable**: Different agent versions can expose different tools
- **Extensible**: Custom tools can be added via subagent configuration
- **Platform-aware**: Windows agent exposes different tools than Linux (Event Log vs syslog)
- **Security boundary**: Agent controls what's exposed, can restrict paths/operations

## Tool Categories

### Log Tools

Primary use case: investigating large log files without transferring entire contents.

| Tool | Purpose |
|------|---------|
| `log_grep` | Search log file for pattern matches with optional context lines |
| `log_read` | Read specific line range by line numbers |
| `log_tail` | Read last N lines from log file |
| `log_head` | Read first N lines from log file |
| `log_find` | Find line numbers matching pattern (positions only, no content) |
| `log_time_range` | Extract entries within time range (requires timestamp format) |
| `log_stats` | Pattern frequency statistics, common error counts |
| `compressed_grep` | Search inside .gz, .bz2, .xz compressed files |
| `multi_file_grep` | Search across multiple files including rotated logs |
| `json_log_query` | Query JSON-formatted logs, filter by field values |

### Filesystem Tools

| Tool | Purpose |
|------|---------|
| `file_info` | Get metadata: size, line count, mtime, permissions |
| `file_list` | List directory contents with glob pattern matching |
| `file_read` | Read text file contents (with byte offset/limit) |
| `config_read` | Read config file with automatic sensitive data masking |

### System Tools

| Tool | Purpose |
|------|---------|
| `process_list` | List running processes with filtering and sorting |
| `process_info` | Detailed process info: open files, connections, environment |
| `network_connections` | List network connections (netstat/ss equivalent) |
| `service_status` | System service status (systemd/Windows services) |
| `command_exec` | Execute predefined whitelisted commands only |

## Key Design Decisions

### Safety and Limits

All tools enforce:
- Maximum lines/bytes per response (configurable)
- Maximum matches returned
- Execution timeouts
- Path restrictions (configurable allowed directories)
- Sensitive data masking in config files

### Response Format

Standardized JSON response for all tools:

```json
{
  "success": true,
  "tool": "tool_name",
  "result": { ... },
  "execution_time_ms": 145
}
```

Error response:

```json
{
  "success": false,
  "tool": "tool_name",
  "error": {
    "code": "ERROR_CODE",
    "message": "Human-readable message"
  }
}
```

### Tool Parameters

All tools use JSON Schema format for parameters with:
- Type definitions
- Default values
- Min/max constraints
- Clear descriptions (important for LLM understanding)
- Required vs optional distinction

## Implementation Considerations

### Subagent Structure

- Single subagent handles all AI tools
- Dispatcher routes to specific tool implementations
- Schema generated from tool definitions (single source of truth)
- Configuration for:
  - Allowed paths/directories
  - Whitelisted commands
  - Resource limits
  - Enabled/disabled tools

### Platform Differences

Linux-specific:
- systemd service queries
- /proc filesystem access
- Standard log paths (/var/log)

Windows-specific:
- Event Log queries (separate tool needed)
- Registry reading (if needed)
- Windows service queries
- Different path conventions

### Future Extensions

Potential additional tools:
- Event Log query (Windows)
- Registry read (Windows)
- Journal query (systemd journal)
- Container log access (Docker/Podman)
- Database query tools (if agent has DB access)
- Certificate/TLS inspection
- Package/software inventory

## Development Tasks

1. **Core subagent framework**
   - Tool registration mechanism
   - Schema generation from definitions
   - Dispatcher implementation
   - Configuration loading

2. **Tool implementations**
   - Start with log tools (highest value)
   - Add filesystem tools
   - Add system tools
   - Platform-specific variants

3. **Server integration**
   - Protocol for GetTools/Execute
   - Tool schema caching
   - Routing tool calls to agents
   - Merging agent tools with server-side tools

4. **Security**
   - Path validation and restrictions
   - Sensitive data masking
   - Command whitelist enforcement
   - Rate limiting / resource protection

5. **Testing**
   - Unit tests for each tool
   - Integration tests with actual LLM
   - Performance testing with large files
   - Security testing (path traversal, etc.)
