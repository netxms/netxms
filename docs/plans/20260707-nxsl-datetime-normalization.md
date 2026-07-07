# NXSL DateTime: lazy normalization of broken-down time (issue #3388)

## Overview

The NXSL `DateTime` class does not normalize its `struct tm` after a member field is
changed. Setting an out-of-range value produces wrong output instead of rolling over
(`day = 50` prints day 50), and an out-of-range `month` sends `_tcsftime()` past the
month-name table — undefined behavior, empty/garbage output.

Fix: normalize lazily, on read. `setAttr()` keeps writing raw values with
`valid = false`; every output path (`getAttr()`, `format()`, `toString()`) first runs
the pending data through `mktime()`/`timegm()` and refreshes the broken-down time from
the resulting timestamp. This matches standard `mktime` semantics and preserves
batch-field-set behavior that the existing test `tests/test-libnxsl/time.nxsl` depends
on (`isDST = -1; month = 0;` must normalize once, after both writes).

GitHub issue: https://github.com/netxms/netxms/issues/3388

## Context (from discovery)

- `src/libnxsl/time.cpp` — the only implementation file involved:
  - `DateTime` struct (line ~40) with `timestamp`, `utc`, `valid`, `data` and
    `updateFromTimestamp()`
  - `NXSL_DateTimeClass::getAttr()` — returns raw fields; only the `timestamp`
    branch normalizes today
  - `NXSL_DateTimeClass::setAttr()` — raw writes + `valid = false`; `isUTC` branch
    has an inline copy of the normalize logic
  - `format` method and `toString()` — call `_tcsftime()` on un-normalized `data`
- `src/libnetxms/timegm.cpp` — NetBSD fallback used where the platform lacks native
  `timegm` (`configure.ac` AC_CHECK_FUNCS; **Windows always uses the fallback**). It
  does NOT normalize and returns `-1` for `tm_mon` outside 0–11 or year < 1970. Its
  day/hour/min/sec arithmetic is linear, so those fields roll over correctly — only
  month/year need pre-folding on our side.
- `tests/test-libnxsl/time.nxsl` — existing DateTime test, run by
  `tests/test-libnxsl/test-libnxsl.cpp` (`RunTestScript`). The harness pins
  `TZ=EET-02EEST-03,M3.5.0,M10.5.0` so local-time assertions are deterministic.

Decisions made during brainstorm (with alk):

- **Lazy normalization, contained in time.cpp** (approach A). Eager normalization in
  `setAttr()` was rejected: breaks multi-field set semantics and the existing test.
  Fixing the fallback `timegm` itself was deferred as a separate-issue candidate.
- Month/year pre-fold runs unconditionally, not under `#if !HAVE_TIMEGM` — redundant
  on good platforms but identical result, and one code path is easier to trust.
- Unrepresentable input (e.g. `year = 0xfffffff`, or pre-1970 UTC on fallback
  platforms): accept `mktime`/`timegm` returning `-1`; `updateFromTimestamp()` then
  yields a valid 1969-12-31 23:59:59 UTC — wrong-but-defined instead of UB. No error
  propagation to the script.
- `dayOfWeek`/`dayOfYear` setters stay writable but are ignored on input and
  recomputed by normalization (standard `mktime` semantics).

Out of scope (mention in issue when closing, do not fix here):

- `F_mktime()` (time.cpp:455) calls `mktime()` on the raw data ignoring the object's
  `utc` flag.
- Fallback `timegm` semantic divergence from native (affects cert.cpp,
  geolocation.cpp, tools.cpp callers).

## Development Approach

- **testing approach**: Regular (code first, then tests) — single-file C++ change
  with script-driven tests
- complete each task fully before moving to the next
- **CRITICAL: every task MUST include new/updated tests** for code changes in that task
- **CRITICAL: all tests must pass before starting next task**
- **CRITICAL: update this plan file when scope changes during implementation**
- C++11 max; follow libnxsl conventions (3-space indent, brace on new line)

## Testing Strategy

- **unit tests**: extend `tests/test-libnxsl/time.nxsl` (run via the
  `test-libnxsl` binary). Use UTC DateTime objects for rollover assertions so they
  are timezone-independent and pass with the fallback `timegm` (day/hour rollover is
  linear there; the pre-fold covers month).
- no e2e layer applies.

## Progress Tracking

- mark completed items with `[x]` immediately when done
- add newly discovered tasks with ➕ prefix
- document issues/blockers with ⚠️ prefix

## Solution Overview

Add `DateTime::normalize()` next to `updateFromTimestamp()`:

```cpp
void normalize()
{
   if (valid)
      return;
   // fold out-of-range month into year: the fallback timegm used where
   // the platform lacks a native one rejects tm_mon outside [0..11]
   if ((data.tm_mon < 0) || (data.tm_mon > 11))
   {
      data.tm_year += data.tm_mon / 12;
      data.tm_mon %= 12;
      if (data.tm_mon < 0)
      {
         data.tm_mon += 12;
         data.tm_year--;
      }
   }
   timestamp = utc ? timegm(&data) : mktime(&data);
   valid = true;
   updateFromTimestamp();
}
```

Call sites:

- `getAttr()`: call `normalize()` where `st` is computed, whenever `st != nullptr`
  (real attribute read, not a `?`-existence query). The `timestamp` branch collapses
  to returning `dt->timestamp`. Normalizing on *any* real read (including `isUTC`
  and unknown attribute names) is intended — slight wasted work, behaviorally
  harmless. Side benefit: the `?`-existence path stays un-normalized, which removes
  the latent `mktime(nullptr)` deref currently reachable via `?timestamp` on an
  invalid object.
- `format` method: `dt->normalize()` before `_tcsftime()`.
- `toString()`: same.
- `isUTC` setter: replace the inline compute-if-invalid block with `normalize()`,
  then flip `utc` and `updateFromTimestamp()`. Note: on a flip while `!valid`,
  `updateFromTimestamp()` runs twice (once inside `normalize()` with the old `utc`,
  once after the flip) — redundant but correct; accepted for simplicity.

`setAttr()` field branches stay untouched (raw write + `valid = false`).

## Technical Details

- Normalization respects the `utc` flag: `timegm()` for UTC objects, `mktime()` for
  local — same as the existing `timestamp` getter.
- `updateFromTimestamp()` regenerates the whole `struct tm` from the timestamp via
  `gmtime_r`/`localtime_r`, so dependent fields (`tm_wday`, `tm_yday`, `tm_isdst`)
  are always consistent after normalization; in-place normalization behavior of
  `mktime`/`timegm` is not relied upon.
- `month` is 0-based in NXSL (`t.month == 6` is July), so `month = 20` normalizes to
  September of the following year (20 = 12 + 8).
- **Attention in format assertions**: `strftime` `%m` is 1-based while NXSL `.month`
  is 0-based. The object where `.month == 8` formats as `%m == "09"`. Pin exact
  expected strings in every `format`-based test.

## Implementation Steps

### Task 1: Add normalize() and wire up all read paths

**Files:**
- Modify: `src/libnxsl/time.cpp`

- [ ] add `normalize()` method to `DateTime` struct (month/year pre-fold, timestamp
      computation via `timegm`/`mktime` per `utc`, `valid = true`,
      `updateFromTimestamp()`)
- [ ] `getAttr()`: call `normalize()` when `st != nullptr`; simplify `timestamp`
      branch to return `dt->timestamp`
- [ ] `format` method: call `dt->normalize()` before `_tcsftime()`
- [ ] `toString()`: call `dt->normalize()` before `_tcsftime()`
- [ ] `isUTC` setter in `setAttr()`: replace inline compute-if-invalid block with
      `normalize()`
- [ ] build libnxsl and test binary; run existing tests — `time.nxsl` must still pass
      (test gate for this task is the existing suite; new tests land in Task 2)

### Task 2: Extend time.nxsl with normalization tests

**Files:**
- Modify: `tests/test-libnxsl/time.nxsl`

- [ ] day overflow (UTC): from a known timestamp set `day = 50`, assert rolled
      `month`/`day`, exact `timestamp`, and pinned `format('%Y-%m-%d')` string
      (remember: `%m` is 1-based, `.month` is 0-based)
- [ ] month overflow (UTC): from 2023-07-21 UTC set `month = 20`, assert
      `year == 2024`, `month == 8`, and pinned `format('%Y-%m-%d')` starting
      `2024-09` plus non-empty `string(t)` (regression for the UB path)
- [ ] backward rollover (UTC): `day = 0` → last day of previous month; `month = -1`
      → December of previous year
- [ ] dependent-field refresh: after a field write, assert `dayOfWeek`/`dayOfYear`
      match the normalized date when read directly (not via `timestamp`)
- [ ] batch-set semantics still hold: existing `isDST = -1; month = 0;` block
      unchanged and passing
- [ ] extreme value: `year = 0xfffffff` (as integer) → `string(t)` is non-empty,
      no crash/garbage (validated only on native-`timegm` platforms; the fallback
      `timegm` has a pre-existing weakness with huge years — skip/soften this case
      in the optional Windows verification)
- [ ] run: build and execute `test-libnxsl` with the absolute script directory path
      (`./test-libnxsl /Users/alk/Development/netxms/netxms-master/tests/test-libnxsl`)
      — all scripts pass

### Task 3: Verify acceptance criteria

- [ ] all four reproduction cases from issue #3388 produce normalized, well-formed
      output (verify with `nxscript` or the test suite)
- [ ] out-of-range month never reaches `_tcsftime` un-normalized on any output path
      (getAttr / format / toString reviewed)
- [ ] full test suite for libnxsl passes; run broader `tests/suite` if built
- [ ] cross-check completed changes against this plan

### Task 4: [Final] Update documentation

- [ ] comment on issue #3388: fix summary + out-of-scope observations
      (`F_mktime` ignoring `utc`, fallback `timegm` divergence)
- [ ] move this plan to `docs/plans/completed/`

## Post-Completion

**Manual verification** (if applicable):
- optional: run reproduction script on a Windows build to confirm fallback-`timegm`
  path (month pre-fold) behaves the same as Linux

**External system updates** (if applicable):
- NXSL documentation repo: DateTime class docs could mention mktime-style
  normalization semantics (separate repo, not part of this change)
