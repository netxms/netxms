# Align server-side TimeSpan formatting with the client (issue #3999)

## Overview

Since af5ed34149 the Java clients format `TimeSpan` DCI values with `TimeFormatter.formatTimeSpan` (`[-][n days, ]hh:mm[:ss]`), while the server uses the C++ `FormatTimeSpan()` behind NXSL `FormatTimeSpan()`, event macros `%<{u}name>` (`DCObjectInfo::formatValue`) and Grafana summary tables (`FormatDCIValue` via `Table::toGrafanaJson`). The C++ output always prints the day part, always "days", space-pads hours, rounds to the minute in minutes mode where Java truncates, and leaves fractional values raw. After this change these paths produce the same text, except Grafana columns with multipliers disabled (mode 2 of the three-argument `FormatDCIValue`), which return the raw value plus unit by design.

`FormatTimeSpan()` landed in 8c01a731e6 (2026-11-16), after the `7.1` tag (2026-11-05), so its output is unreleased and can change without a compatibility cost. Only `SecondsToUptime` has released (7.1) output that scripts may parse.

## Decisions

- **Format** (both C++ and Java): `[-][N day|N days, ]hh:mm[:ss]`. The day part appears only when days > 0; the time part is always printed (86400 → `1 day, 00:00:00`), matching the console format that predates af5ed34149. Dropping a zero time part ("1 day") was considered and rejected: a third format neither side ever produced, variable width.
- **C++ day text** is English singular/plural; the server has no localization. Java keeps its localized `dayText` callback.
- **Rounding**: minutes mode (`withSeconds` / `showSeconds` false) rounds to the nearest minute in both implementations, on the absolute value, so exact halves round away from zero (`±30` → `±00:01`). Do not use `Math.round` on signed seconds: it rounds negative halves the other way. Rounding can carry into the day part: 86370 → `1 day, 00:00`. On the Java side no production caller uses minutes mode today (both `DateFormatFactory` adapters pass `true`, and `formatTimeDifference(…, false)` has no external callers), so the change is for parity and future callers.
- **C++ arithmetic**: keep the unsigned absolute-value idiom, use a 64-bit day count and split with division/modulo. The current `arg -= d * 86400` multiplies at 32-bit width and is wrong past ~49710 days; the rewrite must not copy it.
- **Java extrema not handled**: `abs + 30` overflows only within 30 s of `Long.MAX_VALUE` (~2.9e11 years). Per the "no defensive code for unreachable states" rule, no guard is added.
- **Sign** follows the input, with no special case for a zero result: `-20` in minutes mode gives `-00:00` on both sides.
- **Fractional values**: `FormatDCIValue` parses `TimeSpan` values with `_tcstod`, keeping the full-consumption rule (trailing garbage → raw). A value is formatted only when it lies in `[-2^63, 2^63)`, written as a range comparison against `9223372036854775808.0`, which also rejects NaN; otherwise it is returned raw. This is a deliberate departure from Java, which turns NaN into 0 and clamps infinity: an agent returning `nan` or `inf` should show that, not `00:00:00`. The cast then truncates toward zero, like Java's `(long)Double.parseDouble(value)`. Parsing is locale-safe because `InitNetXMSProcess` sets `LC_NUMERIC` to `C`. Lexical parity with `Double.parseDouble` is not a goal: trailing whitespace and type suffixes differ. The `Epoch time` branch keeps integer parsing (out of scope).
- **One parse point**: `DCObjectInfo::formatValue` replaces its `TimeSpan` and `Epoch time` branches, in the same position (before multiplier handling), with a single `return FormatDCIValue(m_unitName, value)` when `useUnits` is set. Match on `m_unitName`, not the stripped `units`, so names like `TimeSpan (IEC)` are not newly special-cased. Use the two-argument overload, because mode 2 of the three-argument one bypasses the special units. Merging the whole of `formatValue` into `FormatDCIValue` was rejected: they differ in multiplier power (`m_multiplier`) and in the unit exemption lists, so that would change event macro output beyond this issue.
- **Empty macro values**: after delegation, an empty value (a missing named event parameter defaults to `""` in `netobj.cpp`) formats as empty instead of `FormatTimestamp(0)` / a zero time span. This is accepted: printing 1970-01-01 for a missing parameter was wrong, and it matches `FormatDCIValue` and Grafana.
- **`SecondsToUptime`** stops being an alias of `F_FormatTimeSpan` and gets its own handler in `src/libnxsl/functions.cpp`, forward-declared in `env.cpp`. The dispatch entry keeps arity 1 and the deprecated flag, and the handler keeps the numeric argument check. It preserves the 7.1 layout `"%u days, %2u:%02u"` with minute rounding for normal positive uptimes, through a static helper in that file (no libnetxms export for a deprecated function). Negative input keeps the current signed output (`-90061` → `-1 days,  1:01`) rather than the 7.1 `uint64` wrap-around.
- `F_FormatTimeSpan` is unchanged (default `withSeconds = false`).

## Out of scope

- netxms-doc-antora (`formattimespan.adoc`, the TimeSpan row in `dci-reference.adoc`): the user will update it separately.
- Fractional `Epoch time` values; raw `TimeSpan` numbers in the Android app and the Grafana DCI-history mode.

## Implementation Steps

Testing approach: regular (code first, then update the pinned expectations). Each task lands as its own commit, subject ending with `(issue #3999)`.

### Task 1: Server-side formatting (libnetxms, server core, NXSL)

Touches: `src/libnetxms/tools.cpp` (`FormatTimeSpan`, `FormatDCIValue`), `src/server/core/dcobject.cpp` (`DCObjectInfo::formatValue`), `src/libnxsl/functions.cpp` and `src/libnxsl/env.cpp` (`SecondsToUptime`), `doc/nxsl.npp.calltips.xml`, tests in `tests/test-libnetxms/format.cpp` and `tests/test-libnxsl/time.nxsl`.

- [x] `FormatTimeSpan()` produces the format above, with rounding in minutes mode and 64-bit modulo splitting; the function comment describes the new format
- [x] `FormatDCIValue` parses fractional `TimeSpan` values and returns NaN, infinity and out-of-range values raw
- [x] `formatValue` delegates `TimeSpan` and `Epoch time` to `FormatDCIValue`
- [x] `SecondsToUptime` has its own handler with the 7.1 layout
- [x] `FormatTimeSpan` calltip in `doc/nxsl.npp.calltips.xml` describes `[-][n days, ]hh:mm[:ss]` and minute rounding when `withSeconds` is false; the `SecondsToUptime` entry is unchanged
- [x] `format.cpp`: `FormatDCIValue` expectations updated, plus fractional and negative-fraction values, `nan` / `inf` / `1e309` / `1e20` / `9223372036854775807` staying raw, and non-numeric input staying raw. Add direct `FormatTimeSpan` cases for minutes mode (`±29`, `±30`, `±31`, the 86370 carry), since `FormatDCIValue` always requests seconds
- [x] `time.nxsl`: `FormatTimeSpan` expectations updated, with singular vs plural day, zero (`00:00`) and negative below one minute; `SecondsToUptime` cases for zero, under a day, over a day and the minute-rounding carry, plus the existing negative case
- [x] gate: build (the worktree is not configured yet: `./init-source-tree`, `./configure … --with-server --with-tests --enable-debug`, `make -j`) and run `tests/test-libnetxms/test-libnetxms` and `tests/test-libnxsl/test-libnxsl tests/test-libnxsl` (the argument is the script directory); both pass
- [ ] manual check of event macros against a running server (no unit harness covers `DCObjectInfo`), with a `TimeSpan` DCI and an `Epoch time` DCI: `%<{u}name>` formats; `%<{u,m}name>` gives the same as `{u}`; `%<{m}name>` on a `TimeSpan` DCI is unchanged (multiplier path); a missing parameter formats as empty
- ⚠️ manual macro check not run in task 1: it needs an installed server with a database, DCIs and an event, which this run does not set up. `DCObjectInfo` has no unit harness (`test-libnxcore` cannot build one: the unit name comes only from a `DCItem`). The delegation is a single call to `FormatDCIValue`, which `format.cpp` covers; left for the final review.
- ➕ `SecondsToUptime` formats inline in its handler instead of through a separate static helper, since the handler is the only user

### Task 2: Minute rounding in the Java client library

Touches: client library `src/client/java/netxms-client` (`TimeFormatter.formatTimeSpan`, `TimeSpanFormatTest`, `DataFormatterTest`).

- [x] `formatTimeSpan` rounds to the nearest minute on the absolute value when `showSeconds` is false; nothing else changes; the Javadoc mentions the rounding
- [x] `TimeSpanFormatTest` covers `±29`, `±30`, `±31` and the carry into the day part in minutes mode; the existing cases stay valid unchanged
- [x] `DataFormatterTest` TimeSpan test gets a negative fractional value (it uses a fake formatter that returns raw seconds, so this pins the truncation, not the layout)
- [x] gate: `mvn -f src/java-common/netxms-base/pom.xml install` then `mvn -f src/client/java/netxms-client/pom.xml test` passes
- [x] move this plan to `docs/plans/completed/`

## Tandem

| Task | Writer | Reviewer |
|------|--------|----------|
| 1 | claude | codex |
| 2 | codex | claude |
