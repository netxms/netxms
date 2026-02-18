# WebAPI Component

> Shared guidelines: See [root CLAUDE.md](../../../CLAUDE.md) and [server CLAUDE.md](../CLAUDE.md) for C++ development guidelines, build commands, and contribution workflow.

## Overview

The WebAPI module (`webapi.nxm`) is a server module that provides REST API endpoints for NetXMS. It is loaded by the server at startup and registers HTTP routes using the `RouteBuilder` API. The module uses `libmicrohttpd` for HTTP handling.

## Build Commands

```bash
# Build webapi module only (after full configure)
make -C src/server/webapi

# Full server build (includes webapi)
make -C src/server
```

## Key Files

| File | Description |
|------|-------------|
| `webapi.h` | Common header, includes `nms_core.h` and `netxms-webapi.h` |
| `main.cpp` | Module entry point, route registration |
| `openapi.yaml` | OpenAPI 3.0 specification for all endpoints |
| `alarms.cpp` | Alarm management endpoints |
| `datacoll.cpp` | Data collection and history endpoints |
| `dcst.cpp` | DCI summary table endpoints |
| `events.cpp` | Event template endpoints |
| `find.cpp` | Network search endpoints |
| `grafana.cpp` | Grafana integration endpoints |
| `info.cpp` | Server info and status endpoints |
| `objects.cpp` | Object management endpoints |
| `objtools.cpp` | Object tool endpoints |
| `scripts.cpp` | Script library endpoints |
| `tcpproxy.cpp` | TCP proxy / WebSocket endpoints |
| `ai.cpp` | AI chat endpoints |

## Adding New Endpoints

1. Create or edit a handler source file (e.g., `myresource.cpp`).
2. Implement handler functions with signature `int H_MyHandler(Context *context)`.
3. Declare the handler in `main.cpp` and register routes using `RouteBuilder`.
4. Add the source file to `Makefile.am` (alphabetical order in `webapi_la_SOURCES`).
5. **Update `openapi.yaml`** to document the new or modified endpoints.

### Handler Pattern

```cpp
#include "webapi.h"

int H_MyHandler(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_...))
      return 403;

   // ... implementation ...

   json_t *output = json_object();
   // ... populate JSON ...
   context->setResponseData(output);
   json_decref(output);
   return 200;
}
```

### Route Registration (in `main.cpp`)

```cpp
RouteBuilder("v1/my-resource")
   .GET(H_MyResourceList)
   .POST(H_MyResourceCreate)
   .build();
RouteBuilder("v1/my-resource/:resource-id")
   .GET(H_MyResourceDetails)
   .PUT(H_MyResourceUpdate)
   .DELETE(H_MyResourceDelete)
   .build();
```

## API Documentation

When changes are made to API endpoints (adding, modifying, or removing), the OpenAPI specification in `openapi.yaml` must also be updated to reflect those changes. This file serves as the authoritative API reference for external consumers.

## HTTP Response Conventions

| Code | Usage |
|------|-------|
| 200 | Successful GET or PUT |
| 201 | Successful POST (resource created) |
| 204 | Successful DELETE (no content) |
| 400 | Invalid request or parameters |
| 403 | Insufficient access rights |
| 404 | Resource not found |
| 409 | Conflict (e.g., duplicate name) |
| 500 | Internal server error / database failure |

## Access Control

Use `context->checkSystemAccessRights()` with constants from `nxcldefs.h` (e.g., `SYSTEM_ACCESS_VIEW_EVENT_DB`, `SYSTEM_ACCESS_EDIT_EVENT_DB`). Return 403 if the check fails.

## Audit Logging

Use `context->writeAuditLog()` for simple audit entries and `context->writeAuditLogWithValues()` when old/new values should be recorded. Common subsystem: `AUDIT_SYSCFG` for system configuration changes, `AUDIT_OBJECTS` for object operations.

## Related Components

- [Server core](../CLAUDE.md) - Server core that exports functions called by handlers
- [libnetxms](../../libnetxms/CLAUDE.md) - Core utility library
- [libnxdb](../../db/CLAUDE.md) - Database abstraction
