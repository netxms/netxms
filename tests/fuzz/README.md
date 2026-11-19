# NXCP parser fuzz targets

Coverage-guided fuzz tests ([libFuzzer](https://llvm.org/docs/LibFuzzer.html) with address and undefined
behavior sanitizers) for the NXCP message code in libnetxms.

| Target | Code under test | Corpus |
|--------|-----------------|--------|
| `fuzz-nxcp-deserialize` | `NXCPMessage::deserialize()`, every field getter, `serialize()` | `corpus/message` |
| `fuzz-nxcp-dump` | `NXCPMessage::dump()` | `corpus/message` |
| `fuzz-nxcp-receiver` | `AbstractMessageReceiver` framing of clear text messages, then `dump()` of the raw buffer | `corpus/receiver` |
| `fuzz-nxcp-decrypt` | the receiver with an encryption context, `NXCPEncryptionContext::decryptMessage()` | `corpus/decrypt` |

## Reporting what a target finds

This repository is public. A crash input is a working reproducer, so:

- do not attach a crash input or a sanitizer report for an unfixed defect to a public issue, pull request or
  commit;
- report a memory safety finding privately to the maintainers, and add its input to the corpus in the commit
  that fixes it.

## Building

libFuzzer is part of upstream clang. GCC and Apple clang do not provide it, and `configure` stops with an
error when it is missing. On macOS use a Linux container.

```bash
./init-source-tree
CC=clang CXX=clang++ ./configure --enable-fuzzer
make -j$(nproc)
```

`--enable-fuzzer` implies `--enable-sanitizer`, instruments the whole build for coverage and adds `tests/fuzz`
to the build. Nothing else is needed: the default module set is libnetxms, which is all the targets link.
The targets are not installed; run them from `tests/fuzz`.

## Running a target

Fuzz on a copy of the corpus. libFuzzer writes every new input it keeps into the first directory it is given,
and those do not belong in the repository.

```bash
cd tests/fuzz
mkdir -p /tmp/corpus /tmp/artifacts
cp corpus/message/* /tmp/corpus/
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 ./fuzz-nxcp-deserialize \
   -max_total_time=600 -max_len=4096 -timeout=5 -rss_limit_mb=2048 -malloc_limit_mb=512 \
   -artifact_prefix=/tmp/artifacts/ /tmp/corpus
```

The limits matter. Without `halt_on_error` an undefined behavior report is printed and the run goes on.
Without `-malloc_limit_mb` and `-rss_limit_mb` a single input can take the machine's memory.

For a long run add `-fork=2 -ignore_crashes=1 -ignore_timeouts=1 -ignore_ooms=1`, so that the run continues
after a finding and collects all of them in the artifact directory. In this mode libFuzzer itself leaks 16
bytes at exit, which LeakSanitizer reports and libFuzzer records as an empty `crash-da39a3ee...` file. That
file is not a finding.

To replay the checked-in corpus without fuzzing, which is what the first CI step does:

```bash
./fuzz-nxcp-deserialize -runs=0 corpus/message
```

## Reproducing a failure

Pass the artifact file instead of a corpus directory. The sanitizer report is printed again:

```bash
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 ./fuzz-nxcp-deserialize /tmp/artifacts/crash-1dbf9710...
```

To get a smaller input for the same failure:

```bash
./fuzz-nxcp-deserialize -minimize_crash=1 -runs=100000 /tmp/artifacts/crash-1dbf9710...
```

## Adding a reproducer to the corpus

In the commit that fixes the defect, copy the (minimized) input into the corpus directory of the target as
`regression-<what-it-was>`, and check that it replays cleanly with the fix and fails without it. Add a unit
test in `tests/test-libnetxms` as well: fuzz targets are not built by a regular test build.

Corpus file names:

- `seed-*`: written by the seed generator, never edited by hand;
- `handmade-*`: valid or deliberately odd input written by hand;
- `regression-*`: input that used to fail.

## Regenerating seeds

Encrypted and compressed seeds cannot be written by hand, so all `seed-*` files come from
`fuzz-nxcp-seedgen`, which builds them through `NXCPMessage` and the fixed-key encryption context that the
`decrypt` target uses. Run it after changing the generator or the wire format, and commit the result:

```bash
cd tests/fuzz
./fuzz-nxcp-seedgen corpus
```

Its output is deterministic. A seed that changes without a reason is a change in what libnetxms puts on the
wire.

## Input contract of the `deserialize` and `dump` targets

`NXCPMessage::deserialize()` and `NXCPMessage::dump()` take a bare `NXCP_MESSAGE*` and trust the size in its
header. In the product the message receiver guarantees that this size is at least one header, a multiple of 8
and not larger than the data it has. These two targets reproduce that guarantee instead of testing it:

- input shorter than 16 bytes is dropped, and the length is rounded down to a multiple of 8;
- the data is copied into a heap block of exactly that length;
- the size field of the header is overwritten with that length.

The `deserialize` target also drops input with protocol version 1 to 3 in the header. Versions below 4 are
going to be rejected by the product, and `serialize()` of such a message accesses an unaligned UCS-2 string.
Version 0 is kept: the parser treats it as the current version. The `dump`, `receiver` and `decrypt` targets
still take every version.

So the size field of an input for these targets is ignored. A report from them means the parser read or wrote
outside a message whose length it was told correctly. The `receiver` and `decrypt` targets pass their input
through untouched, because they test the code that enforces the guarantee.

Input of the `decrypt` target starts with a mode byte. Even: the rest is fed to the receiver as is, as a
stream of encrypted message envelopes. Odd: the rest is one clear text frame that the target encrypts with its
fixed key and then feeds to the receiver, which gets fuzzer-controlled content past the envelope checksum.

## Campaign record

Runs longer than the CI job are done privately, before a release or after a change to the parser, and
recorded here.

| Date | Target | Duration | Executions | Findings |
|------|--------|----------|------------|----------|
| 2026-11-17 | `deserialize` | 2 h | 71.9 million | none |
| 2026-11-17 | `dump` | 2 h | 36.0 million | none |
| 2026-11-17 | `receiver` | 2 h | 66.2 million | none |
| 2026-11-17 | `decrypt` | 2 h | 29.8 million | none |

Conditions: Linux container (Ubuntu 24.04, clang 18, arm64), all four targets in parallel with `-fork=2`,
`-max_len=4096 -timeout=5 -rss_limit_mb=2048 -malloc_limit_mb=512`, undefined behavior reports fatal. The
`dump` target was built before `dump()` got its larger output buffer step.

The sanitizer build of the test suite and shorter runs while the targets were being written found five
defects. Four are fixed, with a unit test or a `regression-*` corpus file where one applies: a read past the
address in `Ip6ToStr()`, a write past a short destination in the UTF-8 and UTF-16 encoders, pointer arithmetic
on a null pointer in the configuration parser, and slow `dump()` output. The fifth, an unaligned UCS-2 access in
`serialize()` for protocol version 1, is not fixed: versions below 4 are going to be rejected instead, and until
then the `deserialize` target drops such input.

Rediscovery check: with the field size validation of commit `6a7f4b05da` reverted and its regression input
removed from the corpus, each of `deserialize`, `dump` and `receiver` found a crashing input in under two
seconds for `-seed=1`, `2` and `3` (43 to 2995 executions), and every one of those inputs replays cleanly on
the fixed code.

## Known limits

- `NXCPMessage` has no field enumerator. The `deserialize` target finds field IDs by trying every 32 bit value
  of the re-serialized message. When a message carries one ID several times, the getters see one of the
  entries.
- A compressed message of 4096 bytes can inflate into several hundred thousand fields. `dump()` of one takes
  seconds under the sanitizer, so with a larger `-max_len` the `dump` target can report a timeout for an input
  that is slow, not wrong.
- `MemoryPool` gives every allocation its own region in a sanitizer build, so an overrun of a message field is
  reported. An underrun into the region header is not.
- Both receiver targets dump with the current protocol version, not a negotiated one.
