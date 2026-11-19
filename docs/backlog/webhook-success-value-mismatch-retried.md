---
worth: later
where: src/server/ncdrivers/webhook/webhook.cpp
added: 2026-11-17
---
# Webhook driver retries a delivery the endpoint already accepted when SuccessValue does not match

With `SuccessJsonPath`/`SuccessValue` set, a response with a success HTTP code but a different value at the path returns a 10-second retry. The endpoint has already accepted the request, so each retry is a duplicate delivery, up to `NotificationChannels.MaxRetryCount` attempts. Docs now advise relying on `SuccessHttpCodes` for APIs whose bodies vary.

Design question that settles it: should a value mismatch after a success code be a failure (no retry) instead?

Found while documenting netxms/netxms-doc#84 in netxms-doc-antora.
