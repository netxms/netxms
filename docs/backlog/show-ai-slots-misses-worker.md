---
worth: yes
where: src/server/core/ai_core.cpp:2895
added: 2026-11-17
---
# `show ai slots` does not list the `worker` slot

`ShowAISlots` prints a fixed list of known slots (`analytical`, `background`, `default`, `fast`,
`guard`, `interactive`). Workers started by `run-workers` use slot `worker` (`skills.cpp`,
`chat->setSlot("worker")`, since 2a49dc61c1), so a provider mapped to that slot is never shown and
an unmapped one is not reported as "not defined". Add `worker` to `knownSlots`.
