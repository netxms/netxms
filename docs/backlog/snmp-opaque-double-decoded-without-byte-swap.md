---
worth: yes
where: src/snmp/libnxsnmp/variable.cpp:270
added: 2026-10-25
---
# opaque-wrapped SNMP double is decoded without byte swap

`SNMP_Variable::decodeContent()` routes `ASN_FLOAT` and the 64-bit integer types to
`BER_DecodeContent()`, but has no `ASN_DOUBLE` case. An opaque-wrapped double (reached through
`decodeOpaque()`) falls into `default:`, which copies the raw network-order bytes, so
`getValueAsString()` / `getValueAsDouble()` read them as a host double — garbage on little-endian hosts.
`BER_DecodeContent()` already handles `ASN_DOUBLE` with `ntohd()`; the fix is the missing case with
`m_valueLength = sizeof(double)`. Found while working out the fix for GH #3825.
