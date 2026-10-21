# SNMP Library and Tools (libnxsnmp)

> Shared guidelines: see [root CLAUDE.md](../../CLAUDE.md).

## Prefer Binary OIDs in C++ Code

When passing OIDs to `SnmpGet`, `SNMP_Variable`, `CheckSNMPIntegerValue`, etc., use the binary form — a `uint32_t oid[N]` array, an `SNMP_ObjectId`, or the `{1, 3, 6, ...}` `std::initializer_list<uint32_t>` overload. Do **not** build a text OID (`_T(".1.3.6...")`) with `_sntprintf` to splice in a per-row index; that re-parses the string on every call.

- Constant OID, no index → pass the `{1, 3, 6, ...}` initializer list directly.
- Per-row OID with a trailing column/index → `uint32_t oid[N] = { ..., 0 }; oid[N-1] = ifIndex;` before the call (canonical example: `getInterfaceState` in `src/server/libnxsrv/ndd.cpp`).

`SnmpGet` has a `const uint32_t *oidBinary, size_t oidLen` overload precisely for this.
