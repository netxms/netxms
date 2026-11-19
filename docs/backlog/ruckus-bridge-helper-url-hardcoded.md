---
worth: later
where: src/server/wlcbridge/ruckus.cpp
added: 2026-11-17
---
# Ruckus bridge talks to a hard-coded helper at 127.0.0.1:5000 that is not shipped

The Ruckus bridge calls `http://127.0.0.1:5000/` (`inventory`, `clients` paths) with no attribute to change the address, and no helper service ships with the product, so all Ruckus domains share one local helper that users must supply themselves. Docs say only that an external helper is needed.

What settles it: is the Ruckus bridge meant for general use (then make the URL configurable and ship or document the helper) or internal only (then say so)?

Found while documenting netxms/netxms-doc#91 in netxms-doc-antora.
