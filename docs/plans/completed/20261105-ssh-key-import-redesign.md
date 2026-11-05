# SSH key import/edit redesign

## Overview

The SSH Keys import dialog asks for a public key and a private key, both mandatory, and the edit dialog hides
both boxes behind a "Replace keys" checkbox. The redesign makes the private key the only key material a user
supplies: the server derives the public key from it, the import dialog shrinks to Name + Private key, and the
edit dialog shows the current public key read-only with an optional "new private key" box.

Commit subjects need a tracked issue number: file the GitHub issue before the first task is committed.

## Discovery facts that drive the design

- The agent (`src/agent/subagents/ssh/session.cpp`, `SSHSession::connect`) uses the public key only for an
  `ssh_userauth_try_publickey` probe; authentication itself uses the private key alone
  (`ssh_pki_import_privkey_base64` with no passphrase). Passphrase-protected keys therefore never work and are
  rejected at import.
- The NETCONF subagent (`src/agent/subagents/netconf/keys.cpp`, `session.cpp`) is a second consumer of stored
  keys, with the same probe-then-private-key login.
- The public key's real consumer is "Copy public key to clipboard" in the SSH Keys view, so it must stay stored
  and populated — hence server-side derivation instead of simply making the field optional.
- The server never sends the private key to the client, so on edit the private key box is always empty;
  "empty = keep current key" costs nothing.
- `SshKeyPair::generate` (server-generated keys) produces PKCS#1 PEM RSA. It stays as is; generated keys never
  pass through the import parser, and a rename-only edit must not parse or re-derive anything.
- **Agent libssh floor.** libssh 0.7.x reads only Ed25519 from the OpenSSH private key format; RSA and ECDSA in
  that format fall into its "unsupported" branch (`pki_openssh_import_privkey_blob`, libssh 0.7.3). PEM imports
  worked with any libssh. Agents are not version-matched to the server, so the floor per key type (believed to
  be libssh 0.8.0 for RSA/ECDSA) must be confirmed on the oldest supported agent build and stated in release
  notes. This is not a reason to accept PEM — supported agent platforms ship 0.8+ — but it must not be
  advertised as "0.7+".
- `base64_decode` (`src/libnetxms/base64.cpp`) skips CR/LF only at the start of a 4-character group, and
  `ssh-keygen` wraps the body at 70 columns. The armored body has to be stripped of line breaks before decoding,
  otherwise every real key is rejected.
- Name buffers truncate silently today: the NXCP `SshKeyPair` constructor reads straight into `m_name[255]`, the
  JSON path converts into `name[255]`, and `H_SshKeyCreate`'s generate branch into `name[256]`;
  `GenerateSshKey` validates nothing. Length must be checked on the full input before any copy.
- Test wiring: `tests/test-libnxcore` links libnxcore (parser unit tests); `tests/test-server` runs a live server
  core inside `netxms-test-suite`, which CI (`.github/workflows/build_and_test.yaml`) installs and runs.
  `GetSshKeyByIdAsJson` omits the private key, and `FindSshKeyById` (returns both keys in an NXCP message, used
  for agents) is not exported. DB reads right after a save don't work as an observation — `saveToDatabase` is
  queued. No SSH key test exists in `tests/integration`.

## Decisions

- **OpenSSH private key format only** (the `openssh-key-v1` container with its `BEGIN OPENSSH ...` armor). It is the ssh-keygen default
  since 7.8 and carries the public blob. PEM/PKCS#8 imports are rejected; the error message tells the user to
  convert with `ssh-keygen -p -N "" -f <file>` (`-N ""` matters: `-p` alone can leave the key encrypted).
- **Structural validation, no crypto.** netxmsd does not link libssh and adding it for one parser is out of
  proportion. The parser checks the format end to end but cannot detect a corrupted private scalar (e.g. a
  flipped bit in RSA `d`). Code comments and review text say "structurally valid", never "will authenticate".
- **Key type allowlist:** `ssh-rsa`, `ssh-ed25519`, `ecdsa-sha2-nistp256/384/521`. `sk-*` (hardware keys need
  the device, which an unattended agent doesn't have), certificates and unknown types are rejected.
- **Client-supplied public key is ignored** on both NXCP and WebAPI; the derived one is stored. Old consoles
  that still send `VID_PUBLIC_KEY` keep working for OpenSSH keys.
- **No new RCC.** Core returns `RCC_INVALID_ARGUMENT` for key errors and the existing `RCC_INVALID_OBJECT_NAME`
  for name errors, so either RCC is unambiguous for every caller without handlers repeating validation rules.
  The console still handles `RCC_INVALID_OBJECT_NAME` like any other error rather than assuming it can't occur.
- **Name rules**, applied in core by both `CreateOrEditSshKey` overloads *and* `GenerateSshKey`: non-blank after
  trimming, at most 254 `wchar_t` characters, checked on the complete input before any buffer copy — longer
  names are rejected, not truncated. The console limits input to 254 UTF-16 units, which is never more permissive
  than the server on any platform.
- **Stored private key is the normalized text** (LF line endings, surrounding whitespace trimmed), not the raw
  paste, so agents don't depend on libssh's tolerance for CRLF. The parser produces it, so normalization lives
  in one place.
- **Master only**, no backport, no client/server version gating: console and server are expected to match.
- **`SshKeyPair.setKeys(pub, priv)` is replaced** by `setPrivateKey(priv)` with no deprecated forwarder (no
  in-tree callers besides the dialog; repo policy is no transitional paths).
- **Console retry by reopening**, not by saving from inside the dialog (see Task 4).

### Ruled out

- Public key as an optional input — invites mismatched pairs; derivation covers every supported import type.
- A "derive public key" button in the dialog — needs a round trip or client-side parsing for no gain over
  deriving on save.
- PEM RSA/EC/Ed25519 import — extra OpenSSL 1.x/3.x encoding paths for keys nobody generates by default anymore.
- Re-deriving public keys of existing rows via DB upgrade — existing keys stay untouched.
- Saving from `okPressed` with the dialog staying open on error — a new pattern in nxmc that needs
  double-submit, disposed-widget and cancel-during-save handling.

## Implementation Steps

Testing approach: **TDD** — write the failing tests first within each task.

### Task 1: OpenSSH private key parser

**Touches:** server core, unit tests.

New `NXCORE_EXPORTABLE` function in `src/server/core/sshkeys.cpp`, declared in `src/server/include/nms_objects.h`
next to the other SSH key functions. Input: pasted private key text and key name. On success it returns both the
normalized private key text and the derived public key line `<type> <base64(envelope public blob)> <utf8 name>`
(same comment convention as `generate()`); on rejection, nothing. Task 2 stores exactly what it returns. It logs
the rejection reason at debug level under a new tag `ssh.keys` (add to `doc/internal/debug_tags.txt`;
`sshkeys.cpp` has no tag today).

Validation rules (the substance of this task):

- Armor: exact OpenSSH BEGIN/END armor lines, surrounding whitespace and CRLF tolerated, exactly one
  block, nothing after END. Input capped at 16 KB (a 16384-bit RSA key is about 13 KB).
- Body: strip all CR/LF before decoding (see the base64 fact above), then strict base64 alphabet; check both
  `base64_decode_alloc`'s result and a null output buffer.
- Every length read, including partial uint32 fields, is checked against the remaining bytes without
  `offset + len` overflow.
- Identifiers (magic, cipher, KDF, key type, curve) compared by exact length plus bytes — never `strcmp`, which
  would accept an embedded NUL followed by junk.
- Envelope: magic `openssh-key-v1\0`; ciphername `none`; kdfname `none`; empty kdfoptions; nkeys == 1; public
  blob consumed exactly (no bytes after its last field — the blob becomes the stored public key); private
  section string; nothing after the private section in the decoded data.
- Private section: checkint1 == checkint2; key type equals the envelope blob's type and is on the allowlist;
  per-type structure, with public fields agreeing with the envelope blob —
  - RSA: envelope is `type,e,n`, private is `type,n,e,d,iqmp,p,q` (not a contiguous byte-equal prefix). Every
    mpint canonical, positive and non-empty (no negative encoding, no redundant leading zero byte); `e` and `n`
    equal across sections.
  - Ed25519: public field exactly 32 bytes, private field exactly 64 bytes, private field's last 32 bytes equal
    the public field.
  - ECDSA: curve string equals the algorithm suffix (`nistp256` etc.) in both sections; Q is an uncompressed
    point (`0x04` prefix) of the curve's length and equal across sections; private scalar a canonical positive
    mpint.
- Comment string consumed before padding; padding bytes are 1,2,3,…; zero padding allowed when already aligned;
  private section length is a multiple of 8 (the `none` cipher block size).

Fixture sources: valid keys come from `ssh-keygen` with `-C ""` (current `ssh-keygen -y` prints the embedded
comment, so compare only the first two fields otherwise). An `sk-ssh-ed25519` key can't be generated without a
FIDO authenticator — take it from OpenSSH's `regress/unittests/sshkey/testdata`. Structural reject cases are
built by mutating a decoded valid key in the test, not generated.

- [x] fixtures embedded in the test source: ed25519, rsa, ecdsa 256/384/521, one key whose private section needs
      zero padding, CRLF and trailing-newline variants, passphrase-encrypted key, PEM RSA key, `sk-ssh-ed25519`
      key. Two scanners flag embedded keys: the pre-commit `detect-private-key` hook (`.pre-commit-config.yaml`
      already excludes `src/server/core/sshkeys.cpp` — add the new test files there) and semgrep
      (`detected-private-key` is ERROR severity; run it locally and add `// nosemgrep: <rule-id>` if flagged)
- [x] tests in `tests/test-libnxcore` (new source file, registered in `Makefile.am`, `Makefile.w32` and
      `test-libnxcore.cpp`): every valid fixture derives type + blob equal to `ssh-keygen -y` with the key name
      as comment, and CRLF/whitespace input returns LF-normalized trimmed private text; every reject class from
      the rules above fails — including Ed25519 31/33-byte public and 63/65-byte private fields, mismatched
      trailing public half, curve suffix/string mismatch, non-empty kdfoptions, type mismatch between sections,
      non-canonical mpints, truncated uint32, input over 16 KB, misaligned private section
- [x] parser implemented; `tests/test-libnxcore/test-libnxcore` passes

➕ API: `bool ParseOpenSshPrivateKey(const char *text, const wchar_t *name, char **privateKey, char **publicKey)`,
both outputs `MemAlloc`'d. Body lines must be non-empty; an empty line inside the body is rejected like any other
non-alphabet content. Semgrep (CI rule sets) reported nothing on the fixtures, so no `nosemgrep` was needed.

### Task 2: Server import/edit/generate path and Java client library

**Touches:** server core, NXCP session, Java client library, server tests, integration tests.

- Both `CreateOrEditSshKey` overloads and `GenerateSshKey` apply the name rules (→ `RCC_INVALID_OBJECT_NAME`).
- `CreateOrEditSshKey` calls the parser whenever a private key is present and stores what it returns (normalized
  private text + derived public key); parser failure → `RCC_INVALID_ARGUMENT`, nothing stored.
- Create (id 0) without a private key fails with `RCC_INVALID_ARGUMENT`.
- Edit with an unknown or deleted id → `RCC_INVALID_SSH_KEY_ID`. This fixes the existing null dereference in the
  NXCP overload (`updateKeys(oldKey)` with a missing key).
- Edit without a private key keeps both stored keys (rename only, no parsing).
- JSON overload: absent `privateKey` on edit = keep; present but not a string, empty, or invalid = fail.
- NXCP overload returns an RCC instead of `void`; `ClientSession::updateSshKey` sends it back instead of
  unconditional success.
- `SshKeyPair` (Java): `setKeys` → `setPrivateKey`; `fillMessage` sends `VID_PRIVATE_KEY` only when non-null and
  never `VID_PUBLIC_KEY`. The one call in `SSHKeyEditDialog.okPressed` switches to `setPrivateKey` in this task
  so the console keeps compiling; the dialog rewrite is Task 4.
- Exports for `tests/test-server` on Windows: the NXCP `CreateOrEditSshKey` overload and `FindSshKeyById` get
  `NXCORE_EXPORTABLE`; `FindSshKeyById` is the test's way to observe stored private text.
- The per-ID update race stays out of scope (`docs/backlog/ssh-key-update-not-serialized-per-id.md`).

- [x] new `tests/test-server/sshkeys.cpp` (registered in `Makefile.am`, `Makefile.w32`, `test-server.cpp`),
      written first, calling the core functions directly and observing both keys through `FindSshKeyById`
      before and after each change; removes every key it creates. Cases: valid import stores the derived public
      key and normalized private text (CRLF + surrounding whitespace input); junk / encrypted / PEM →
      `RCC_INVALID_ARGUMENT`, no key created; blank name, 255-char name and oversized raw NXCP/JSON input →
      `RCC_INVALID_OBJECT_NAME` on import and generate, 254-char name accepted; create without private key fails;
      rename of a generated (PEM RSA) key keeps both keys; replacement with a bad key keeps both old keys; edit of
      a deleted id → `RCC_INVALID_SSH_KEY_ID` (no crash); JSON present-but-invalid `privateKey` fails. Gate:
      `make install` to the configured prefix, then `tests/suite/netxms-test-suite` (it runs the installed
      binaries from `BINDIR`, like CI)
- [x] `tests/integration/.../SshKeyTest.java` for the client-library path only: `setPrivateKey` import succeeds
      and the listed public key is the derived one (the client never receives the private key, so the public key
      is the only observable); gate: `mvn -f tests/integration/pom.xml test -Dtest=SshKeyTest`

➕ Core order: unknown id is checked before the name, so an edit of a deleted key reports `RCC_INVALID_SSH_KEY_ID`
whatever the name. A missing or non-string JSON `name` is `RCC_INVALID_OBJECT_NAME` in core, matching the name
rules. Both overloads share one static `StoreSshKey`; the NXCP `SshKeyPair` constructor and `updateKeys` are gone.
A rename keeps the stored public key line as is, including its old comment (no re-derivation, as decided).
➕ Gate note: in a `--with-private-extensions` tree `test-server` stops at "Missing NetXMS EE license"; the suite was
run with a scratch `NETXMS_HOME` whose `nxdbmgr` installs a license after `init`. `SshKeyTest` ran against a
throwaway SQLite server on port 14801.

### Task 3: WebAPI handlers and spec

**Touches:** WebAPI, OpenAPI spec.

- `POST /v1/ssh-keys` imports whenever `privateKey` is present (including explicit `null`, numbers or empty
  string, which fail) and generates only when it is absent. Requests whose body is not a JSON object are
  rejected with 400 in the handler before core.
- POST and PUT map core RCCs instead of repeating the rules: `RCC_INVALID_OBJECT_NAME` → 400 "invalid name",
  `RCC_INVALID_ARGUMENT` → 400 "invalid private key" (unencrypted OpenSSH required), `RCC_INVALID_SSH_KEY_ID` →
  404, anything else → 500. POST passes the name to core untruncated (no more `name[256]`).
- An obsolete `publicKey` property in a request is accepted and ignored.
- `src/server/webapi/openapi.yaml`: drop `publicKey` from `SshKeyCreate` / `SshKeyUpdate`, add `maxLength: 254`
  to `name`, document `privateKey` as an unencrypted OpenSSH private key (absent on POST = generate, absent on
  PUT = keep); keep `publicKey` in `SshKey` (response). Update the `createSshKey` / `updateSshKey` operation
  descriptions (the create one still says import needs both keys) and their 400 response descriptions. Regenerate
  the `/webapi-spec` command afterwards (`~/.claude/scripts/gen-webapi-spec.py`).

- [x] verified by hand against a running server (no WebAPI integration harness):
  - POST with only `privateKey` imports that key; with an extra mismatched `publicKey`, the derived one is stored
  - POST with junk / `null` / numeric / empty `privateKey` → 400 and no key created (no silent generation)
  - POST without `privateKey` generates; with missing, non-string, blank or 255-char name → 400, nothing
    generated; 254-char name accepted
  - PUT with only a name renames; PUT with junk / `null` / empty `privateKey` → 400 with a key-specific message,
    old key intact; PUT with a bad name → 400 "invalid name"; PUT unknown id → 404
  - non-object JSON body on POST and PUT → 400

➕ Regeneration step obsolete: the `webapi-spec` skill reads `openapi.yaml` directly (agreed with Claude).
The WebAPI module builds; YAML parsing and SSH request/response schema checks pass. A throwaway SQLite server
on port 14980 passed 45 HTTP checks, including all cases above and oversized names, using a short-lived bearer
token issued through NXCP (password login in this private-extensions build requires a separate WebAPI license).
The pre-change check reproduced private-key-only POST generating an RSA key instead of importing it.

### Task 4: Console dialog

**Touches:** UI (SWT and RWT share the dialog), SSH Keys view.

- `SSHKeyEditDialog`:
  - Import: Name, Private key, hint "Public key will be derived from the private key".
  - Edit: Name, read-only selectable "Current public key" (3–4 lines), "New private key" box, hint "Leave empty
    to keep the current key".
  - "Replace keys" checkbox removed. Name limit 254.
  - OK enabled only when the trimmed name is non-blank and, on import, the key box is non-empty; state set
    after the buttons exist and on every modify. No warning popup.
  - Mode (import vs. edit) is decided by the key's id (0 = import), not by whether a key object was passed —
    the retry passes the failed object back.
  - On every edit submission the private key is set explicitly: `setPrivateKey(null)` when the box is empty, so
    a value from a failed attempt is never resent.
- Saving stays in `SSHKeys.importKeys` / `editKey` (existing nxmc pattern: dialog collects, view job saves). The
  job catches `RCC_INVALID_ARGUMENT` itself (the generic job error handler would report it after the fact) and
  schedules, on the UI thread and only if the view is not disposed, a reopen of the dialog pre-filled with the
  entered name and key, the original id and current public key, plus the inline error "Invalid private key.
  Expected an unencrypted OpenSSH private key (convert with `ssh-keygen -p -N "" -f <file>`)". The retry works
  on the object it got back, never on the viewer's current selection. Any other error is reported as today.

- [x] verified with real desktop and web widgets (programmatically driven; coverage below):
  - import: OK disabled until name and key filled; valid ed25519 key imports and "Copy public key" yields the
    derived line
  - failed import reopens in import mode with the key still required; several invalid attempts followed by a
    valid one create exactly one key; cancelling the reopened dialog saves nothing
  - edit: current public key shown read-only; rename with empty key box keeps the key; pasting a new key
    replaces it and the public key changes; bad key → dialog reopens pre-filled, old key intact
  - failed edit → clear the key box → rename succeeds and sends no private key

➕ Both desktop and web Maven builds pass. Scratch checks drive real SWT widgets and real RWT widgets in an
isolated RAP application loaded by headless Chrome. They cover required input and the 254-unit limit, read-only
public text, retry mode and prefill, repeated failures followed by one save, cancellation, clearing a failed
replacement, and the disposed-view guard. Deterministic job checks inject RCC failures; a separate SWT run uses
the actual view jobs against a throwaway server and confirms two rejected imports followed by exactly one
Ed25519 key, failed replacement then clear/rename preserving the pair, and successful ECDSA replacement on the
same ID. The listed public key used by the existing clipboard action matches the derived line. The initial SWT
check failed on the old two-key import layout before implementation. Names retain the entered text as before.

## Post-Completion

- Admin guide (separate netxms-doc repository): SSH Keys import section — private key only, OpenSSH format,
  conversion command.
- Release notes: imported keys must be unencrypted OpenSSH format; state the confirmed agent libssh floor per key
  type.
- Pre-existing problems left out of scope, already in `docs/backlog/`: agent (ssh and netconf subagents) treats
  a successful key probe as login, agent key cache is never invalidated after replacement, per-ID update race.

## Tandem

| Task | Writer | Reviewer |
|------|--------|----------|
| 1 | claude | codex |
| 2 | claude | codex |
| 3 | codex | claude |
| 4 | codex | claude |
