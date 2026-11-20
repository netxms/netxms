---
worth: later
where: src/agent/subagents/prometheus/main.cpp
added: 2026-11-20
---
# Prometheus remote write receiver is enabled by default on port 9090 and its failure unloads the subagent

`EnableReceiver` defaults to yes, `Port` to 9090, the default port of the Prometheus server itself. If
the listener cannot bind (port in use, bad address, `Port = 0`), subagent init fails, so scraping is lost
too even when only scrape targets are configured.

Open question: default the receiver to off (scrape-only users are the common case), or keep it on and
only log the bind failure while scraping continues. The admin guide (`monitoring/prometheus-monitoring.adoc`,
"Enabling Prometheus Monitoring") documents the current behaviour; update it if this changes.
