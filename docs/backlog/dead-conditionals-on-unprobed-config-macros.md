---
worth: yes
added: 2026-11-27
---
# dead preprocessor conditionals on macros configure no longer defines

A few `#if` tests read macros that nothing in configure or the tree defines any more, so they always take
the same branch:

- `include/nms_util.h`: `HAVE_HTONLL` / `HAVE_NTOHLL` select `htonll()` / `ntohll()` for `htonq` /
  `ntohq`. Configure has never probed them; `bswap_64` is always used.
- `include/symbol_visibility.h`: `DLL_DECLSPEC_SUPPORTED` (probe dropped with the HP-UX code in
  0fdcc06a13) and `VISIBILITY_ATTRIBUTE_SUPPORTED` (probe dropped in 53469411f7). The `_WIN32` and
  `__GNUC__ >= 4` / `__ibmxl__` arms already cover every supported compiler.

Removing the dead arms changes no build output. Found by a `-Wundef` scan of the tree; to recheck, list
identifiers used by value in `#if`/`#elif` and look for ones with no `#define`, no `config.h.in` template
and no `-D` in `Makefile.am` / `configure.ac`.
