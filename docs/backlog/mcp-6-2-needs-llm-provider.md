---
worth: later
where: src/server/core/ai_core.cpp
added: 2026-11-19
---
# MCP on 6.2 still needs an LLM provider (#3892 not backported)

On `stable-6.2`, `InitAIAssistant()` returns early when no default provider is configured
("AI assistant initialization failed (no providers configured)"). That happens before script-defined tools,
`LoadAIDisabledLists()` and the core functions are registered. With `mcp.nxm` and no provider, the endpoint is
therefore missing the built-in and script tools, and functions and skills disabled by the admin are still
listed and callable. Master (`f0fa165743`) and `stable-7.0` (`ab49021a93`, first shipped in 7.0.3) fixed this
under #3892, but the MCP module itself was backported to 6.2 (`52add217b7`) without that fix.

Unknown that decides it: whether 6.2.x still gets non-security backports. If it gets this fix, also update
netxms-doc-antora `v6.2` (via `next-6.2`), `ai/ai-howto.adoc`, "How to Connect an MCP Client": the sentence
there that says the server needs a default LLM provider becomes "since 6.2.N, no provider needed".
