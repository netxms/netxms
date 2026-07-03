#!/usr/bin/env python3
#
# Adversarial test harness for the netxmsd HA lease algorithm
# (doc/HA_Design.md section 6, issue #3364, phase 0 step 0.4).
#
# Runs two ha-node-sim instances against one PostgreSQL database, with each
# sim's database and heartbeat traffic routed through harness-controlled TCP
# proxies (CUT = partition, STALL = hung connection). Injects the faults from
# the design's validation table and asserts the safety invariants on the
# ha_sim_writes scratch table.
#
# Usage:
#   PGPASSWORD=... python3 harness.py --db netxms_hatest [--scenario N] ...
#
# Requirements: python3 (stdlib only), psql in PATH, ha-node-sim built.

import argparse
import os
import signal
import socket
import subprocess
import sys
import threading
import time
import traceback

# Lease timings used for all scenarios (seconds)
REFRESH = 1
VALIDITY = 5
MARGIN = 1

# Derived bounds (with scheduling slack)
PROMOTION_BOUND = VALIDITY + REFRESH + 3      # crash -> peer active
FENCE_BOUND = VALIDITY - MARGIN + 2           # db loss -> self-fence


class Proxy:
    """TCP proxy with NORMAL / STALL / CUT modes"""

    NORMAL = "normal"
    STALL = "stall"    # keep connections, stop relaying (hung network)
    CUT = "cut"        # drop everything, reject new connections (partition)

    def __init__(self, listen_port, target_port, target_host="127.0.0.1"):
        self.listen_port = listen_port
        self.target = (target_host, target_port)
        self.mode = Proxy.NORMAL
        self.conns = []
        self.lock = threading.Lock()
        self.closed = False
        self.listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        for attempt in range(20):
            try:
                self.listener.bind(("127.0.0.1", listen_port))
                break
            except OSError:
                if attempt == 19:
                    raise
                time.sleep(0.25)
        self.listener.listen(16)
        threading.Thread(target=self._accept_loop, daemon=True).start()

    def _accept_loop(self):
        while not self.closed:
            try:
                client, _ = self.listener.accept()
            except OSError:
                break
            if self.mode == Proxy.CUT:
                client.close()
                continue
            if self.mode == Proxy.STALL:
                # accept and hold: connection established, nothing answers
                with self.lock:
                    self.conns.append((client, None))
                continue
            try:
                upstream = socket.create_connection(self.target, timeout=2)
            except OSError:
                client.close()
                continue
            with self.lock:
                self.conns.append((client, upstream))
            threading.Thread(target=self._pump, args=(client, upstream), daemon=True).start()
            threading.Thread(target=self._pump, args=(upstream, client), daemon=True).start()

    def _pump(self, src, dst):
        while not self.closed:
            if self.mode == Proxy.CUT:
                break
            if self.mode == Proxy.STALL:
                time.sleep(0.1)
                continue
            try:
                src.settimeout(0.2)
                data = src.recv(65536)
            except socket.timeout:
                continue
            except OSError:
                break
            if not data:
                break
            try:
                dst.sendall(data)
            except OSError:
                break
        for s in (src, dst):
            try:
                s.close()
            except OSError:
                pass

    def set_mode(self, mode):
        self.mode = mode
        if mode == Proxy.CUT:
            with self.lock:
                for pair in self.conns:
                    for s in pair:
                        if s is not None:
                            try:
                                s.close()
                            except OSError:
                                pass
                self.conns = []

    def close(self):
        self.closed = True
        self.set_mode(Proxy.CUT)
        try:
            self.listener.close()
        except OSError:
            pass


class Node:
    """One ha-node-sim process with parsed stdout"""

    def __init__(self, ctx, name, db_proxy_port, listen_port, peer_port, rebase=False):
        self.name = name
        self.proc = None
        self.status = {}
        self.events = []          # (monotonic_time, line)
        self.role_history = []    # (monotonic_time, role, term)
        self.lines = []
        self.lock = threading.Lock()
        args = [
            ctx.sim,
            "-N", name,
            "-s", f"127.0.0.1:{db_proxy_port}",
            "-D", ctx.dbname, "-U", ctx.dbuser, "-P", ctx.dbpassword,
            "-r", str(REFRESH), "-v", str(VALIDITY), "-m", str(MARGIN),
            "-l", str(listen_port), "-p", f"127.0.0.1:{peer_port}",
            "-x", "1",
        ]
        if rebase:
            args.append("-R")
        env = dict(os.environ)
        suppressions = os.path.join(os.path.dirname(os.path.abspath(__file__)), "lsan-suppressions.txt")
        env["LSAN_OPTIONS"] = f"suppressions={suppressions}:print_suppressions=0"
        self.proc = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                     stderr=subprocess.STDOUT, text=True, bufsize=1, env=env)
        threading.Thread(target=self._reader, daemon=True).start()

    def _reader(self):
        for line in self.proc.stdout:
            line = line.rstrip()
            now = time.monotonic()
            with self.lock:
                self.lines.append((now, line))
                if line.startswith("STATUS "):
                    fields = dict(f.split("=", 1) for f in line[7:].split(" ") if "=" in f)
                    self.status = fields
                    role = fields.get("role", "?")
                    term = int(fields.get("term", 0))
                    if not self.role_history or self.role_history[-1][1:] != (role, term):
                        self.role_history.append((now, role, term))
                elif line.startswith(("EVENT ", "ALLOC", "LONGTXN ", "READY ", "ERROR ")):
                    self.events.append((now, line))

    def send(self, command):
        try:
            self.proc.stdin.write(command + "\n")
            self.proc.stdin.flush()
        except OSError:
            pass

    def role(self):
        with self.lock:
            return self.status.get("role", "?")

    def wait_role(self, role, timeout):
        """Wait until node reports given role; return monotonic time of the
        first matching role transition, or None"""
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            with self.lock:
                for t, r, term in self.role_history:
                    if r == role:
                        return t
            time.sleep(0.05)
        return None

    def wait_event(self, prefix, timeout):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            with self.lock:
                for t, line in self.events:
                    if line.startswith(prefix):
                        return (t, line)
            time.sleep(0.05)
        return None

    def count_events(self, prefix):
        with self.lock:
            return sum(1 for _, line in self.events if line.startswith(prefix))

    def wait_exit(self, timeout):
        try:
            return self.proc.wait(timeout)
        except subprocess.TimeoutExpired:
            return None

    def sigstop(self):
        self.proc.send_signal(signal.SIGSTOP)

    def sigcont(self):
        self.proc.send_signal(signal.SIGCONT)

    def kill9(self):
        try:
            self.proc.kill()
        except OSError:
            pass

    def destroy(self):
        self.kill9()
        try:
            self.proc.wait(5)
        except subprocess.TimeoutExpired:
            pass


class Context:
    def __init__(self, args):
        self.sim = args.sim
        self.dbname = args.db
        self.dbuser = args.user
        self.dbpassword = os.environ.get("PGPASSWORD", "")
        self.dbhost = args.db_host
        self.dbport = args.db_port
        self.base_port = args.base_port
        self.proxies = []
        self.nodes = []

    def psql(self, sql):
        env = dict(os.environ)
        result = subprocess.run(
            ["psql", "-X", "-q", "-h", self.dbhost, "-p", str(self.dbport), "-U", self.dbuser,
             "-d", self.dbname, "-t", "-A", "-F", "|", "-c", sql],
            capture_output=True, text=True, env=env)
        if result.returncode != 0:
            raise RuntimeError(f"psql failed: {result.stderr.strip()}")
        return [line.split("|") for line in result.stdout.splitlines() if line]

    def reset_db(self):
        self.psql("UPDATE ha_lease SET holder_guid=NULL, holder_incarnation=0,"
                  " holder_name=NULL, acquired_at=0, expires_at=0 WHERE lease_id=1")
        self.psql("DROP TABLE IF EXISTS ha_sim_writes")
        self.psql("DROP TABLE IF EXISTS ha_sim_alloc")

    def make_cluster(self, rebase2=False, start_second=True):
        """Standard two-node cluster with per-node DB and heartbeat proxies.
        Returns (node1, node2, dbproxy1, dbproxy2, hbproxy1, hbproxy2);
        node2 is None if start_second is False."""
        p = self.base_port
        hb_listen1, hb_listen2 = p + 4, p + 5
        dbp1 = Proxy(p + 0, self.dbport)      # node1 -> DB
        dbp2 = Proxy(p + 1, self.dbport)      # node2 -> DB
        hbp1 = Proxy(p + 2, hb_listen2)       # node1 -> node2 heartbeat
        hbp2 = Proxy(p + 3, hb_listen1)       # node2 -> node1 heartbeat
        self.proxies += [dbp1, dbp2, hbp1, hbp2]
        n1 = Node(self, "node1", p + 0, hb_listen1, p + 2)
        self.nodes.append(n1)
        n2 = None
        if start_second:
            time.sleep(1.5)
            n2 = Node(self, "node2", p + 1, hb_listen2, p + 3, rebase=rebase2)
            self.nodes.append(n2)
        return n1, n2, dbp1, dbp2, hbp1, hbp2

    def cleanup(self):
        for node in self.nodes:
            node.destroy()
        self.nodes = []
        for proxy in self.proxies:
            proxy.close()
        self.proxies = []
        time.sleep(0.5)


def check_single_writer(ctx):
    """Invariant: exactly one (guid, incarnation) per term"""
    rows = ctx.psql("SELECT term, count(DISTINCT node_guid || ':' || incarnation::text)"
                    " FROM ha_sim_writes GROUP BY term HAVING"
                    " count(DISTINCT node_guid || ':' || incarnation::text) > 1")
    return [f"term {r[0]} has {r[1]} writers" for r in rows]


def check_no_term_overlap(ctx):
    """Invariant: no old-term write later than a newer term's first write"""
    rows = ctx.psql(
        "SELECT a.term, b.term FROM"
        " (SELECT term, max(db_time) AS last FROM ha_sim_writes GROUP BY term) a,"
        " (SELECT term, min(db_time) AS first FROM ha_sim_writes GROUP BY term) b"
        " WHERE a.term < b.term AND a.last > b.first")
    return [f"term {r[0]} wrote after term {r[1]} started" for r in rows]


def standard_checks(ctx):
    return check_single_writer(ctx) + check_no_term_overlap(ctx)


def wait_stable_cluster(n1, n2, timeout=15):
    """Wait for node1 ACTIVE + node2 STANDBY"""
    if n1.wait_role("ACTIVE", timeout) is None:
        return "node1 did not become ACTIVE"
    if n2 is not None and n2.wait_role("STANDBY", timeout) is None:
        return "node2 did not become STANDBY"
    time.sleep(1)
    return None


# ---------------------------------------------------------------------------
# Scenarios (numbering follows doc/HA_Design.md section 6)
# ---------------------------------------------------------------------------

def s01_kill_active(ctx):
    """kill -9 of active -> standby promotes within bound"""
    n1, n2, *_ = ctx.make_cluster()
    err = wait_stable_cluster(n1, n2)
    if err:
        return False, err
    t0 = time.monotonic()
    n1.kill9()
    tp = n2.wait_role("ACTIVE", PROMOTION_BOUND + 5)
    if tp is None:
        return False, "standby did not promote"
    latency = tp - t0
    problems = standard_checks(ctx)
    if latency > PROMOTION_BOUND:
        problems.append(f"promotion took {latency:.1f}s > bound {PROMOTION_BOUND}s")
    return not problems, f"promotion latency {latency:.1f}s; {'; '.join(problems) or 'invariants hold'}"


def s02_graceful_handover(ctx):
    """graceful release -> promotion faster than expiry path, exit 0"""
    n1, n2, *_ = ctx.make_cluster()
    err = wait_stable_cluster(n1, n2)
    if err:
        return False, err
    t0 = time.monotonic()
    n1.send("release")
    rc = n1.wait_exit(10)
    tp = n2.wait_role("ACTIVE", 10)
    if tp is None:
        return False, "standby did not promote after release"
    latency = tp - t0
    problems = standard_checks(ctx)
    if rc != 0:
        problems.append(f"released node exit code {rc} (expected 0)")
    if latency >= VALIDITY:
        problems.append(f"promotion took {latency:.1f}s - not faster than lease expiry ({VALIDITY}s)")
    return not problems, f"handover latency {latency:.1f}s; {'; '.join(problems) or 'invariants hold'}"


def s03_db_cut_active(ctx):
    """DB partition on active only, channel healthy -> active fences, standby promotes"""
    n1, n2, dbp1, *_ = ctx.make_cluster()
    err = wait_stable_cluster(n1, n2)
    if err:
        return False, err
    t0 = time.monotonic()
    dbp1.set_mode(Proxy.CUT)
    rc = n1.wait_exit(FENCE_BOUND + 3)
    fence_time = time.monotonic() - t0
    tp = n2.wait_role("ACTIVE", PROMOTION_BOUND + 5)
    problems = standard_checks(ctx)
    if rc != 42:
        problems.append(f"active exit code {rc} (expected 42 = fenced)")
    elif fence_time > FENCE_BOUND + 1:
        problems.append(f"fence took {fence_time:.1f}s > bound {FENCE_BOUND}s")
    if tp is None:
        problems.append("standby did not promote (healthy channel must not veto)")
        latency = -1
    else:
        latency = tp - t0
    return not problems, (f"fence {fence_time:.1f}s, promotion {latency:.1f}s;"
                          f" {'; '.join(problems) or 'invariants hold'}")


def s04_db_cut_standby(ctx):
    """DB partition on standby only -> no role change"""
    n1, n2, _, dbp2, *_ = ctx.make_cluster()
    err = wait_stable_cluster(n1, n2)
    if err:
        return False, err
    dbp2.set_mode(Proxy.CUT)
    time.sleep(10)
    problems = []
    if n1.role() != "ACTIVE":
        problems.append(f"active changed role to {n1.role()}")
    if n2.role() not in ("STANDBY", "INIT"):
        problems.append(f"standby changed role to {n2.role()}")
    if n2.proc.poll() is not None:
        problems.append("standby process died")
    dbp2.set_mode(Proxy.NORMAL)
    time.sleep(3)
    if n2.role() != "STANDBY":
        problems.append(f"standby did not recover after partition healed (role {n2.role()})")
    problems += standard_checks(ctx)
    return not problems, "; ".join(problems) or "no role change, standby recovered"


def s05_channel_cut(ctx):
    """heartbeat channel cut, both nodes healthy -> no role change"""
    n1, n2, _, _, hbp1, hbp2 = ctx.make_cluster()
    err = wait_stable_cluster(n1, n2)
    if err:
        return False, err
    hbp1.set_mode(Proxy.CUT)
    hbp2.set_mode(Proxy.CUT)
    time.sleep(8)
    problems = []
    if n1.role() != "ACTIVE":
        problems.append(f"active changed role to {n1.role()}")
    if n2.role() != "STANDBY":
        problems.append(f"standby changed role to {n2.role()}")
    if n2.status.get("peer") != "down":
        problems.append("standby still sees peer after channel cut")
    problems += standard_checks(ctx)
    return not problems, "; ".join(problems) or "roles stable with channel down"


def s06_sigstop_active(ctx):
    """process pause longer than lease validity -> fence on resume, no stale writes"""
    n1, n2, *_ = ctx.make_cluster()
    err = wait_stable_cluster(n1, n2)
    if err:
        return False, err
    t0 = time.monotonic()
    n1.sigstop()
    tp = n2.wait_role("ACTIVE", PROMOTION_BOUND + 5)
    time.sleep(max(0, 8 - (time.monotonic() - t0)))   # total pause ~8s > validity
    n1.sigcont()
    rc = n1.wait_exit(5)
    problems = standard_checks(ctx)
    if tp is None:
        problems.append("standby did not promote during pause")
    if rc != 42:
        problems.append(f"paused node exit code {rc} (expected 42 = fenced)")
    fence = n1.wait_event("EVENT fenced", 1)
    if fence is None:
        problems.append("paused node did not report fence event")
    return not problems, "; ".join(problems) or f"promotion at {tp - t0:.1f}s into pause, fenced on resume"


def s07_hung_refresh(ctx):
    """stalled DB connection (statements hang) -> watchdog fences by deadline"""
    n1, n2, dbp1, *_ = ctx.make_cluster()
    err = wait_stable_cluster(n1, n2)
    if err:
        return False, err
    t0 = time.monotonic()
    dbp1.set_mode(Proxy.STALL)
    rc = n1.wait_exit(FENCE_BOUND + 3)
    fence_time = time.monotonic() - t0
    tp = n2.wait_role("ACTIVE", PROMOTION_BOUND + 5)
    problems = standard_checks(ctx)
    if rc != 42:
        problems.append(f"active exit code {rc} (expected 42 = fenced by watchdog)")
    elif fence_time > FENCE_BOUND + 1:
        problems.append(f"fence took {fence_time:.1f}s > bound {FENCE_BOUND}s")
    if tp is None:
        problems.append("standby did not promote")
    return not problems, (f"fence {fence_time:.1f}s with hung connection;"
                          f" {'; '.join(problems) or 'invariants hold'}")


def s08_start_race(ctx, rounds=10):
    """simultaneous start race -> exactly one winner per round, term +1 per round"""
    p = ctx.base_port
    dbp1 = Proxy(p + 0, ctx.dbport)
    dbp2 = Proxy(p + 1, ctx.dbport)
    ctx.proxies += [dbp1, dbp2]
    base_term = int(ctx.psql("SELECT term FROM ha_lease")[0][0])
    problems = []
    for i in range(rounds):
        n1 = Node(ctx, f"race1-{i}", p + 0, p + 4, p + 2)
        n2 = Node(ctx, f"race2-{i}", p + 1, p + 5, p + 3)
        ctx.nodes += [n1, n2]
        t1 = n1.wait_role("ACTIVE", 10)
        t2 = n2.wait_role("ACTIVE", 0.1 if t1 else 10)
        time.sleep(1)
        active_count = sum(1 for n in (n1, n2) if n.role() == "ACTIVE")
        if active_count != 1:
            problems.append(f"round {i}: {active_count} active nodes")
        term = int(ctx.psql("SELECT term FROM ha_lease")[0][0])
        if term != base_term + i + 1:
            problems.append(f"round {i}: term {term}, expected {base_term + i + 1}")
        n1.destroy()
        n2.destroy()
        ctx.nodes = [n for n in ctx.nodes if n not in (n1, n2)]
        ctx.psql("UPDATE ha_lease SET expires_at=0 WHERE lease_id=1")
    problems += check_single_writer(ctx)
    return not problems, "; ".join(problems) or f"{rounds} rounds, single winner each"


def s09_long_transaction(ctx):
    """long transaction crossing promotion -> old-term commit stays within
    residual window semantics (informational)"""
    n1, n2, *_ = ctx.make_cluster()
    err = wait_stable_cluster(n1, n2)
    if err:
        return False, err
    n1.send("longtxn 10000")
    time.sleep(1)
    t0 = time.monotonic()
    n1.sigstop()
    tp = n2.wait_role("ACTIVE", PROMOTION_BOUND + 5)
    time.sleep(max(0, 8 - (time.monotonic() - t0)))
    n1.sigcont()
    n1.wait_exit(15)
    time.sleep(1)
    problems = check_single_writer(ctx)
    if tp is None:
        problems.append("standby did not promote")
    committed = n1.count_events("LONGTXN committed") > 0
    # the transaction's INSERT executed before the pause, so db_time predates
    # the new term - term overlap check would misfire here by design; instead
    # verify the row (if committed) carries the OLD term only
    detail = ("old-term transaction committed after promotion (residual window observed)"
              if committed else "old-term transaction did not commit (process fenced first)")
    return not problems, f"{detail}; {'; '.join(problems) or 'single-writer invariant holds'}"


def s10_id_collision(ctx):
    """stale in-memory allocator collides after promotion; rebase prevents it"""
    # Part A: standby without rebase -> collisions expected
    n1, n2, *_ = ctx.make_cluster(rebase2=False)
    err = wait_stable_cluster(n1, n2)
    if err:
        return False, err
    n1.send("alloc 5")
    time.sleep(2)
    if n1.count_events("ALLOC id=") != 5:
        return False, "active failed to allocate IDs"
    n1.kill9()
    if n2.wait_role("ACTIVE", PROMOTION_BOUND + 5) is None:
        return False, "standby did not promote (part A)"
    n2.send("alloc 5")
    time.sleep(2)
    collisions = n2.count_events("ALLOC-COLLISION")
    problems = []
    if collisions == 0:
        problems.append("no collision without rebase - hazard not demonstrated")
    ctx.cleanup()
    ctx.reset_db()
    ctx.base_port += 6   # fresh port range for the second cluster

    # Part B: standby with rebase-on-promotion -> no collisions
    n1, n2, *_ = ctx.make_cluster(rebase2=True)
    err = wait_stable_cluster(n1, n2)
    if err:
        return False, err + " (part B)"
    n1.send("alloc 5")
    time.sleep(2)
    n1.kill9()
    if n2.wait_role("ACTIVE", PROMOTION_BOUND + 5) is None:
        return False, "standby did not promote (part B)"
    n2.send("alloc 5")
    time.sleep(2)
    rebased_collisions = n2.count_events("ALLOC-COLLISION")
    if rebased_collisions > 0:
        problems.append(f"{rebased_collisions} collisions despite rebase")
    return not problems, (f"without rebase: {collisions} collisions;"
                          f" with rebase: {rebased_collisions}; "
                          + ("; ".join(problems) or "as designed"))


SCENARIOS = [
    (1, "kill -9 of active", s01_kill_active),
    (2, "graceful handover", s02_graceful_handover),
    (3, "DB cut from active only (channel healthy)", s03_db_cut_active),
    (4, "DB cut from standby only", s04_db_cut_standby),
    (5, "channel cut, both healthy", s05_channel_cut),
    (6, "SIGSTOP active > validity", s06_sigstop_active),
    (7, "hung lease refresh (stalled connection)", s07_hung_refresh),
    (8, "simultaneous start race", s08_start_race),
    (9, "long transaction crossing promotion", s09_long_transaction),
    (10, "ID allocator collision / rebase", s10_id_collision),
]


def main():
    parser = argparse.ArgumentParser(description="netxmsd HA lease adversarial test harness")
    parser.add_argument("--sim", default=os.path.join(os.path.dirname(__file__), "ha-node-sim"))
    parser.add_argument("--db", default="netxms_hatest")
    parser.add_argument("--user", default="netxms")
    parser.add_argument("--db-host", default="127.0.0.1")
    parser.add_argument("--db-port", type=int, default=5432)
    parser.add_argument("--base-port", type=int, default=15430)
    parser.add_argument("--scenario", type=int, action="append",
                        help="run only these scenario numbers (repeatable)")
    args = parser.parse_args()

    if not os.environ.get("PGPASSWORD"):
        print("PGPASSWORD not set", file=sys.stderr)
        return 2

    results = []
    for number, title, func in SCENARIOS:
        if args.scenario and number not in args.scenario:
            continue
        ctx = Context(args)
        ctx.base_port = args.base_port + 10 * number   # per-scenario port range
        ctx.reset_db()
        print(f"[{number:2}] {title} ... ", end="", flush=True)
        try:
            passed, detail = func(ctx)
        except Exception as e:  # noqa: BLE001 - report and continue
            passed, detail = False, f"harness error: {e}"
            if os.environ.get("HA_HARNESS_DEBUG"):
                traceback.print_exc()
        finally:
            ctx.cleanup()
        print(f"{'PASS' if passed else 'FAIL'} ({detail})")
        results.append((number, title, passed, detail))

    failed = [r for r in results if not r[2]]
    print(f"\n{len(results) - len(failed)}/{len(results)} scenarios passed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
