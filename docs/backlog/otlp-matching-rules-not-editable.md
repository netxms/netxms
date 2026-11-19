---
worth: yes
where: src/server/otlp/matching.cpp
added: 2026-11-17
---
# OTLP resource matching rules cannot be edited and are loaded only at startup

`otlp_matching_rules` has no client UI, NXCP command or Web API endpoint, and `ReloadMatchingRules()` is never called, so changing how OTLP resources map to nodes means editing the database by hand and restarting the server.

Found while documenting netxms/netxms-doc#67 in netxms-doc-antora.
