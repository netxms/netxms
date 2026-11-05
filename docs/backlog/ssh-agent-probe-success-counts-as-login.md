---
worth: yes
where: src/agent/subagents/ssh/session.cpp
added: 2026-11-05
---
# SSH session treats a successful public key probe as a completed login

`SSHSession::connect` sets `success = true` when `ssh_userauth_try_publickey` returns `SSH_AUTH_SUCCESS`,
then tries `ssh_pki_import_privkey_base64` + `ssh_userauth_publickey` without clearing the flag on failure.
The probe only means the server would accept that public key; libssh requires the follow-up
`ssh_userauth_publickey` to actually authenticate. With a stored public key that doesn't match the private
key (or an unloadable private key), the session reports connected, skips password fallback, and fails later
at channel open. Surfaced in the SSH key dialog redesign review (Codex). Fix: let the probe only gate the real
auth attempt, and set `success` from `ssh_userauth_publickey` alone.

The NETCONF subagent (`src/agent/subagents/netconf/session.cpp`) has the same probe-then-private-key flow and
needs the same fix.
