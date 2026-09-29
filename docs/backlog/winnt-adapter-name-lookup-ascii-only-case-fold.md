---
worth: yes
where: src/agent/subagents/winnt/net.cpp:154
added: 2026-09-29
---
# winnt adapter name lookup folds case for ASCII only

`AdapterNameToIndex` compares `IP_ADAPTER_ADDRESSES::FriendlyName` against the requested name with CRT
`_wcsicmp` directly (lines 154 and 156). The agent never sets `LC_CTYPE` on Windows, so this folds only
ASCII. A localized friendly name ("Подключение по локальной сети") passed in a different case does not
match, and the `Net.Interface.*(name)` metrics return no such instance.

Fix is mechanical: switch to `wcsicmp`, which `nms_common.h` maps to `nx_wcsicmp`. The subagent already
links libnetxms. Deferred from the Windows `_tcsicmp` remap in `include/unicode.h`, where it was out of
scope because it bypasses the macros.
