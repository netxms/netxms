# NXCP parser fuzzing

## Overview

Add coverage-guided fuzz tests (libFuzzer + ASan/UBSan) for the NXCP message parser in libnetxms, so that
length-validation mistakes of the kind fixed in `6a7f4b05da` (advisory GHSA-xf27-fr7g-vcrj, 32-bit wrap in
`CalculateFieldSize()`) are found before release. The tree has no fuzzing today.

Result: four fuzz targets under `tests/fuzz/`, built by a new `--enable-fuzzer` configure switch, a checked-in
seed and regression corpus, and a short per-PR CI run. Three parser defects that would stop every fuzz run are
fixed first.

## Scope

In: `NXCPMessage` deserialization and dump, `AbstractMessageReceiver` framing, `NXCPEncryptionContext::decryptMessage()`.

Out, decided during design:
- OSS-Fuzz, AFL++, a dictionary file, scheduled long runs on public runners.
- Server and agent command handlers (consumers of parsed fields).
- The Java `NXCPMessage` parser.
- The stored-notification reader in `src/agent/core/nproc.cpp`, which deserializes without a length; its input
  is the agent's own database, not the network.
- Windows build wiring (`Makefile.w32`): libFuzzer targets are autotools/clang only.
- A public field enumerator on `NXCPMessage`: not added just for the harness.

## Context (from discovery)

- `NXCPMessage::deserialize()` and `NXCPMessage::dump()` take a bare `NXCP_MESSAGE*` and trust the header size.
  The receiver (`AbstractMessageReceiver::getMessageFromBuffer()`) is what guarantees size >= 16, size % 8 == 0
  and size <= buffered bytes for clear-text frames. This is the receiver's contract, not that of every caller.
- `AbstractMessageReceiver::readBytes()` is a protected virtual, so a harness can feed bytes without a socket.
- An oversized header makes the receiver report a protocol error without consuming input; calling
  `readMessage()` again returns the same error forever. Real callers stop on that error.
- `MemoryPool` hands out fields from one region that ASan sees as a single allocation, so overruns between
  fields are invisible. The advisory's own ASan report shows the overflow flagged only at the region end.
- Compressed messages allocate the uncompressed size declared on the wire before inflating. A 24-byte input can
  request gigabytes; libFuzzer's malloc limit would end every run on it.
- Fields are padded to 8 bytes only from protocol version 2; version 1 packs them. `deserialize()` adopts the
  version encoded in the header flags, `dump()` uses its argument.
- `NXCPEncryptionContext` has a private constructor; `create(uint32_t ciphers)` builds a context with a random
  key, and `getSessionKey()` / `getIV()` return the internal buffers that both cipher operations re-read on
  every call.
- `tests` is entered only with `--with-tests` or `--with-dist`, and `tests/Makefile.am` then always builds
  `suite` and `test-libnxdb`. Both `SUBDIRS` lists are configure-substituted and there is no `DIST_SUBDIRS`, so
  the tarball contains exactly what the `--with-dist` block lists.
- The existing sanitizer flag probes test with the C compiler only and just print "no" when a flag is unsupported.
- CI builds with GCC and without `--enable-sanitizer`; no automated run uses the sanitizers today.
- `6a7f4b05da` fixed three validation defects in the same field loop, which produce the same crash stacks.
- The design was cross-checked twice by a second agent and by a plan review; two findings became
  private/netxms#47 and private/netxms#48 on forge.netxms.com.

## Development Approach

- Testing approach: regular (code first, then tests), except Tasks 1-3 where the failing unit test is written
  first because it is the reproducer.
- Unit tests go into `tests/test-libnetxms` and gate each task through the `test-libnetxms` binary, in a normal
  build and in an `--enable-sanitizer` build.
- Fuzz targets are not part of `netxms-test-suite`; they gate through corpus replay (`-runs=0`).
- **Disclosure rule for this work**: the repository is public. An unfixed memory-safety finding is filed in
  `private/netxms` on forge.netxms.com, not in `docs/backlog/` and not described in this file. Do not push this
  branch before Tasks 1-3 are committed, since their descriptions name unfixed defects.
- libFuzzer needs upstream clang. Apple clang and GCC do not ship it, so local runs on macOS need a Linux
  container or an LLVM toolchain; which one is the user's call (no `brew install` without asking).

## Solution Overview

Key decisions and why:

- **libFuzzer behind a configure switch**, not a standalone build script: the targets link the real
  `libnetxms.la` with the product's source list and flags. A script outside autotools would drift.
- **`--enable-fuzzer` implies `--enable-sanitizer`** so the sanitizer flag set exists once. It must be applied
  before the jemalloc mutual-exclusion check.
- **Hard failure at configure time** in fuzz mode when any required piece is missing: the sanitizer compile and
  link flags for C and C++, and libFuzzer itself. The libFuzzer probe links a C++ program that defines only
  `LLVMFuzzerTestOneInput`; a probe with its own `main()` conflicts with the one libFuzzer supplies.
- **Fuzz-only traversal**: `tests/fuzz` is appended to the top-level module list directly, so a fuzz build does
  not need `--with-tests` and does not pull in the test suite and libnxdb. For tarballs it also joins the test
  module list of the `--with-dist` block, and its Makefile defines programs only under the fuzz conditional.
- **Harness input contract** for `deserialize` and `dump`: inputs shorter than 16 bytes are dropped, length is
  made a multiple of 8, the data is copied into an exact-size heap block, and the header size field is set to
  the real length. This reproduces the receiver's guarantee; without it every input is a false report. The
  `receiver` and `decrypt` targets pass bytes through untouched, because they test the code that enforces it.
- **Field discovery without an enumerator**: the parsed message is serialized uncompressed, and every byte
  offset of that output is read as a candidate 32-bit field ID, filtered by `isFieldExist()` and de-duplicated
  before any getter runs. Scanning the raw input instead was rejected: compressed inputs hide their IDs, and
  version 1 messages have IDs at arbitrary offsets. A fixed ID range loses all getter coverage once a mutation
  changes an ID. Walking the serialized fields was rejected because it puts a second parser into the harness.
  Known limit: for duplicate IDs the getters see one entry; the others are covered by serialization only.
- **MemoryPool under ASan** gives each request its own region, so ASan sees the end of every field, and
  `clear()` frees all regions so stale pointers are caught. Underruns into the region's link header stay
  undetected; accepted. This applies to every `--enable-sanitizer` build, not only fuzz builds.
- **Deterministic encryption context**: the `decrypt` target creates one context through the public factory
  with AES-256 only (no fallback mask; other ciphers need the legacy provider on OpenSSL 3, so seeds would not
  be valid everywhere) and overwrites the key and IV buffers with constants. No new API.
- **Seeds come from a generator kept in the tree**: compressed and encrypted seeds cannot be written by hand,
  and binary seeds with no provenance cannot be reviewed.
- **Private campaign before public CI**: a crash in public CI publishes a working reproducer. Existing defects
  are mined privately and fixed first; CI fuzzing stays short and catches regressions from the PR's own diff.

## Implementation Steps

### Task 1: Bound uncompressed size declared by compressed messages

Touches: libnetxms (`src/libnetxms/message.cpp`), unit tests.

A wire-declared size drives the inflate output buffer in three places: the structured and binary branches of
the deserializing constructor, and the structured branch of `dump()` (`dump()` does not inflate binary
messages). The pool size hint is a fourth, independent use, evaluated before the constructor body and for
every message with the compressed flag, including ones that are never inflated.

Reject a declared size that the compressed payload cannot produce. Deflate cannot expand beyond 1032:1, so the
bound is 1032 times the compressed payload length, computed without overflow; a fixed absolute cap was rejected
because the parser does not know the receiver's limit. Keep the existing field reads and add the check after them.

Facts that constrain the fix:
- The size being allocated differs by branch: structured messages use the four bytes after the header, binary
  messages use the header's field-count word. The Java sender writes a different value into those four bytes
  for binary messages than the C++ senders do, so the two must not be required to agree.
- The check belongs to each inflate site, not to a flag. Only the binary branch skips messages with the stream
  flag, because only it skips inflating them; the structured branch and `dump()` inflate regardless of that
  flag and must check regardless of it.

After a successful inflate nothing compares the produced length with the declared one, so a short stream leaves
the tail of the buffer unwritten. Require the lengths to match: the C++ senders (`serialize()`,
`CreateRawNXCPMessage()`) and the Java sender all declare exactly what they deflate.

- [ ] failing unit test in `tests/test-libnetxms/nxcp.cpp`: small compressed structured and binary messages
      declaring a huge uncompressed size are rejected by `deserialize()`; `dump()` reports a format error for them
- [ ] declared size validated against the compressed payload length at the three inflate sites; size hint no
      longer grows from unvalidated metadata
- [ ] inflated length must equal the declared length
- [ ] unit test added for the highest-ratio legitimate case (long run of one byte), structured and binary
- [ ] Java client still exchanges compressed structured and binary messages with the server (one integration
      test from `tests/integration` that transfers a file)
- [ ] `test-libnetxms` passes in normal and `--enable-sanitizer` builds

### Task 2: Fix private/netxms#47 (raw receive buffer used for debug dump)

Touches: libnetxms (`src/libnetxms/msgrecv.cpp`, `include/nxcpapi.h`), unit tests. Callers in server and agent
sessions are not changed; all three dump immediately after a successful read and none keeps the pointer.

Details are in the private ticket. Chosen fix: `getRawMessageBuffer()` keeps pointing at the message just
returned until the next `readMessage()` call, by deferring the buffer shift. Dumping from the parsed
`NXCPMessage` instead was rejected because it loses the raw hex dump that level 8 exists for.

Only a successfully returned message defers its shift. Cases that need care:
- An encrypted message arriving with no encryption context is consumed without returning a message or an
  error, and the same `readMessage()` call goes on to read more; that consumption must stay immediate.
- Decryption overwrites the envelope in place, including its size field. The consumed length is the original
  envelope length, captured before decryption.
- The pending shift is applied before any parse, read, grow or skip work, including calls with
  `allowReadBytes` false. The skip logic assumes remaining data starts at offset zero.
- A decryption failure is not terminal for every caller: the agent session and the mobile device session keep
  reading after it.

- [ ] failing unit test with a test receiver subclass: two messages delivered in one read, and a message
      followed by a partial header; the raw buffer after each `readMessage()` holds the returned message
- [ ] unit tests for the cases above: encrypted message without context followed by a clear one in the same
      read, encrypted message followed by another message, read continuing after a decryption failure
- [ ] buffer shift deferred for returned messages only; grow and skip paths unchanged in behaviour
- [ ] `test-libnetxms` passes in normal and `--enable-sanitizer` builds
- [ ] ticket updated with the fix commit

### Task 3: Fix private/netxms#48 (validation of decrypted messages)

Touches: libnetxms (`src/libnetxms/crypto.cpp` or `src/libnetxms/msgrecv.cpp`), unit tests.

Details are in the private ticket. A decrypted message must meet the same contract as a clear-text frame before
it is parsed; a violation is reported the way a decryption failure is. Where the check lives is the
implementer's choice: `decryptMessage()` already rejects one inconsistent case, so extending it keeps the
checks together.

- [ ] failing unit test: envelopes built with a test context whose inner message violates the contract are
      rejected by the receiver with a decryption or protocol error
- [ ] valid encrypted messages still pass, including the existing encrypted file transfer test
- [ ] `test-libnetxms` passes in normal and `--enable-sanitizer` builds
- [ ] ticket updated with the fix commit

### Task 4: Per-request MemoryPool regions under address sanitizer

Touches: libnetxms (`src/libnetxms/mempool.cpp`), unit tests.

With `WITH_ADDRESS_SANITIZER`, every `MemoryPool::allocate()` request becomes its own region in the existing
linked list: the existing aligned link header, then an exact-size payload (no rounding up to 8), with the
shared-region fast path never taken. The list linkage stays, so destructor, move assignment and
`getRegionCount()` keep their logic. `clear()` frees every region in this mode instead of retaining the newest
one, so a pointer used after `clear()` is reported. The `ObjectMemoryPool` template is a separate class and is
not changed.

What this detects: overruns past the end of an allocation, and use after `clear()`. What it does not: underruns
into the link header. zlib's free callback stays a no-op, so inflate scratch memory lives until the pool goes.

The change affects every pool user in a sanitizer build (NXSL, SNMP, log parser, `StringList`, `NXCPMessage`),
and code that silently relied on the 8-byte round-up will now be reported. The two `clear()` callers were
checked and neither keeps pointers across it.

- [ ] allocation and `clear()` switched under `WITH_ADDRESS_SANITIZER`; non-sanitizer behaviour unchanged
- [ ] region-count assertions in `tests/test-libnetxms/mempool.cpp` hold in both build modes
- [ ] unit test added for `clear()` followed by allocation (not covered today)
- [ ] verified by hand once in a sanitizer build: a one-byte overrun of a pool allocation, and a read after
      `clear()`, are both reported by ASan
- [ ] full `netxms-test-suite` passes once in an `--enable-sanitizer` build; anything it reports is fixed or
      ticketed before continuing

### Task 5: `--enable-fuzzer` build and the `dump` target

Touches: build system (`configure.ac`, new `tests/fuzz/Makefile.am`), new fuzz target, corpus.

`fuzz-nxcp-dump` is the smallest target and proves the build. Targets are `noinst_PROGRAMS` linked against the
build-tree `libnetxms.la`; instrumentation (`-fsanitize=fuzzer-no-link`) is added tree-wide for C and C++, and
the targets themselves link with `-fsanitize=fuzzer`. Linking with `-no-install` avoids libtool wrapper
scripts, which get in the way of coverage reports and CI artifacts.

The `dump` target passes the header's encoded version when present and the current version otherwise, so the
unpadded version 1 layout is exercised.

To settle here: whether libtool passes `-fsanitize=fuzzer-no-link` through at link time unchanged. A bare
`./configure --enable-fuzzer` should be the minimal invocation, since the default module set is libnetxms only.

- [ ] `./configure --enable-fuzzer` fails with a clear message on GCC and Apple clang, succeeds on upstream clang
- [ ] a default build and a `--with-tests` build are unaffected (no new flags, no fuzz programs built)
- [ ] `make dist` from a `--with-dist` tree contains `tests/fuzz`, and the tarball configures and builds
- [ ] `fuzz-nxcp-dump` builds, replays a hand-made seed and the advisory's 32-byte regression input cleanly
- [ ] coverage of `message.cpp` grows during a short run (libFuzzer `cov:` rises beyond the harness itself)

### Task 6: `deserialize` target and the seed generator

Touches: new fuzz target, seed generator, corpus.

`fuzz-nxcp-deserialize` exercises, for each discovered field: every typed getter, the caller-buffer string
getters with tiny buffers as well as the allocating variants, binary, integer array, address, MAC, GUID and
JSON getters. It also serializes the parsed message, compressed and uncompressed. Allocated results are
released; the internal pointer from `getBinaryFieldPtr()` is not.

A seed generator program in `tests/fuzz/` writes valid messages through `NXCPMessage`: one per field type,
protocol versions 1 and current, binary and control messages, compressed structured and binary. Its output
replaces the hand-made seed from Task 5; the advisory's regression input stays as a hand-added file.

The vulnerable baseline, used here and in Tasks 7 and 8, is the branch head with `6a7f4b05da` reverted in a
scratch worktree, keeping Tasks 1-4. The untouched parent commit has no harnesses. The revert conflicts with
Task 1, which edits the same code, and needs manual resolution.

- [ ] target builds and replays the generated corpus cleanly
- [ ] a compressed input that inflates into many fields with one ID does not time out (candidate de-duplication)
- [ ] on the vulnerable baseline, the regression input fails in `deserialize` and in `dump`; on the branch head
      both replay cleanly

### Task 7: `receiver` and `decrypt` targets

Touches: new fuzz targets, corpus. Depends on Tasks 2 and 3.

`fuzz-nxcp-receiver`: a receiver subclass serving the input unchanged, in a mix of small chunks and coalesced
reads chosen from the input; initial buffer small but at least one header, small explicit maximum size so the
grow and skip branches are reachable; stops at the first non-success result; after each message calls
`NXCPMessage::dump()` on the raw buffer the way the sessions do.

`fuzz-nxcp-decrypt`: built only with encryption support (existing `USE_ENCRYPTION` conditional). The first
input byte selects the mode. Envelope mode passes the rest to a receiver with the context set. Inner-frame mode
treats the rest as a clear-text frame, encrypts it with the context and feeds that, which gets past the
checksum. Unlike `receiver`, this target keeps reading after a decryption failure, as the agent and mobile
sessions do; each failure consumes input, so the loop is bounded. Its receiver's maximum size must exceed the
inner-frame cap plus envelope overhead, or valid frames go to the skip path.

`encryptMessage()` trusts the header it is given: it reads the flags and the declared size before anything
else and uses that size for the checksum, the allocation and the cipher input. Passing fuzz bytes to it
untouched would make the harness itself read out of bounds. Inner-frame mode therefore keeps the declared
size as the fuzzer wrote it, but copies the frame into a zero-padded buffer of at least that size and at least
one header, and drops inputs whose declared size exceeds a fixed cap. Rewriting the declared size to the real
length was rejected because it removes the malformed inner frames that Task 3 guards against. Building the
envelope by hand in the harness was rejected as a second implementation of the envelope format.

Crypto library initialization: the context factory works without `InitCryptoLib()` on OpenSSL 1.1 and later,
and without it the do-not-encrypt shortcut in `encryptMessage()` is inactive. The target does not call it.

Seeds: multi-message streams for `receiver`; encrypted messages produced with the constant key for `decrypt`.

- [ ] both targets build and replay their corpus cleanly
- [ ] `receiver` reaches the grow, skip and garbage-resync branches in a short run (check with a coverage report)
- [ ] `decrypt` inner-frame mode produces messages that decrypt and deserialize successfully for valid input
- [ ] on the vulnerable baseline, the regression input fails in `receiver`

### Task 8: Private campaign and acceptance measurements

Touches: whatever the findings touch; corpus.

Run each target for hours on a private machine. Every finding is triaged: memory-safety issues and anything
reachable before login go to `private/netxms`; each fix lands with its minimized reproducer added to the
target's corpus. Resource limits for all runs: explicit RSS and malloc limits, a per-input timeout of a few
seconds, UBSan halting on error.

Rediscovery is a measurement, not a gate. On the vulnerable baseline, with the regression input removed, record
for a few fixed `-seed` values and a fixed time budget whether each target finds a crash input that replays
cleanly on the branch head, that is, any of the three defects `6a7f4b05da` fixed. Their stacks are identical,
so the measurement does not try to tell them apart. A target that never finds one gets its seeds or harness
reworked.

- [ ] campaign run for all four targets; duration and executions recorded in the README
- [ ] all findings fixed or ticketed privately; reproducers committed with their fixes
- [ ] rediscovery measurement recorded for `deserialize`, `dump` and `receiver`
- [ ] all four targets replay their final corpus cleanly on the branch head

### Task 9: CI job and documentation

Touches: `.github/workflows/`, `tests/fuzz/README.md`, `tests/CLAUDE.md`.

Enabled only after Task 8. A separate workflow with clang on Ubuntu, one job per target so one failure does not
hide the others: build libnetxms and `tests/fuzz`, replay the corpus, then fuzz about two minutes on a writable
copy of the corpus with the Task 8 resource limits, and upload the failing input as an artifact on failure.

README covers: configure and build, running a target, reproducing from a crash file, adding a reproducer to the
corpus, regenerating seeds, the input contract of the `deserialize` and `dump` targets, and the disclosure
rule. `tests/CLAUDE.md` gets the build gating of `tests/fuzz` and a pointer to the README.

- [ ] workflow passes on the branch; a deliberately broken parser check makes the matching job fail and upload its input
- [ ] README and `tests/CLAUDE.md` written
- [ ] move this plan to `docs/plans/completed/`

## Tandem

| Task | Writer | Reviewer |
|------|--------|----------|
| 1 | claude | codex |
| 2 | claude | codex |
| 3 | claude | codex |
| 4 | codex | claude |
| 5 | claude | codex |
| 6 | claude | codex |
| 7 | claude | codex |
| 8 | claude | codex |
| 9 | codex | claude |

Decisions taken by the two agents before the run, without the user:

- ⚠️ Fuzz builds and runs use a Linux container with upstream clang through the Docker installation already
  on the machine (the host has Apple clang only). No toolchain is installed on the host.
- ⚠️ "ticket updated with the fix commit" in Tasks 2 and 3 is left to the user: the commits are local, get new
  hashes when the branch is finalized, and writing to forge is outside this worktree.
- ⚠️ Task 9 "workflow passes on the branch" cannot be checked without pushing. The workflow's commands are run
  in the container instead; the GitHub run and the artifact upload are reported as not verified.
- ⚠️ Task 8 campaign runs in the container on this machine. Its duration is agreed between the agents before it
  starts; actual durations and execution counts are recorded.
- ⚠️ No public GitHub issue exists for this work, so commit subjects carry no issue reference. None is created
  or invented during the run.

## Post-Completion

- A GitHub issue for the fuzzing work is needed for commit subjects; Tasks 1-3 are security fixes and their
  public wording and release timing (maintenance branch backport, advisory or not) are the maintainers' call.
- Decide whether long private runs become a routine (for example before each release) and on which machine.
- No CI job runs the sanitizers on the regular test suite today; Task 4 makes such a job more valuable.
- Follow-up candidates once this is in place: fuzzing other wire parsers (SNMP PDU decoding, syslog), and the
  command handlers that consume parsed fields.
