# Semgrep CI Automation

## Overview
- Add automated semgrep SAST scanning to GitHub Actions CI (issue #2785)
- Detect security vulnerabilities (OWASP, injection, buffer overflows) across C/C++, Java, Python
- Create exception suppression for vendored/generated code via `.semgrepignore`
- Complements existing cppcheck (code quality) with security-focused analysis

## Context
- **No existing semgrep config** in the repository
- **Existing CI workflows** (`.github/workflows/`): `build_and_test.yaml`, `cppcheck.yaml`, `lint-openapi.yaml`, `validate-templates.yaml`
- **Workflow conventions**: `ubuntu-latest`, `actions/checkout@v4`, concurrency groups with `cancel-in-progress`
- **Languages**: C/C++ (~1,200 files), Java (~2,200 files), Python (~900 files)
- **Vendored code to exclude**: `src/jansson/`, `src/sqlite/`, `src/libpng/`
- **Generated code to exclude**: lexer/parser outputs in `src/libnxsl/`, `src/snmp/nxmibc/`, `src/server/core/`; `messages.c`/`messages.h` files; autoconf output; generated SQL

## Development Approach
- **No tests** — this is CI configuration (YAML + ignore files), not application code
- 3 new files, no modifications to existing files
- Follow existing workflow conventions exactly

## Implementation Steps

### Task 1: Create `.semgrepignore`

**Files:**
- Create: `.semgrepignore`

- [x] Add vendored C libraries: `src/jansson/`, `src/sqlite/`, `src/libpng/`
- [x] Add generated lexer/parser files: `src/libnxsl/lex.parser.cpp`, `src/libnxsl/parser.tab.*`, `src/snmp/nxmibc/lex.parser.cpp`, `src/snmp/nxmibc/parser.tab.*`, `src/server/core/lex.nxmp_parser.cpp`, `src/server/core/nxmp_parser.tab.*`
- [x] Add generated message files: `**/messages.c`, `**/messages.h`, `**/MSG00001.bin`
- [x] Add build/autoconf artifacts: `target/`, `configure`, `config.h.in`, `aclocal.m4`, `m4/`, `autom4te.cache/`, `build/netxms-build-tag.*`
- [x] Add non-code directories: `doc/`, `docs/`, `manpages/`, `packages/`, `images/`, `share/`, `contrib/mibs/`, `sql/dbinit_*.sql`, `sql/dbschema_*.sql`
- [x] Add IDE/tool configs: `.idea/`, `.vscode/`, `.vs/`, `.claude/`, `.ralphex/`, `.serena/`
- [x] Add vendored JS and samples: `src/client/nxmc/java/src/rwt/resources/vncviewer/`, `src/client/nxshell/samples/`
- [x] Add generated OTLP: `src/server/otlp/generated/`

### Task 2: Create `.semgrep.yml`

**Files:**
- Create: `.semgrep.yml`

- [x] Create empty custom rules file with `rules: []`
- [x] Add commented examples for future NetXMS-specific rules (e.g., `malloc` vs `MemAlloc`, `NULL` vs `nullptr`)

### Task 3: Create `.github/workflows/semgrep.yaml`

**Files:**
- Create: `.github/workflows/semgrep.yaml`

- [x] Add workflow name and triggers: `schedule` (daily 04:00 UTC) + `pull_request`
- [x] Add concurrency group with `cancel-in-progress: true`
- [x] Add job: `ubuntu-latest`, `actions/checkout@v4` with `fetch-depth: 0`
- [x] Install semgrep via `pip install semgrep` (no Semgrep Cloud dependency)
- [x] Scheduled step: full scan with `--config .semgrep.yml --config p/security-audit --config p/owasp-top-ten --config p/c --config p/java --config p/python`
- [x] PR step: diff-aware scan with `--baseline-commit ${{ github.event.pull_request.base.sha }}` and `--error --severity ERROR` (blocks on high-severity only)
- [x] Add SARIF output: `--sarif --output semgrep-results.sarif`
- [x] Add SARIF upload step: `github/codeql-action/upload-sarif@v3`

### Task 4: Verify locally

- [x] Run `semgrep scan --config .semgrep.yml --config p/security-audit --config p/c --config p/java --config p/python --dry-run` to verify config
- [x] Verify `.semgrepignore` excludes vendored/generated files in scan output (1,136 files skipped)
- [x] Check scan completes in reasonable time (<15 min) — completed in 46 seconds

## Technical Details

**Semgrep rule registries:**
| Registry | Scope |
|----------|-------|
| `p/security-audit` | Cross-language security patterns |
| `p/owasp-top-ten` | OWASP vulnerability detection |
| `p/c` | C/C++ security (buffer overflows, format strings) |
| `p/java` | Java security and bugs |
| `p/python` | Python security and bugs |

**Inline suppression convention:**
- `// nosemgrep: <rule-id>` for C/C++ and Java
- `# nosemgrep: <rule-id>` for Python
- Always include specific rule-id, never bare `nosemgrep`

**SARIF upload** sends results to GitHub Security > Code Scanning tab, which provides triage UI for dismissing false positives (the "exception suppression" from the issue title).

## Post-Completion

**Manual verification:**
- Push to a PR branch and confirm workflow triggers on PR event
- Wait for daily schedule trigger and confirm full scan runs
- Check GitHub Security > Code Scanning tab shows uploaded results
- Review initial findings and add inline `nosemgrep` suppressions for confirmed false positives
