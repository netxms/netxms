# Seed Stock Image Library Images from C++ Code

## Overview

Database initialization currently seeds the 9 stock image-library images by executing
`INSERT INTO images ...` statements with the base64 PNG payload embedded as a SQL string
literal (`sql/images.in`, included into `dbinit_<db>.sql`). This breaks on backends with
string-literal size limits:

- **Oracle**: literals are capped at 4000 bytes → `ORA-01704: string literal too long`
  on the Node image (8324-byte payload). Database init fails.
- **MSSQL**: non-Unicode literals are typed `varchar(8000)` → the Node image payload is
  silently truncated to 8000 bytes, storing a corrupted PNG.

Fix: remove image seeding from the SQL batch entirely. Move the stock image data
(guid, name, category, mimetype, base64 payload) into a dedicated C++ file in nxdbmgr.
`InitDatabase()` iterates the table and inserts each image via a bound prepared
statement — parameter binding bypasses literal limits on every backend (the Oracle
driver binds `DB_SQLTYPE_TEXT` through a temporary CLOB, `oracle.cpp:504-530`).

Bonus deduplication: `upgrade_v62.cpp` already embeds a copy of the same payloads
(`s_stockImages[]`, line 733) for the 62.13 backfill. That copy is removed and the
upgrade procedure switches to a lookup-by-guid function exposed by the new shared file.

## Context (from discovery)

- `sql/images.in` — generated file with the 9 INSERT statements; included via
  `sql/dbinit.in:36` (`#include "images.in"`), listed in `SOURCE` of `sql/Makefile.am`,
  `sql/Makefile.w32`, `sql/Makefile.msvc.w32`.
- `src/server/tools/nxdbmgr/init.cpp` — `InitDatabase()` runs the batch via
  `ExecSQLBatch()`; failures land on `init_failed`.
- `src/server/tools/nxdbmgr/upgrade_v62.cpp:725-787` — duplicate `s_stockImages[]`
  (guid + base64 only) and `FindStockImage()` used by `H_UpgradeFromV12()` (62.13
  backfill for issue #3359); also file-local `s_missingImagePlaceholder` (stays there,
  upgrade-only concern).
- `tools/regen_images_in.py` — regenerates `sql/images.in` from `images/<guid>` PNG
  files; holds the metadata table (guid, name, category, mimetype).
- `images/Makefile.am` — `EXTRA_DIST` of the 9 PNG source files; comment references
  `sql/images.in` and the regen script.
- Insert pattern to follow: `upgrade_v62.cpp:617` / `webapi/images.cpp:280` — prepared
  `INSERT INTO images (guid,name,category,mimetype,protected,image_data) VALUES (...)`
  with binary payload bound as `DB_SQLTYPE_TEXT` (libnxdb base64-encodes transparently).
- `src/server/tools/nxdbmgr/Makefile.am` — explicit `nxdbmgr_SOURCES` list (must add new
  file); `Makefile.w32` uses `$(wildcard *.cpp)` (picks it up automatically).

## Development Approach

- **Testing approach**: regular (code first, then verification). nxdbmgr has no unit-test
  harness; each task ends with a build + behavioral verification step instead of unit
  tests, and the full init/upgrade smoke check happens in the verification task.
- Complete each task fully before moving to the next.
- Update this plan file when scope changes during implementation.

## Solution Overview

New file `src/server/tools/nxdbmgr/stock_images.cpp`:

```cpp
// Between generator markers (see Task 4):
static const struct
{
   const wchar_t *guid;
   const wchar_t *name;
   const wchar_t *category;    // always L"Network Objects" today, kept per-image
   const char *mimetype;       // always "image/png" today, kept per-image
   const char *data;           // base64-encoded PNG
} s_stockImages[] = { ... 9 entries ... };

// Lookup by GUID for upgrade procedures; returns nullptr if not a stock image
const char *FindStockImageData(const wchar_t *guid);

// Seed all stock images at database initialization; returns false on first failure
bool SeedStockImages();
```

`SeedStockImages()` prepares
`INSERT INTO images (guid,name,category,mimetype,protected,image_data) VALUES (?,?,?,?,1,?)`
once (`protected` hardcoded to 1 — all stock images are protected), then for each entry
decodes the base64 payload with `base64_decode_alloc()` and binds:

- 1: `DB_SQLTYPE_VARCHAR` guid
- 2: `DB_SQLTYPE_VARCHAR` name
- 3: `DB_SQLTYPE_VARCHAR` category
- 4: `DB_SQLTYPE_VARCHAR` mimetype (`DB_CTYPE_UTF8_STRING`)
- 5: binary `DBBind(..., DB_SQLTYPE_TEXT, data, size, DB_BIND_DYNAMIC)` — same pattern
  as `upgrade_v62.cpp:845` and `webapi/images.cpp`; libnxdb re-encodes to base64 for the
  text column, so the stored value is identical to what `images.in` produced.

`InitDatabase()` calls `SeedStockImages()` right after `ExecSQLBatch()` succeeds
(before the user-GUID updates); failure goes to `init_failed`. The generic
`nxdbmgr batch` command path is not touched.

Key decisions:

- **Data lives in nxdbmgr, not a SQL file** — images become unavailable to anyone
  hand-applying `dbinit_<db>.sql` without nxdbmgr; acceptable because init is documented
  to run through `nxdbmgr init`, and the GUID/password updates in `InitDatabase()`
  already make raw-SQL-only init incomplete.
- **Upgrade dedup**: `H_UpgradeFromV12()` drops its local table and calls
  `FindStockImageData()`. Note this softens the "freeze helpers used by historical
  upgrades" convention: if a stock PNG is ever changed and regenerated, the 62.13
  backfill will restore the *current* artwork instead of the 62.13-era artwork. For a
  backfill whose purpose is "restore missing stock icons" this is acceptable (arguably
  preferable); flagged here deliberately.
- **`s_missingImagePlaceholder` stays in `upgrade_v62.cpp`** — upgrade-only concern,
  not stock image data.
- **Regen tooling retargeted** (user decision): `tools/regen_images_in.py` becomes
  `tools/regen_stock_images.py` and rewrites only the marker-delimited data block in
  `stock_images.cpp`, keeping hand-written code out of the generator.

## Technical Details

- Processing flow at init: `ExecSQLBatch(dbinit_<db>.sql)` (schema + non-image seed data)
  → `SeedStockImages()` → user GUID/password updates.
- Transactions: on PostgreSQL/SQLite/TSDB the batch's own `COMMIT` has already run when
  seeding starts, so image inserts auto-commit individually — same behavior as the
  existing post-batch GUID updates. No change for Oracle/MSSQL (per-statement commit).
- Wide (`wchar_t`) fields bind directly with `DB_BIND_STATIC`; server-side tools are
  Unicode builds, plain `L"..."` literals per server code conventions.
- Generator markers in `stock_images.cpp`:
  `// BEGIN GENERATED STOCK IMAGE DATA (tools/regen_stock_images.py)` /
  `// END GENERATED STOCK IMAGE DATA`.

## Progress Tracking

- mark completed items with `[x]` immediately when done
- add newly discovered tasks with ➕ prefix
- document issues/blockers with ⚠️ prefix

## Implementation Steps

### Task 1: Create stock_images.cpp with data table, lookup, and seeding

**Files:**
- Create: `src/server/tools/nxdbmgr/stock_images.cpp`
- Modify: `src/server/tools/nxdbmgr/nxdbmgr.h`
- Modify: `src/server/tools/nxdbmgr/Makefile.am`

- [x] create `stock_images.cpp` with `s_stockImages[]` (9 entries: guid, name, category,
      mimetype, base64 data — metadata from `tools/regen_images_in.py` `STOCK_IMAGES`,
      payloads identical to current `sql/images.in`), wrapped in generator markers
      ➕ data block populated by running the retargeted regen script (Task 4 pulled
      forward) instead of hand-copying payloads
- [x] implement `FindStockImageData(const wchar_t *guid)` (case-insensitive `wcsicmp`
      match, returns `nullptr` when not found — same contract as the current
      `FindStockImage` in upgrade_v62.cpp)
- [x] implement `SeedStockImages()`: prepared insert, per-image `base64_decode_alloc` +
      binary `DB_SQLTYPE_TEXT` bind, terminal error message via `WriteToTerminalEx` on
      decode or SQL failure, `false` on first failure
- [x] declare both functions in `nxdbmgr.h`
- [x] add `stock_images.cpp` to `nxdbmgr_SOURCES` in `Makefile.am`
- [x] verify: `make -C src/server/tools/nxdbmgr` compiles clean (both symbols present
      in nxdbmgr-stock_images.o)

### Task 2: Call seeding from InitDatabase and switch upgrade to shared lookup

**Files:**
- Modify: `src/server/tools/nxdbmgr/init.cpp`
- Modify: `src/server/tools/nxdbmgr/upgrade_v62.cpp`

- [x] in `InitDatabase()` call `SeedStockImages()` after the `ExecSQLBatch` success
      check; on failure `goto init_failed`
- [x] in `upgrade_v62.cpp` delete the local `s_stockImages[]` and `FindStockImage()`;
      point `H_UpgradeFromV12()` at `FindStockImageData()`; keep
      `s_missingImagePlaceholder` and the rest of the backfill logic unchanged
- [x] update the comment in `H_UpgradeFromV12()`/data-table area so it no longer claims
      the payloads are file-local ("frozen as of 62.13" text moves/adjusts to reference
      `stock_images.cpp`)
- [x] verify: nxdbmgr builds; `grep -rn "FindStockImage\b" src/` shows no stale callers

### Task 3: Remove images.in from the SQL batch and build system

**Files:**
- Delete: `sql/images.in`
- Modify: `sql/dbinit.in`
- Modify: `sql/Makefile.am`
- Modify: `sql/Makefile.w32`
- Modify: `sql/Makefile.msvc.w32`
- Modify: `images/Makefile.am`

- [x] remove `#include "images.in"` from `sql/dbinit.in`
- [x] remove `images.in` from the `SOURCE` lists in all three sql makefiles
- [x] delete `sql/images.in`
- [x] update the comment in `images/Makefile.am` to reference `stock_images.cpp` and
      `tools/regen_stock_images.py` instead of `sql/images.in` / `regen_images_in.py`
- [x] verify: `grep -rn "images.in" --exclude-dir=.claude .` returns no live references;
      regenerated `sql/dbinit_sqlite.sql` contains no `INSERT INTO images` (all 7
      dbinit variants rebuilt; Oracle and MSSQL scripts contain zero image INSERTs)

### Task 4: Retarget the regeneration script

**Files:**
- Create: `tools/regen_stock_images.py` (renamed from `tools/regen_images_in.py`)
- Delete: `tools/regen_images_in.py`

- [x] rewrite the script to regenerate only the marker-delimited `s_stockImages[]`
      block in `src/server/tools/nxdbmgr/stock_images.cpp` from `images/<guid>` files,
      preserving the metadata table (guid, name, category, mimetype) in the script
- [x] `git mv tools/regen_images_in.py tools/regen_stock_images.py` (then rewrite) so
      history is preserved
- [x] verify: running `python3 tools/regen_stock_images.py` on the freshly written
      `stock_images.cpp` produces zero diff (identical md5 before/after)

### Task 5: Verify acceptance criteria

- [x] full build passes (sql: all 7 dbinit + 7 dbschema variants; nxdbmgr binary)
- [x] `nxdbmgr init` against SQLite succeeds; `SELECT count(*) FROM images` returns 9
      and `image_data` of guid `904e7291-ee3f-41b7-8132-2bd29288ecc8` matches the
      base64 of `images/904e7291-ee3f-41b7-8132-2bd29288ecc8` (the 8324-char payload,
      byte-identical via cmp)
- [x] `nxdbmgr init` output shows no images-related errors; init-twice still reports
      "already initialized"
- [x] cross-check this plan's file list against the diff — no file silently skipped

### Task 6: Update documentation

- [x] check `doc/` and component CLAUDE.md files for references to `images.in` or the
      regen script; update if any (none found)
- [x] move this plan to `docs/plans/completed/`

## Post-Completion

**Manual verification:**
- Run `nxdbmgr init` against a real **Oracle** instance — the original ORA-01704
  scenario — and confirm all 9 images seed and render in the management console.
- Run `nxdbmgr init` against **MSSQL** and confirm the Node image is not truncated
  (fetch via WebAPI `/images/904e7291-...` and validate the PNG decodes).
- Upgrade path spot-check: on a pre-62.13 database with a missing stock image file,
  run `nxdbmgr upgrade` and confirm the 62.13 backfill still restores stock images
  (now via `FindStockImageData`).

**External systems:**
- Packaging: `sql_DATA` installed files are unchanged in name (`dbinit_*.sql` still
  built), but any downstream tooling that parsed `sql/images.in` directly must switch
  to `stock_images.cpp` / the regen script.
