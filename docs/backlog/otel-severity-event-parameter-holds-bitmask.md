---
worth: later
where: src/server/core/otellog.cpp
added: 2026-11-17
---
# otelSeverity event parameter carries the parser level bitmask

Events generated from OpenTelemetry log records get a named parameter `otelSeverity` whose value is the internal log parser level (`1 << severity`: 1, 2, 8, 16), not an OpenTelemetry or NetXMS severity code. `otelSeverityNumber` carries the real number. The new docs leave `otelSeverity` out until this is decided.

The syslog callback (`SyslogParserCallback`, `syslogd.cpp`) exposes the same `1 << severity` parser level as its `severity` parameter, so the value follows an existing convention rather than being plainly wrong. Unknown that settles it: what domain the `otelSeverity` parameter is meant to have (NetXMS severity, OTel severity text, or the parser level), and whether changing it is worth breaking EPP rules that compare against the current values.

Found while documenting netxms/netxms-doc#67 in netxms-doc-antora.
