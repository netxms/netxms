---
worth: yes
where: src/agent/subagents/weather/location.cpp:68
added: 2026-10-26
---
# weather subagent: current values not re-selected from the cached forecast

`WeatherLocation::poll()` replaces `m_current` only when a 200 response is parsed. While the cached
response is within its `Expires` window the request is skipped, and on 304 it only logs, so the current
entry is never re-picked from the cached hourly curve as time moves on. For a `metno` location outside the
Nordic area (new forecast about every 6 hours) the current values lag by hours and `Weather.DataAge`
climbs to ~6 h in healthy operation; Bright Sky's snapshot likewise stays on the hour of the last full
response. Fix: on every poll, re-select the entry at or before `now` from the cached curve (the parsers
already apply that rule to a fresh response). Once fixed, the admin guide page
`data-collection/weather-data.adoc` (netxms-doc-antora) can drop its 6-hour DataAge caveat. Surfaced by
the doc review for netxms-doc-antora#40.
