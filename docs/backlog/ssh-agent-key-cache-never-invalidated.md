---
worth: yes
where: src/agent/subagents/ssh/keys.cpp
added: 2026-11-05
---
# Agent SSH key cache is never invalidated after a key is replaced on the server

`GetSshKey` caches `KeyPair` in `s_keyList` keyed by server ID + key ID and nothing ever removes or refreshes
an entry. Replacing a key's private key on the server (SSH Keys view → edit) is not seen by an agent that
already used that key until the agent restarts, so key rotation silently keeps using the old key. Needs either
a server→agent invalidation (e.g. on `NX_NOTIFY_SSH_KEY_DATA_CHAGED`-equivalent push) or a TTL on cache
entries. Surfaced in the SSH key dialog redesign review.

The NETCONF subagent (`src/agent/subagents/netconf/keys.cpp`) keeps an identical never-invalidated `s_keyList`
cache; fix both together.
