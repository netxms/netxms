---
worth: yes
where: src/server/core/session.cpp
added: 2026-11-05
---
# NXCP SSH key import, update and generate write no audit record on success

`ClientSession::updateSshKey` and `ClientSession::generateSshKey` call `writeAuditLog` only on access denied.
The NXCP delete handler (`ClientSession::deleteSshKey`) and the WebAPI handlers in `src/server/webapi/sshkeys.cpp`
audit success as `AUDIT_SYSCFG` "SSH key [%u] created/updated". A private key imported or replaced from the
console, which changes the credentials agents log in with, therefore leaves no audit trail. Pre-existing;
surfaced by the revmux review of the SSH key import redesign branch. Fixing it needs the new key ID on the NXCP
path, which `CreateOrEditSshKey(const NXCPMessage&)` does not return today.
