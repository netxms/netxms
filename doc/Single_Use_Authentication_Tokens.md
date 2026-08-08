---
title: "Single-Use Authentication Tokens"
author: [NetXMS Team]
date: "2026-08-02"
lang: "en"
...

# Single-Use Authentication Tokens

This document explains what a single-use authentication token is, why the NetXMS server treats it
differently from every other token it issues, and what an administrator should expect from one in
operation. It is not a procedure — the REST field and the NXCP flag that request such a token are
described in the WebAPI schema and the Java client API.

## Context

A NetXMS authentication token is a bearer credential: whoever holds the value can act as the user it
was issued for, until it expires or is revoked. The server issues four kinds:

- **Ephemeral** tokens live in server memory only. The management console requests one after every
  successful login so that it can reconnect without prompting for the password again.
- **Persistent** tokens are stored in the `auth_tokens` table and survive a server restart. These are
  the long-lived API credentials an administrator creates for an integration.
- **Service** tokens are issued internally, currently only by the reporting subsystem.
- **Single-use** tokens live in memory only and are destroyed by the login that spends them. They
  exist to hand a session from one process to another, and they are what the rest of this document is
  about.

The first three share one property: they can be presented as many times as the holder likes for as
long as they remain valid. For a credential that only ever travels inside a TLS session, that is
reasonable. It stops being reasonable the moment the value has to leave the process that received it.

The concrete case is `nxmc-launcher`. It logs in with the user's credentials, asks the server for a
short-lived token, closes its own connection, and starts the console as a separate process with the
token on the command line. That command line is readable by any local user running `ps`. The token
cannot be made instantaneous — the console needs time to start, so the value has to stay valid across
the launch window, which today means it stays *replayable* for that whole window too. Anybody who
looked at the process list during those ten minutes holds a working credential for the rest of them.

## What single-use changes

A single-use token is spent exactly once, and the act of spending destroys it.

The interesting part is not that the token is deleted — it is *where* it can be deleted. The server
has several places that accept a token: an NXCP login, a REST request carrying a bearer header, and
the anonymous object access endpoint. Only one of them, the NXCP login, is allowed to spend a
single-use token. Everywhere else, presenting one is simply an authentication failure: the REST layer
answers 401 and the token is left untouched, still spendable by the login it was actually meant for.

That asymmetry is the whole design. It means a stolen value cannot be burned through some unrelated
API call, and it cannot be used to do anything the legitimate client was not already going to do.
What is left is a race to the login, and the server does not take sides in it: whichever login
claims the token first gets it, and the other one is refused. That is still the outcome we want,
because the race is both bounded and visible — if a stolen value wins it, the console's own login
fails where the user can see it, instead of the token staying quietly replayable for the rest of its
lifetime. The exposure window closes on consumption instead of on expiry, which is why the token's
TTL can stay comfortably long without weakening anything.

Because destruction is the point, the server is deliberately unforgiving about it. Once a login has
claimed the token, the token is gone even if that login then fails — a disabled account, an intruder
lockout, anything. Letting a failed attempt hand the credential back would turn a one-shot token into
a retry oracle, which is precisely the property we were trying to remove. The trade-off is real: a
user whose account is locked out will need a fresh token, not a second try with the old one. We
consider that the correct side to err on for a credential whose entire purpose is to be unusable
twice.

The same rule applies to a login that is not finished yet. Consumption happens during authentication,
before two-factor authentication is evaluated, so a user with 2FA configured spends the token and is
then still asked for a second factor. Abandoning or failing that challenge does not give the token
back — the client must request a new one. The alternative, holding the token until the login
completes, would keep a spendable credential alive for the whole duration of a challenge the attacker
could be racing, which is the exposure the single-use property exists to close.

Two smaller behaviours follow from treating these tokens as handoffs rather than as fresh logins:

- A login that spends a single-use token does **not** close the user's other sessions, even if the
  account has `UF_CLOSE_OTHER_SESSIONS` set. A handoff is the same person opening one more client,
  not a new login that should displace what is already running. In the launcher flow this is
  defensive more than load-bearing — the launcher's own credential login has already done any closing
  by then — but it matters when the issuer is something that stays connected, such as a WebAPI
  integration.
- Issuance is always written to the audit log, including when a user issues one for themselves. The
  server normally skips that audit entry for self-issued ephemeral tokens because the console
  requests them constantly and the noise would drown the log. A single-use token is a deliberate
  handoff, not background chatter, so it is always recorded.

## Memory only, and what that costs

Single-use tokens exist in server memory and nowhere else. They are never written to `auth_tokens`,
and asking for one that is both persistent and single-use is rejected outright — over NXCP with
`RCC_INVALID_ARGUMENT`, over REST with HTTP 400. Note that the REST `persistent` field defaults to
**true**, so a request body containing only `singleUse` is rejected as well; it has to say
`"persistent": false` explicitly. A zero validity time is rejected on both surfaces too — such a
token expires at the instant it is issued. That rule is not specific to single-use tokens: it now
applies to every type, and a request that previously returned an already-expired token fails with
`RCC_INVALID_ARGUMENT` or HTTP 400 instead. A single-use token in particular is worth nothing unless
it stays valid across the handoff window.

The practical consequence for an administrator is that **a single-use token does not survive a server
restart**. It also does not exist on any other node of an HA pair. If the server goes down between
issuance and the login that was supposed to spend it, the token is simply gone and a new one must be
requested. For a credential that lives for seconds or minutes on its way from one process to another,
that is not much of a loss.

We could have stored them, but the cost is out of proportion to the benefit: a new column on
`auth_tokens`, an `nxdbmgr` upgrade procedure, and propagation of that schema change across every
supported release branch — all to persist something designed to be consumed within the next minute.
Unclaimed tokens are cleaned up by the same expiration sweep that handles ordinary ephemeral tokens,
so nothing special is needed to reclaim the memory either.

## Observing them

In the server console, `show authtokens` lists live tokens with a `Type` column naming the type:
`ephemeral`, `persistent`, `service` or `single-use`. A single-use token disappears from the listing
the moment it is consumed — which is the most direct way to confirm a handoff actually happened.
The `Token` column shows a masked form of the value (`abcd****wxyz`), enough to match a listing entry
against a token someone is holding without disclosing it; it reads `unavailable` for persistent
tokens read back from the database, whose value this server instance never saw.

The clear-text value itself is returned exactly once, by the response to the request that issued the
token, and is dropped from the descriptor as soon as that response is built. Token listings —
`GET /v1/users/{id}/tokens` over REST and `CMD_GET_AUTH_TOKENS` over NXCP — carry only the token
attributes, so `MANAGE_USERS` grants the ability to issue, list and revoke another user's tokens, not
to read the secret of one already issued. A lost token is revoked and re-issued; there is no recovery
path.

Everything else the feature does is logged under the existing `auth` debug tag. Level 4 covers the
rejections an administrator would want to see: a single-use token presented somewhere other than the
login entry point, and an attempt to reuse one that has already been claimed. Level 5 records the
successful consumption.

## Where this fits

Single-use is a token *type*, not a property layered on top of one. `AuthenticationTokenType` has
four values — `EPHEMERAL`, `PERSISTENT`, `SERVICE` and `SINGLE_USE` — and they are mutually
exclusive. One field answers both "where does this token live and how does the login behave" and
"how many times can it be spent".

An earlier iteration modelled single-use as a boolean orthogonal to the type. That advertised a
freedom of combination which did not exist: persistent + single-use was silently corrected back to
persistent by the descriptor constructor, and service + single-use was a combination nothing ever
asked for. Making the invalid pairings unrepresentable is the point of the current model. The
composition with `SERVICE` is deliberately gone; if a service-like handoff is ever needed it belongs
on `SINGLE_USE` as behaviour, not as a resurrected flag.

The type is a server-side concept. Neither protocol carries it: NXCP and REST both represent a token
as a set of booleans, which the server derives from the type when it serializes a descriptor and maps
back to a type when it accepts an issuance request.

There is deliberately no REST equivalent of the login spend point — a single-use token cannot be
exchanged over HTTP for a session token. Such a route would give REST clients the same handoff
capability and would be defensible over TLS, but it adds an authentication-critical endpoint that
nothing needs yet. It is worth revisiting if a REST-only integration ever wants the handoff.

Finally, note that the launcher does not yet ask for single-use tokens. `nxmc-launcher` lives in its
own repository and has to opt in; until it does, its handoff token behaves exactly as it always has.
The request field is a boolean that defaults to false everywhere, so no existing client changes
behaviour by upgrading the server.

## Further reading

- WebAPI schema: `singleUse` on `AuthenticationTokenCreateInput` and `AuthenticationToken` in
  `src/server/webapi/openapi.yaml`
- Java client: `NXCSession.requestAuthenticationToken()` and `AuthenticationToken.isSingleUse()`
- Debug tags: `doc/internal/debug_tags.txt`
