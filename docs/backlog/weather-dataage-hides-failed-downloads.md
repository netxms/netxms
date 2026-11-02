---
worth: later
where: src/agent/subagents/weather/location.cpp:92
added: 2026-10-30
---
# `Weather.DataAge` no longer shows failed forecast downloads

Since #3845 (`e6a6e59378`, stable-6.2 `abab22a269`) the current values are re-selected from the cached forecast on
every poll, and a failed download leaves that forecast in place. So `Weather.DataAge` stays within the hour plus
`PollInterval` while downloads fail, and starts to grow only when the cached forecast runs out, up to `ForecastDays`
days later. No metric reports a failed download, so a threshold on `Weather.DataAge` is the only check and it fires
late.

The admin guide page `data-collection/weather-data.adoc` states this behaviour, so a change here needs a docs update.

Unknown that settles the value: whether a delay of up to `ForecastDays` days matters to users, or a separate forecast
age metric is worth adding.
