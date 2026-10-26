---
worth: yes
where: src/server/jira/jira.cpp:107
added: 2026-10-26
---
# Jira.VerifyPeer ignored on first connection

`JiraLink::connect()` sets `CURLOPT_SSL_VERIFYPEER` from `m_verifyPeer` before calling `setCredentials()`,
which is where `Jira.VerifyPeer` is actually read. On the first connection after server startup
`m_verifyPeer` still holds the constructor's `false`, so the TLS certificate of the Jira server is not
verified, and the Jira credentials are sent over that unverified connection. Later reconnects use the value
read by the previous `setCredentials()` call, and `checkConnection()` re-reads the variable without applying
it to the existing handle, so changing `Jira.VerifyPeer` has no effect until the next reconnect.

Fix: read the variable before setting the option (or set `CURLOPT_SSL_VERIFYPEER` inside
`setCredentials()`). Same code in stable-6.2 (release-6.2.6). Found while verifying the helpdesk page for
the v6.2 docs.
