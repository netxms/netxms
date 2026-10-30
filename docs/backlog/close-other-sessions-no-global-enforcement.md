---
worth: yes
where: src/server/core/userdb.cpp:633
added: 2026-10-30
---
# "close other sessions after login" cannot be enforced globally

`UF_CLOSE_OTHER_SESSIONS` is a per-user flag and `AuthenticateUser` checks only the user's own flags. There
is no server config variable and no group lookup, unlike `UF_2FA_ENFORCE` (`userdb.cpp:2105`). The server
also refuses to store the flag on the "Everyone" group (`userdb_objects.cpp:303`).

The only workaround is setting the flag per user, by hand in the console or with
`PUT /v1/users/<id>` and `{"flags": {"closeOtherSessions": true}}`. It has to be repeated, because new users
and users created by LDAP sync (`userdb.cpp:1303`) don't get the flag.

Design still to pick: a server config variable or a group flag modelled on `UF_2FA_ENFORCE`.

Decide the scope at the same time. `CloseOtherSessions()` has one caller, the client login path
(`session.cpp:3110`). WebAPI login reads the flag into `LoginRequest.closeOtherSessions`
(`webapi_auth.cpp:197`) and never uses it, and mobile login (`mdsession.cpp`) ignores it too. Service and
single-use token logins skip it on purpose (`session.cpp:2661`) and should keep doing so.
