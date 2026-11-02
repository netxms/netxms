---
worth: yes
where: src/libnetxms/array.cpp:417
added: 2026-11-02
---
# `Array::bsearch` and `NXSL_Array::get` call `bsearch()` with a null array

`Array::bsearch` (`src/libnetxms/array.cpp:417`) passes `m_data` straight to `::bsearch`, and `m_data` is null
for an array built with initial size 0 that has no elements. `NXSL_Array::get` (`src/libnxsl/array.cpp:127`)
does the same, while `NXSL_Array::set` at line 188 of that file guards with `m_size > 0`.

Reachable through an empty FDB: `ForwardingDatabase` builds `m_fdb(0, ...)` (`fdb.cpp:61`) and `findMacAddress`
(`fdb.cpp:109`) searches it. Also through an empty package table in `CalculatePackageChanges` (`swpkg.cpp:58`).

This is the undefined behaviour eb74b2f53d removed from `Group`. libc reads nothing at size 0, so it shows up as a
sanitizer report rather than a crash.

Guarding `Array::bsearch` also makes `FindMember` in `userdb_objects.cpp` unnecessary: `IntegerArray` inherits
`Array::bsearch`, so `Group` can call `m_members.bsearch(&userId, CompareUserId)`.

Same code on stable-6.2. Surfaced in review of #3861.
