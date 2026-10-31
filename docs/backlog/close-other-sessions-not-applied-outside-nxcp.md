---
worth: later
where: src/server/core/client.cpp:648
added: 2026-10-30
---
# "close other sessions" applies only to NXCP client logins

The flag takes effect only at `session.cpp:3107`. WebAPI login reads it into `LoginRequest.closeOtherSessions`
(`webapi_auth.cpp:197`) and `CompleteLogin` never uses it. Mobile login (`mdsession.cpp:335`, `:347`) and the
local admin interface (`admin.cpp:265`) discard it too. `CloseOtherSessions()` only walks `ClientSession`
instances, so an NXCP login can't close WebAPI tokens or mobile sessions either.

Two `/v1/login` calls leave both tokens usable and any console session connected, even with the flag set on
Everyone. Unknown that decides this: whether an admin enforcing the flag expects it to cover API and mobile
access, or only interactive consoles. Extending it needs a definition of "session" for WebAPI tokens, and
would break integrations that share an account.

Split from `close-other-sessions-no-global-enforcement`; surfaced in review of that change.
