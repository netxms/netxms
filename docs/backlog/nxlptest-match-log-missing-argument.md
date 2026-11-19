---
worth: yes
where: src/tools/nxlptest/nxlptest.cpp:166
added: 2026-11-19
---
# nxlptest match message has four placeholders but three arguments

The match callback logs `"Parser match (eventCode=%u eventName=%s eventTag=%s) \"%s\""` with only
`data.eventCode, data.eventName, data.eventTag`; the final `%s` (presumably the matched record text) reads a
missing vararg, which is undefined behaviour every time a line matches at debug level 3 or higher. That is
exactly the level the tool's users need to see matches. Add the record text argument.
