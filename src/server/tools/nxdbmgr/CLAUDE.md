# nxdbmgr — NetXMS database manager

Command line tool that initializes, checks, upgrades, migrates, exports and imports the NetXMS server database.

> **STOP — read this before editing any `upgrade_v*.cpp`.**
>
> A database schema change is almost never a single-branch edit. NetXMS maintains several release branches at once, each with its own schema major version, and a change committed to one branch normally has to be replicated into every newer branch with a version guard — otherwise upgrades from the older branch either fail outright ("Unable to find upgrade procedure for version X.Y") or apply the same change twice.
>
> If the `netxms-db-backport` skill is available, use it — it is the authoritative procedure. **This file is a condensed fallback for when that skill is not installed.**
>
> Before writing an upgrade procedure, confirm with the user which branches the change must land on (master only? a stable branch and everything newer?). Do not guess. If the user asks for "just master", say explicitly that stable branches will not get the fix and let them confirm that is intended.

## Layout

| File | Purpose |
|---|---|
| `nxdbmgr.cpp` | Command line parsing, DB connect, command dispatch |
| `upgrade.cpp` | Upgrade driver; version checks; `SetMajorSchemaVersion` / `SetMinorSchemaVersion` / `Get`/`SetSchemaLevelForMajorVersion` |
| `upgrade_v<major>.cpp` | One file per schema major version — the upgrade procedures for that chain |
| `upgrade_online.cpp` | Background ("online") upgrades that run in the server after schema upgrade |
| `check.cpp` | `nxdbmgr check` — consistency checks and repairs |
| `init.cpp` | `nxdbmgr init` — fresh install from `sql/*.in` baselines |
| `export.cpp`, `migrate.cpp`, `convert.cpp`, `tdata_convert.cpp` | Export/import, cross-DB migration, TimescaleDB conversion |
| `../libnxdbmgr/` | Shared helpers (`CreateTable`, `CreateConfigParam`, `SQLQuery`, …), exported to nxdbmgr and the server |
| `../../include/nxdbmgr_tools.h` | Declarations of those helpers and the `CHK_EXEC` macros |
| `include/netxmsdb.h` (repo root) | Compiled-in target schema version |
| `sql/*.in` (repo root) | Fresh-install schema baseline |

Commands: `init`, `upgrade`, `check`, `check-data-tables`, `export`, `import`, `migrate`, `convert`, `background-upgrade`, `background-convert`, `batch`, `get`, `set`, `set-user-password`, `unlock`, `unlock-user`, `reset-monitoring`, `reset-system-account`. Run `nxdbmgr -h` for the full option list.

## Schema versioning model

Schema version is `major.minor`, stored in the `metadata` table (`SchemaVersionMajor`, `SchemaVersionMinor`). The version the code expects is compiled in from `include/netxmsdb.h`:

```cpp
#define DB_LEGACY_SCHEMA_VERSION       700     // frozen pre-2.1 single-number version; never touch
#define DB_SCHEMA_VERSION_MAJOR        70      // this branch's current major
#define DB_SCHEMA_VERSION_MINOR        9       // this branch's chain tip
#define DB_SCHEMA_VERSION_V70_MINOR    DB_SCHEMA_VERSION_MINOR   // alias, CURRENT major only
```

Each major version has a chain file `upgrade_v<major>.cpp` holding `H_UpgradeFromV<n>` procedures (each moves `<major>.<n>` to the next version) and a descending `s_dbUpgradeMap[]` of `{ minor, nextMajor, nextMinor, proc }`.

For the branch's **own** major, the driver loop is bounded — `while ((major == 70) && (minor < DB_SCHEMA_VERSION_V70_MINOR))`. For every **older** major, the same file on this branch has an unbounded loop `while (major == 62)` plus a jump procedure at the top of the map whose entire body is `CHK_EXEC(SetMajorSchemaVersion(70, 0));`.

`SetMajorSchemaVersion(next, 0)` records where the old chain ended as `metadata` entry `SchemaVersionLevel.<oldMajor>`. That marker is how a database remembers which backported fixes it already received before it jumped to a newer major. `GetSchemaLevelForMajorVersion(M)` reads it; if it is absent but a marker for a *newer* major exists, it returns `INT_MAX` (database was initialized fresh on a newer version, so the change is already in the baseline schema), otherwise `-1`.

**Core invariant:** a change is keyed by `(originMajor, originLevel)` — the `major.minor` it got on the **oldest** branch that received it. Newer branches apply it under a guard on that key and set the key when done.

## Adding an upgrade procedure

### On the origin (oldest) branch — plain procedure

At the top of `upgrade_v<M>.cpp`, with `L = current tip + 1`:

```cpp
/**
 * Upgrade from 62.33 to 62.34
 */
static bool H_UpgradeFromV33()
{
   CHK_EXEC(SQLQuery(L"ALTER TABLE package_deployment_jobs ADD retry_count integer"));
   CHK_EXEC(SetMinorSchemaVersion(34));
   return true;
}
```

Add the map entry at the top (`{ 33, 62, 34, H_UpgradeFromV33 },`) and bump `DB_SCHEMA_VERSION_MINOR` in `include/netxmsdb.h`.

### On every newer branch — two edits

**(a) Replicate the origin chain extension.** In this branch's copy of `upgrade_v<originMajor>.cpp`: rename the jump procedure from `H_UpgradeFromV<L-1>` to `H_UpgradeFromV<L>`, insert the new step procedure with a body **identical** to the origin branch's, and make the top of the map read:

```cpp
   { 34, 70, 0,  H_UpgradeFromV34 },   // jump to next major, renamed
   { 33, 62, 34, H_UpgradeFromV33 },   // new step, body copied verbatim
```

No `netxmsdb.h` change for this — that file only ever describes the branch's own current major. Never add `V*_MINOR` constants for other majors.

**(b) Add a guarded procedure to this branch's own chain.** In `upgrade_v<ownMajor>.cpp`, with `K = own tip + 1`:

```cpp
/**
 * Upgrade from 70.8 to 70.9
 */
static bool H_UpgradeFromV8()
{
   if (GetSchemaLevelForMajorVersion(62) < 34)
   {
      CHK_EXEC(SQLQuery(L"ALTER TABLE package_deployment_jobs ADD retry_count integer"));
      CHK_EXEC(SetSchemaLevelForMajorVersion(62, 34));
   }
   CHK_EXEC(SetMinorSchemaVersion(9));
   return true;
}
```

`SetSchemaLevelForMajorVersion` goes **inside** the guard, `SetMinorSchemaVersion` **outside** (it must always run). Bump `DB_SCHEMA_VERSION_MINOR` on this branch.

This covers every upgrade path: a database below `62.34` gets the change while walking the old chain (the guard in (b) then sees the level and skips); a database already past the jump has no/low `SchemaVersionLevel.62` and gets it from the guarded procedure; a fresh install on a newer version resolves to `INT_MAX` and skips.

### Fresh-install baseline — on every branch that gets the change

Upgrade procedures only serve existing databases. Mirror the change into the matching `sql/` file:

| Change | File |
|---|---|
| Table / index / column | `sql/schema.in` |
| Config parameter | `sql/setup.in` (keep alphabetical order) |
| Event template | `sql/events.in` |
| Metadata entry | `sql/metadata.in` |
| Library script | `sql/scripts.in` |
| SNMP trap mapping | `sql/traps.in` |

### Retrofit — change already shipped unguarded on a newer branch

Common when a fix lands on master first and the backport decision comes later. Wrap the already-shipped procedure's body in the `GetSchemaLevelForMajorVersion` guard and add `SetSchemaLevelForMajorVersion` — editing in place is safe, since databases past that minor never re-run it. Then do step (a). Do **not** bump the minor: it was consumed when the procedure first shipped.

## Branch discovery

Never assume the branch layout or the version numbers. Stable branches are checked out as sibling worktrees:

```bash
git worktree list
for d in ../netxms-*; do printf "%-16s " $d; grep -h "define DB_SCHEMA_VERSION_MAJOR\|define DB_SCHEMA_VERSION_MINOR" $d/include/netxmsdb.h | tr -s ' ' | cut -d' ' -f3 | paste -sd. -; done
```

A newer branch may already have reserved the next minor of an older chain (master's `upgrade_v62.cpp` can contain a 62.34 step before stable-6.2 has committed it). Before allocating a minor, check the top of that major's chain file on **every** newer branch and reuse the reserved number if it is the same change, or take the next free number above all copies.

One commit per branch. Branches are independent — never merge stable branches into each other or into master.

## Writing procedure bodies

- Wrap every statement in `CHK_EXEC(...)` — it creates a savepoint and rolls back to it on failure. The whole procedure runs inside a transaction managed by the driver.
- Use an existing helper rather than raw SQL where one exists. From `nxdbmgr_tools.h`: `CreateTable`, `CreateConfigParam`, `CreateEventTemplate`, `CreateLibraryScript`, `GenerateGUID`, `IsIndexExists`, `SQLQuery`, `SQLQueryFormatted`, `SQLSelect`. From libnxdb (`include/nxdbapi.h`), for portable column changes: `DBSetNotNullConstraint`, `DBRenameColumn`, `DBResizeColumn`, `DBDropColumn` — all take `g_dbHandle` as first argument. `CreateTable` understands the `$SQL:TEXT` / `$SQL:INT64` type placeholders that keep DDL portable across the supported databases.
- Bodies must be byte-identical across branches (except the trailing `SetMinorSchemaVersion` and the guard wrapper). Copy verbatim; do not "improve" one copy.
- Be defensive against pre-existing state — e.g. check `IsIndexExists` before `CREATE INDEX`. On PostgreSQL a failed statement poisons the entire upgrade transaction.
- A helper needed by procedures in more than one chain file goes into `../libnxdbmgr/upgrade.cpp` with `LIBNXDBMGR_EXPORTABLE` and a declaration in `src/server/include/nxdbmgr_tools.h`.
- Long data conversions that should not block the upgrade go through `RegisterOnlineUpgrade()` in `upgrade_online.cpp`; check `IsOnlineUpgradePending()` if a new step depends on one.

## Verification checklist

1. Every `SetMinorSchemaVersion(n)` argument matches its own procedure's target minor **in that file**. Copying a procedure from another branch and keeping the source branch's minor is the classic backport bug. Eyeball that each `Upgrade from X.Y to X.Z` comment matches `SetMinorSchemaVersion(Z)`.
2. The guard key is the **origin** branch's `(major, level)` everywhere, and identical on all branches.
3. On each newer branch, the jump procedure is renamed and the map's top two entries are correct.
4. `DB_SCHEMA_VERSION_MINOR` bumped by exactly one on every edited branch, and only for that branch's own major. A missed bump means the new procedure compiles but **silently never runs** (it is outside the loop bound), and `nxdbmgr upgrade` still reports success — while netxmsd, which requires the DB version to *exactly equal* the compiled-in version, refuses to start. Cross-check: `DB_SCHEMA_VERSION_MINOR` equals the highest `SetMinorSchemaVersion()` argument in the branch's own chain file.
5. `sql/*.in` baseline updated on every edited branch.
6. Build in each worktree: `make -C src/server/tools/nxdbmgr`.
7. If practical, test both paths against a scratch database: upgrade from a dump below the origin level (change applied once, via the plain procedure), and from one past the major jump (applied once, via the guard).
