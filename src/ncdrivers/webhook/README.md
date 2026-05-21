# webhook — generic HTTP notification channel driver

Sends NetXMS notifications as a templated HTTP request (POST/PUT/PATCH) with a
user-supplied body. Most REST/JSON notification gateways can be onboarded by
configuration alone, without writing a vendor-specific driver.

## Files

- `webhook.cpp` — driver implementation (libcurl-based transport, config parsing, send loop)
- `webhook_helpers.{cpp,h}` — pure helpers (JSON-context placeholder substitution, integer-list parsing), libcurl-free so they can be unit-tested in isolation
- Unit tests: `tests/test-ncd-webhook/`

## Configuration keys

| Key | Type | Default | Notes |
|---|---|---|---|
| `URL` | string | (required) | Endpoint. Placeholders are percent-encoded. |
| `Method` | string | `POST` | `POST`, `PUT`, or `PATCH` (case-insensitive). |
| `ContentType` | string | `application/json` | Sent as `Content-Type` header. |
| `TemplateFile` | path | (required) | Body template; re-read on every send. |
| `VerifyPeer` | bool | `yes` | `CURLOPT_SSL_VERIFYPEER`. |
| `Timeout` | int (sec) | `10` | `CURLOPT_TIMEOUT`. |
| `SuccessHttpCodes` | csv | `200,201,202,204` | HTTP codes treated as success. |
| `RetryHttpCodes` | csv | `429,502,503,504` | HTTP codes producing a retry. |
| `SuccessJsonPath` | string | (empty) | Slash-separated path through nested response objects (e.g. `/data/status`). |
| `SuccessValue` | string | (empty) | Required stringified value at `SuccessJsonPath`. |
| `[headers]` section | name=value | none | Extra headers; literal values, no substitution. |

## Placeholders

`${recipient}`, `${subject}`, `${body}` are substituted in the URL
(percent-encoded via `curl_easy_escape`) and template body (JSON-escaped via
`EscapeStringForJSON()`). Unknown tokens are left literal so typos stay
visible. Headers are sent as literal config values — no substitution.

## Return codes

| Condition | Return |
|---|---|
| HTTP in `SuccessHttpCodes` AND (no `SuccessJsonPath` OR value matches `SuccessValue`) | `0` (success) |
| HTTP in `SuccessHttpCodes`, path set, value mismatch | `10` (retry) |
| HTTP in `RetryHttpCodes` | `10` (retry) |
| Other HTTP status / transport error / malformed response | `-1` (hard fail) |
| Missing `URL`/`TemplateFile`, unreadable template, bad `Method` | driver fails to load |

## Example

```ini
URL = https://api.example.com/v1/notify/${recipient}
Method = POST
ContentType = application/json
TemplateFile = /etc/netxms/webhook-example.json.tmpl
Timeout = 15
SuccessHttpCodes = 200,202
RetryHttpCodes = 429,502,503,504
SuccessJsonPath = /status
SuccessValue = ok

[headers]
Authorization = Bearer abc123
X-Source = netxms
```

`webhook-example.json.tmpl`:

```json
{
  "to": "${recipient}",
  "title": "${subject}",
  "text": "${body}"
}
```

## Non-goals (v1)

Header-value placeholder substitution; per-recipient URL aliasing; mtime-based
template-reload short-circuit; multi-endpoint per channel; GET/DELETE methods;
HMAC/signing helpers; response-body capture beyond debug tracing.

## Debugging

```
nxlog_debug_tag(_T("ncd.webhook"), level, ...)
```
