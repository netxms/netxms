---
worth: yes
where: src/server/core/dcitem.cpp:2739
added: 2026-10-25
---
# DCItem::getLastValue returns pointer into cache after unlocking

`getLastValue()` returns `m_ppValueCache[0]->getString()` after releasing the DCI lock. A concurrent poll
that fills the cache (`dcitem.cpp:1558`, cache size 1 is the common case) deletes that `ItemValue`, so the
caller reads freed memory. Callers: summary table building in `dctarget.cpp:2184` and `:2201`.

Fix is to return a copy (`String` / `SharedString`) or copy under the lock, as `getInternalLastValue()`
already does. Surfaced by Codex during the server memory review.
