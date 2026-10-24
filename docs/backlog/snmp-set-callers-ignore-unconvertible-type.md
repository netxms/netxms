---
worth: later
added: 2026-10-25
---
# server SNMP set/trap callers send NULL for unconvertible value types

NXSL `SNMPSet` (`nxsl_classes.cpp`, `nxslext.cpp`), the snmptrap notification channel driver and the
EPP trap sender (`ef_snmptrap.cpp`) resolve a type name with `SnmpResolveDataType()` and pass it to
`SNMP_Variable::setValueFromString()`. For types it cannot convert (FLOAT, DOUBLE, INTEGER64,
UINTEGER64, OPAQUE, BIT STRING, NSAP ADDRESS, SEQUENCE) the varbind silently becomes NULL. GH #3825
fixed only `nxsnmpset`, which checks `getType() == ASN_NULL` after the conversion and rejects
FLOAT/DOUBLE/INTEGER64/UINTEGER64 outright, since libnxsnmp encodes them with bare tags instead of
wrapping them into Opaque; the server callers would need the same two checks. Open question:
whether anyone sets these types from scripts or trap configs at all — if not, the change is a log line
nobody reads; if so, `SNMPSet` returning `false` is a script-visible behaviour change worth doing.
