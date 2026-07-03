# HA lease algorithm test harness

Validation tooling for the netxmsd high-availability design
([doc/HA_Design.md](../../doc/HA_Design.md), issue
[#3364](https://github.com/netxms/netxms/issues/3364)), phase 0.

## ha-node-sim

A fake netxmsd that runs the real `HALeaseManager` (compiled directly from
`src/server/core/halease.cpp`) against a shared database. While it holds the
lease it continuously inserts role-stamped rows into the `ha_sim_writes`
scratch table — the ground truth the harness asserts safety invariants
against. A dumb TCP heartbeat between two sims proves that the promotion rule
ignores channel state. Fault injection commands are accepted on stdin
(`release`, `alloc <n>`, `longtxn <ms>`, `quit`); machine-parseable state
records are emitted on stdout (`STATUS`, `EVENT promoted/fenced/released`,
`ALLOC`, `ALLOC-COLLISION`, `LONGTXN`).

Exit codes: 0 = normal exit, 42 = fenced, 1 = startup error.

The sim only needs the `ha_lease` table (created by the regular NetXMS schema
at version 70.1 or later); it creates its own scratch tables on startup.

Example (two nodes against one PostgreSQL database):

```
ha-node-sim -N node1 -s dbhost -D netxms -U netxms -P secret -l 9101 -p 127.0.0.1:9102
ha-node-sim -N node2 -s dbhost -D netxms -U netxms -P secret -l 9102 -p 127.0.0.1:9101
```

Built only when tests are enabled (`--with-tests`).

## harness.py

Adversarial scenario runner (phase 0 step 0.4): runs the ten scenarios from
doc/HA_Design.md section 6 against two sims and one PostgreSQL database, with
each sim's database and heartbeat traffic routed through harness-controlled
TCP proxies (CUT = partition, STALL = hung connection). Asserts the safety
invariants on `ha_sim_writes`: exactly one writer per term, strictly
increasing terms, no old-term write after a newer term's first write.

```
PGPASSWORD=... python3 harness.py --db netxms_hatest [--scenario N]
```

The target database needs the `ha_lease` table with its seed row and a
`metadata` table containing `Syntax` (`DBGetSyntax` reads it) — any regular
NetXMS database at schema 70.1+ qualifies, or create the two tables manually
in a scratch database. The harness drops and recreates the `ha_sim_*` scratch
tables between scenarios.

On AddressSanitizer builds the harness passes `LSAN_OPTIONS` pointing at
`lsan-suppressions.txt` (libpq leaks one OpenSSL BIO method registration per
process by design); use the same option when running ha-node-sim manually.
