---
worth: yes
where: .github/workflows/semgrep.yaml
added: 2026-11-09
---
# semgrep workflow pinned to Python 3.14 until semgrep supports 3.15

`setup-python` with `'3.x'` moved to Python 3.15 on 2026-10-28, and semgrep (wheels tagged up to
cp314/py314, still installable on 3.15) then crashes with `semgrep-core exited with -11!` in every
nightly and PR scan. Reproduced in Linux amd64 docker: semgrep 1.184.0 under `python:3.15` crashes,
under `python:3.14` the same scan completes. The workflow is pinned to `'3.14'`; go back to `'3.x'`
once a semgrep release lists Python 3.15 support and a scan under 3.15 completes.
