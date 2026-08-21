# Notification Channel Drivers (ncdrivers)

> Shared guidelines: See [root CLAUDE.md](../../CLAUDE.md) for C++ development guidelines.

## Overview

Notification channel drivers provide different methods for sending notifications from NetXMS:
- Email (SMTP)
- SMS (various gateways)
- Messaging platforms (Slack, Teams, Telegram, etc.)
- Custom channels (shell, database, etc.)

## Directory Structure

```
src/server/ncdrivers/
├── smtp/           # Email via SMTP
├── gsm/            # SMS via GSM modem
├── slack/          # Slack messaging
├── msteams/        # Microsoft Teams
├── telegram/       # Telegram bot
├── mattermost/     # Mattermost
├── googlechat/     # Google Chat
├── mqtt/           # MQTT publish
├── xmpp/           # XMPP/Jabber
├── twilio/         # Twilio SMS
├── nexmo/          # Nexmo SMS
├── smseagle/       # SMSEagle gateway
├── kannel/         # Kannel SMS gateway
├── websms/         # Web SMS gateway
├── anysms/         # Generic SMS via HTTP
├── webhook/        # Generic templated HTTP webhook
├── portech/        # Portech GSM gateway
├── mymobile/       # MyMobile SMS
├── text2reach/     # Text2Reach SMS
├── shell/          # Shell command execution
├── dbtable/        # Database table logging
├── textfile/       # Text file logging
├── snmptrap/       # SNMP trap sending (deprecated - use "snmptrap" event forwarder driver)
├── nxagent/        # Agent notification
└── dummy/          # Testing driver
```

## Driver Interface

All notification channel drivers implement the `NCDriver` interface defined in `src/server/include/ncdrv.h` (`NCDRV_API_VERSION` is checked at load time; a driver built against another version is rejected with a clear log message).

### Basic Structure

`send()` receives a `NotificationContext` (see `ncdrv.h`). Message text is provided both as UTF-8 (`recipient`, `subject`, `body`) and as wide string originals (`recipientW`, `subjectW`, `bodyW`); `body`/`bodyW` are always plain text, and when the message was submitted as markdown the original markdown is available in `markdownBody` (otherwise `nullptr`). Event context (`event`, `sourceObject`) is optional and may be `nullptr` — test sends, digest messages, and notifications sent with only an event code carry no event. Drivers that call `Event`/`NetObj` methods must link `libnxcore` (see `msteams` for an example). Persistent storage (`NCDriverStorageManager`) uses UTF-8 keys and values. Configuration (`Config*`) is wide-character.

```cpp
// mydriver.h
#include <ncdrv.h>

class MyNotificationDriver : public NCDriver
{
private:
   TCHAR m_server[256];
   TCHAR m_apiKey[256];

public:
   MyNotificationDriver(Config *config);
   virtual ~MyNotificationDriver();

   virtual int send(const NotificationContext& context) override;
};
```

```cpp
// mydriver.cpp
#include "mydriver.h"

MyNotificationDriver::MyNotificationDriver(Config *config) : NCDriver()
{
   // Read configuration
   _tcslcpy(m_server, config->getValue(_T("/Server"), _T("api.example.com")), 256);
   _tcslcpy(m_apiKey, config->getValue(_T("/ApiKey"), _T("")), 256);
}

int MyNotificationDriver::send(const NotificationContext& context)
{
   // context.recipient / context.subject / context.body are UTF-8; context.event may be nullptr
   // Return 0 on success, positive value to retry after that many seconds, negative value on permanent failure
   return 0;
}

// Entry point
DECLARE_NCD_ENTRY_POINT(MyNotificationDriver, &s_config)
{
   return new MyNotificationDriver(config);
}
```

### Driver Registration

```cpp
// At the end of driver source file
DECLARE_NCD_ENTRY_POINT(DriverClassName, configTemplate)
{
   return new DriverClassName(config);
}
```

## Example Implementations

### SMTP Driver

```cpp
int SMTPDriver::send(const NotificationContext& context)
{
   const char *recipient = context.recipient;
   const char *subject = context.subject;
   const char *body = context.body;

   SOCKET sock = ConnectToHost(m_server, m_port, m_timeout);
   if (sock == INVALID_SOCKET)
      return -1;

   // SMTP conversation
   SendCommand(sock, "EHLO %s\r\n", m_localHost);
   ReadResponse(sock);

   SendCommand(sock, "MAIL FROM:<%s>\r\n", m_from);
   ReadResponse(sock);

   SendCommand(sock, "RCPT TO:<%s>\r\n", recipient);
   ReadResponse(sock);

   SendCommand(sock, "DATA\r\n");
   ReadResponse(sock);

   // Send message content
   SendMessage(sock, subject, body);

   SendCommand(sock, "QUIT\r\n");
   closesocket(sock);

   return 0;
}
```

### Webhook-Based Driver (Slack, Teams, etc.)

```cpp
int SlackDriver::send(const NotificationContext& context)
{
   // Build JSON payload
   json_t *root = json_object();
   json_object_set_new(root, "channel", json_string(context.recipient));

   json_t *attachments = json_array();
   json_t *attachment = json_object();
   json_object_set_new(attachment, "title", json_string(context.subject));
   json_object_set_new(attachment, "text", json_string(context.body));
   json_array_append_new(attachments, attachment);
   json_object_set_new(root, "attachments", attachments);

   char *jsonStr = json_dumps(root, 0);

   // Send HTTP POST
   int result = PostToWebhook(m_webhookUrl, jsonStr);

   MemFree(jsonStr);
   json_decref(root);

   return result;
}
```

### Shell Driver

```cpp
int ShellDriver::send(const NotificationContext& context)
{
   // Build command line with escaped arguments
   char command[4096];
   snprintf(command, 4096, "%s %s %s %s",
            m_command,
            EscapeShellArg(context.recipient),
            EscapeShellArg(context.subject),
            EscapeShellArg(context.body));

   // Execute command
   return ExecuteCommand(command, nullptr, nullptr);
}
```

### Generic Webhook Driver

`webhook/` is a generic, templated HTTP driver (`ncdrv/webhook.ncd`). Instead of hard-coding a vendor's payload it sends a user-supplied body template as an HTTP request, so most REST/JSON notification gateways can be onboarded via config alone. Pure helpers (JSON-context placeholder substitution and integer-list parsing) live in `webhook_helpers.{cpp,h}` and are unit-tested by `tests/test-ncd-webhook/` (libcurl-free by design). URL percent-encoding uses `curl_easy_escape` and response JSON traversal uses `json_object_get_by_path_a` — both inline in `webhook.cpp`.

**Config keys:**

| Key | Type | Default | Notes |
|---|---|---|---|
| `URL` | string | (required) | Endpoint. Supports placeholders (URL-encoded). |
| `Method` | string | `POST` | `POST`, `PUT`, or `PATCH` (case-insensitive). |
| `ContentType` | string | `application/json` | Sent as `Content-Type` header. |
| `TemplateFile` | path | (required) | Body template; re-read on every send (edits apply without channel restart). Readability verified at init. |
| `VerifyPeer` | bool | `yes` | `CURLOPT_SSL_VERIFYPEER`. |
| `Timeout` | int (sec) | `10` | `CURLOPT_TIMEOUT`. |
| `SuccessHttpCodes` | csv | `200,201,202,204` | HTTP codes treated as success. |
| `RetryHttpCodes` | csv | `429,502,503,504` | HTTP codes producing a retry. |
| `SuccessJsonPath` | string | (empty) | Slash-separated path through nested response objects (`/data/status`). Empty = skip body check. |
| `SuccessValue` | string | (empty) | Required stringified value at `SuccessJsonPath`. |
| `[Headers]` section | name=value | none | Extra headers, literal values (no substitution). |

**Placeholders:** `${recipient}`, `${subject}`, `${body}` are substituted in the URL and template body. Unknown tokens are left literal so typos stay visible.

**Escaping rules:** in the URL, values are percent-encoded via `curl_easy_escape` (inline in `webhook.cpp`); in the template body, values are JSON-escaped via `EscapeStringForJSON()` (libcurl-free helper). Headers carry literal config values (no substitution).

**Success-detection model:**

| Condition | Return |
|---|---|
| HTTP in `SuccessHttpCodes` AND (no `SuccessJsonPath` OR path value == `SuccessValue`) | `0` (success) |
| HTTP in `SuccessHttpCodes`, `SuccessJsonPath` set, value mismatch | `10` (retry) |
| HTTP in `RetryHttpCodes` | `10` (retry) |
| Any other HTTP status / transport error / malformed response | `-1` (hard fail) |
| Missing `URL`/`TemplateFile`, unreadable template, bad `Method` | driver fails to load |

**Non-goals (v1):** header-value placeholder substitution; per-recipient URL aliasing; mtime/stat-based template-reload short-circuit; multi-endpoint per channel; GET/DELETE methods; HMAC/signing helpers; response-body capture beyond debug tracing. (Mirrored in the `webhook.cpp` header comment.)

## Configuration Template

Each driver declares which message parts it uses; the server drops messages without a recipient when `needRecipient` is set:

```cpp
static const NCConfigurationTemplate s_config(true, true);   // (needSubject, needRecipient)
```

## Configuration Access

```cpp
MyDriver::MyDriver(Config *config)
{
   // Simple values
   m_timeout = config->getValueAsInt(_T("/Timeout"), 30);

   // String values
   const TCHAR *server = config->getValue(_T("/Server"), _T("default"));
   _tcslcpy(m_server, server, MAX_PATH);

   // Boolean values
   m_useTLS = config->getValueAsBoolean(_T("/UseTLS"), false);

   // Nested values
   const TCHAR *user = config->getValue(_T("/Auth/Username"), _T(""));
}
```

## Return Codes

| Code | Meaning |
|------|---------|
| 0 | Success |
| > 0 | Temporary failure — server retries after that many seconds (up to `NotificationChannels.MaxRetryCount` attempts) |
| < 0 | Permanent failure — message dropped, `SYS_NOTIFICATION_FAILURE` generated |

Drivers may also override `checkHealth()` (default `true`); it is polled periodically and shown as channel health status.

## Adding a New Driver

A new NC driver must be registered in **all** of the following — easy to miss one and end up with a driver that compiles in isolation but doesn't appear in the build summary or `.ncd` install dir:

1. **Create directory and files** under `src/server/ncdrivers/mydriver/`:
   - `mydriver.cpp` — implementation
   - `Makefile.am` — only needs `EXTRA_DIST = mydriver.cpp` (the actual build rules live in the parent `src/server/ncdrivers/Makefile.am`)
   - `Makefile.w32` — Windows/MinGW build; copy from a similar driver and swap `DRIVER` + `DRV_LIBS` (see `webhook/Makefile.w32`, which pulls in `-lnxjansson $(CURL_LIBS)`)
2. **`src/server/ncdrivers/Makefile.am`**:
   - Add `mydriver.la` to `EXTRA_LTLIBRARIES`
   - Add per-driver block (sources, CPPFLAGS, LDFLAGS, LIBADD) — alphabetical placement. For jansson-using drivers wrap the jansson link inside `if USE_INTERNAL_JANSSON … else … endif` (see `matrix_la_*` or `slack_la_*` as templates).
3. **`configure.ac`**:
   - Line ~1097 (default `NCDRV_MODULES` list) — add driver name, alphabetical
   - Line ~2529 (libcurl-conditional `NCDRV_MODULES` list) — add driver name if it uses libcurl
   - AC_CONFIG_FILES block (~line 5181) — add `src/server/ncdrivers/mydriver/Makefile`
   - `NCDRV_LTLIBRARIES` (line 4890) is auto-derived from `NCDRV_MODULES` via perl — **no manual edit**
4. **`src/server/ncdrivers/Makefile.w32`** — add the driver name to the `DRIVERS` list (alphabetical) so the Windows/MinGW build recurses into it.
5. **`doc/internal/debug_tags.txt`** — add `ncd.mydriver` if you introduce a new debug tag (root CLAUDE.md requirement).
6. **Implement** the `NCDriver` interface and add `DECLARE_NCD_ENTRY_POINT` macro with a static `NCConfigurationTemplate s_config(needSubject, needRecipient)`.
7. Run `./init-source-tree && ./configure --with-server` and check that the driver appears in the build summary's NC driver list. `install-exec-hook` in `src/server/ncdrivers/Makefile.am` iterates `$(DRIVERS)` automatically — nothing to edit there.
8. **(Optional) Unit tests** — extract pure logic (no libcurl) into `mydriver_helpers.{cpp,h}` and add a `tests/test-ncd-mydriver/` target (see `webhook` for the established pattern):
   - `Makefile.am` modelled on `tests/test-libnetxms/Makefile.am` (`bin_PROGRAMS` + `@EXEC_LDFLAGS@`/`@EXEC_LIBS@`), compiling `../../src/server/ncdrivers/mydriver/mydriver_helpers.cpp`; `_CPPFLAGS` must include `-I../include` (for `<testtools.h>`); keep it libcurl-free
   - Wire via `TEST_MODULES` in **both** `configure.ac` blocks (~line 1092 and ~line 1468) and add `tests/test-ncd-mydriver/Makefile` to AC_CONFIG_FILES — **not** a `tests/Makefile.am` `SUBDIRS` edit
   - Register the binary in `tests/suite/netxms-test-suite.in` and `netxms-test-suite.cmd` so the documented test command runs it

## Build Configuration

`mydriver/Makefile.am` is intentionally minimal — actual build rules live in the parent `src/server/ncdrivers/Makefile.am`:

```makefile
EXTRA_DIST = mydriver.cpp
```

Parent `src/server/ncdrivers/Makefile.am` block (for a jansson-using driver):

```makefile
mydriver_la_SOURCES = mydriver/mydriver.cpp
mydriver_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
mydriver_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)'
mydriver_la_LIBADD = ../../libnetxms/libnetxms.la
if USE_INTERNAL_JANSSON
mydriver_la_LIBADD += @top_srcdir@/src/jansson/libnxjansson.la
else
mydriver_la_LIBADD += -ljansson
endif
```

## Debugging

```bash
# Debug tags
nxlog_debug_tag(_T("ncd.mydriver"), level, ...)    # Driver-specific
nxlog_debug_tag(_T("nc"), level, ...)              # Notification channel framework (server core)
```

## Testing

Use the dummy driver (`ncdrivers/dummy/`) as a template for testing new implementations.

```cpp
// Dummy driver logs all notifications to debug output (level 6)
int DummyDriver::send(const NotificationContext& context)
{
   nxlog_debug_tag(DEBUG_TAG, 6, _T("recipient=\"%hs\", subject=\"%hs\", text=\"%hs\""), context.recipient, context.subject, context.body);
   return 0;
}
```

## Built-in NXSL Driver

The NXSL notification channel driver (`NXSLDriver` in `src/server/core/notification_channel.cpp`) is a regular `NCDriver` implementation registered as built-in driver `NXSL` by `LoadNotificationChannelDrivers()`; it is compiled into the server core rather than loaded as a module because it needs `libnxsl` and server objects.

### Configuration
- Configuration is raw NXSL script text (not XML) — the driver is registered with `xmlConfiguration = false`, so the channel code passes the text unparsed as `/Configuration`
- Script has access to `NXSL_ServerEnv` with all server functions
- Compilation errors are logged with line number under tag `nc`; the channel then reports "Unable to create instance of driver NXSL"

### Script Variables
| Variable | Availability |
|----------|--------------|
| `$RECIPIENT` | Always |
| `$SUBJECT` | Always |
| `$MESSAGE` | Always |
| `$event` | When triggered from EPP action |
| `$object` / `$node` | When triggered from EPP with source object |
| `$dci` | When the triggering event is bound to a DCI (`$event.dciId != 0`) |

### Return Values
| Return | Result |
|--------|--------|
| `null` (no return) | Success |
| `true` | Success |
| `false` | Failure |
| integer > 0 | Retry in N seconds |

### NXSL Value Checking
- `NXSL_Value::isFalse()` returns `true` for both `false` AND `null`
- To distinguish explicit `false` from no return, check `isNull()` first

## Related Components

- [Server](../server/CLAUDE.md) - Server loads and uses notification drivers
- [libnetxms](../libnetxms/CLAUDE.md) - Core library (networking, JSON, etc.)
