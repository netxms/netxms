# WebAPI Component

The WebAPI module (`webapi.nxm`) is a server module that provides REST API endpoints on top of `libmicrohttpd`, with routes registered through `RouteBuilder` in `main.cpp`.

## Adding New Endpoints

1. Implement handlers as `int H_MyHandler(Context *context)` in a resource source file.
2. Declare the handler in `main.cpp` and register its routes with `RouteBuilder`.
3. Add a new source file to `webapi_la_SOURCES` in `Makefile.am` (alphabetical).
4. Update `openapi.yaml` for every added, modified or removed endpoint — it is the authoritative API reference for external consumers.

## Modifying Objects — Use `modifyFromJSON`, Not NXCP Translation

Do **not** translate incoming JSON into an `NXCPMessage` just to reuse `modifyFromMessage()` / `updateFromMessage()`. Instead, extend the affected class with a parallel `modifyFromJSON()` / `updateFromJSON()` method that operates on `json_t*` directly (same virtual dispatch, per-class overrides, stage1/stage2 split where applicable). The webapi handler stays thin: load → ACL check → call the JSON method → return the updated representation. No `NXCPMessage` construction belongs in `webapi/` source files.

The JSON interface is becoming the primary interface to the system; a JSON→NXCP shim would tie the REST API to NXCP encoding permanently. Most of `modifyFromMessageInternal` is "is field X present? apply it" — trivially preserved under JSON via `json_object_get(body, key) != nullptr` plus the jansson helpers listed in the root CLAUDE.md.

## Request Parsing Conventions

- **Point-in-time values:** parse string values with `ParseTimestamp()` from libnetxms (`nms_util.h`) at the use site — it accepts ISO 8601, UNIX timestamps as strings, relative offsets (`[+|-]<number>[s|m|h|d]`), and `now`. Detect a parse failure by passing an out-of-band `defaultValue` (`-1`) and testing for it; do not treat a returned 0 as failure, epoch is valid input. Range-check the result where it matters — timestamps are stored in the database as 32 bit values, so a millisecond timestamp or a max-date sentinel from a generated client wraps silently. JSON integers are UNIX timestamps directly. For query parameters use `Context::getQueryParameterAsTime()`, which already delegates to `ParseTimestamp()`. Do not add new shared time-parse helpers to webapi headers.
- **Content types:** `RouteBuilder` accepts `application/json` on every POST/PUT/PATCH route by default; `.acceptProtobuf()` / `.acceptImage()` only widen the accepted set. A binary-only route (protobuf, raw image body) must call `.acceptJson(false)`, otherwise it silently accepts a JSON-labeled body and the handler parses it blindly. Anything not accepted gets 415.

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
