# Core Library (libnetxms)

> Shared guidelines: see [root CLAUDE.md](../../CLAUDE.md).

## Strings

**Choosing a string class for a member field:**
- Set once at construction (ctor arg or move) → `String` (its `operator=` is deleted, so it can't be reassigned later).
- Reassigned wholesale but never appended/inserted → `MutableString` (lightweight assignable variant; supports `operator=` from `String` / `MutableString` / `const TCHAR*`).
- Built up incrementally (append/insert/format) → `StringBuffer`.

## Memory

**Temporary buffers:** for a scratch buffer that should live on the stack when small but fall back to the heap when large, use `Buffer<T, BUFFER_SIZE>` (`nms_util.h`) instead of the hand-rolled `T local[N]; T *p = (len < N) ? local : MemAllocArrayNoInit<T>(len)` + manual `MemFree` pattern — no leak/double-free risk and no size-threshold off-by-one. Its inline storage is `alignas(T)`, so it is also the correct fix for unaligned-access/SIGBUS bugs on strict-alignment CPUs (SPARC, older ARM/MIPS): `memcpy` bytes from an arbitrary stream/wire offset into the `Buffer`, do all `T`-typed work there, and `memcpy` back — never `reinterpret_cast<T*>` into a raw byte stream. Note `BUFFER_SIZE` is in bytes, `reserve()` does not initialize contents, and `operator T*()` makes it a drop-in `T*`.

## Network

For members holding a MAC address or bridge ID, use a `MacAddress` object, not a raw `BYTE[MAC_ADDR_LENGTH]` array — it provides `isValid()`, `equals()`, `parse()`, `toString()`, `toJson()`, and eliminates `memset`/`memcmp`/`BinToStr` boilerplate and zero-MAC sentinels. When persisting to the database, `MacAddressNotation::FLAT_STRING` matches the old `BinToStr` flat-hex format, so on-disk round-trips are preserved when converting existing `BYTE[]` fields.

## JSON (jansson) Helpers

Never `#include <jansson.h>` directly — include `nms_util.h`, which selects the correct (system or bundled) jansson copy and provides `json_t`.

A family of typed convenience helpers wraps raw jansson access — prefer them over manual `json_object_get` + `json_is_*` + `json_*_value` chains. They give consistent default handling and one fewer place to forget a type check:

- `json_object_get_string_t` (allocated `TCHAR*`, caller frees) / `json_object_get_string_utf8` (no-alloc view) / `json_object_get_string` (returns `String`)
- `json_object_get_int32` / `int64` / `uint32` / `uint64` / `json_object_get_boolean` / `json_object_get_time` / `json_object_get_uuid`
- For LLM/AI tool arguments that may arrive stringified, the lenient `json_object_get_array_ex` / `json_object_get_object_ex` (declared in `nms_util.h`).

See the full list near `json_object_get` in `include/nms_util.h`. When emitting a `uuid` as JSON, use `guid.toJson()` (from `include/uuid.h`) — don't format into a `char[]` and wrap in `json_string()` by hand.

**Helper placement** when adding a new shared JSON/argument helper: trivial single-expression helpers go `static inline` directly in `include/nms_util.h`; anything with a non-trivial body or extra dependencies gets a `LIBNETXMS_EXPORTABLE` declaration in the header and its body in `src/libnetxms/tools.cpp`, next to the existing ones. Don't bloat every TU with a heavy `static inline`.

## Character Encoding

**Termination gotcha:** `wchar_to_utf8` / `ucs2_to_utf8` / `ucs4_to_utf8` / `mb_to_utf8` / `tchar_to_utf8` do **not** guarantee null termination when the destination buffer is too small — if conversion stops early because a multi-byte UTF-8 sequence doesn't fit, the buffer is left unterminated. Convert into `bufferSize - 1` and terminate via the return value:

```cpp
size_t bytes = tchar_to_utf8(value, -1, buffer, bufferSize - 1);
buffer[bytes] = 0;
```

## Thread Safety

For trivial accessors to mutex-guarded members, use the `GetAttributeWithLock(attr, mutex)` / `SetAttributeWithLock` templates (`nms_util.h`) as header one-liners (copy under lock, return by value) instead of hand-written LockGuard getter bodies in the .cpp — established pattern: `DCObject` getters in `nms_dcoll.h`. Give the explicit template argument to convert derived types (`MutableString`/`StringBuffer` → `String`). Caveat: raw array members (`wchar_t[]`, `char[]`) can't go through the template — argument binding would construct the copy *outside* the lock; use a LockGuard one-liner (`{ LockGuard g(m_mutex); return String(m_arr); }`) for those.

## Compiler Floor and Dead Version Gates

The supported floor is GCC 4.8, Clang 3.3 and Visual Studio 2015 (`_MSC_VER >= 1900`), with C++11 as the language ceiling. Preprocessor gates that can never be false at these minimums (`__GNUC__ < 4`, `__GNUC__ == 4 && __GNUC_MINOR__ < 8`, `_MSC_VER < 1900` and the like) are dead and may be removed during cleanups. Keep fallbacks for C++14+ features (e.g. `HAVE_STD_MAKE_UNIQUE`) since the ceiling is C++11.

Exception: the Windows XP / pre-2003 64-bit atomic block in `include/nxatomic.h` (guarded by `!defined(_WIN64) && (_WIN32_WINNT < 0x0502)`) is intentionally retained even though the default `_WIN32_WINNT` makes it dead today — a WinXP agent build may still be needed. Do not remove it as obsolete code.
