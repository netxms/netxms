# Notification Channel Drivers (ncdrivers)

## Driver Interface

All notification channel drivers implement the `NCDriver` interface defined in `src/server/include/ncdrv.h` (`NCDRV_API_VERSION` is checked at load time; a driver built against another version is rejected with a clear log message).

`send()` receives a `NotificationContext` (see `ncdrv.h`). Message text is provided both as UTF-8 (`recipient`, `subject`, `body`) and as wide string originals (`recipientW`, `subjectW`, `bodyW`); `body`/`bodyW` are always plain text, and when the message was submitted as markdown the original markdown is available in `markdownBody` (otherwise `nullptr`). Event context (`event`, `sourceObject`) is optional and may be `nullptr` — test sends, digest messages, and notifications sent with only an event code carry no event. `location` carries a geolocation to send after the text (from the action's "send source object's geolocation" option or the NXSL `SendNotification()` location argument); it is set only when `getType() != GL_UNSET`, and drivers without location messages (everything except Telegram today) simply ignore it. Drivers that call `Event`/`NetObj` methods must link `libnxcore` (see `msteams` for an example). Persistent storage (`NCDriverStorageManager`) uses UTF-8 keys and values. Configuration (`Config*`) is wide-character.

Each driver declares which message parts it uses; the server drops messages without a recipient when `needRecipient` is set:

```cpp
static const NCConfigurationTemplate s_config(true, true);   // (needSubject, needRecipient)
DECLARE_NCD_ENTRY_POINT(MyDriver, &s_config)
{
   return new MyDriver(config);
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
3. **`configure.ac`** (grep for an existing driver such as `webhook` to find each spot):
   - `NCDRV_MODULES` in the `--with-dist` block — add driver name, alphabetical
   - `NCDRV_MODULES` in the `HAVE_LIBCURL` block — add driver name if it uses libcurl
   - `AC_CONFIG_FILES` — add `src/server/ncdrivers/mydriver/Makefile`
   - `NCDRV_LTLIBRARIES` is auto-derived from `NCDRV_MODULES` via perl — **no manual edit**
4. **`src/server/ncdrivers/Makefile.w32`** — add the driver name to the `DRIVERS` list (alphabetical) so the Windows/MinGW build recurses into it.
5. **`doc/internal/debug_tags.txt`** — add `ncd.mydriver` if you introduce a new debug tag (root CLAUDE.md requirement).
6. **Implement** the `NCDriver` interface and add `DECLARE_NCD_ENTRY_POINT` macro with a static `NCConfigurationTemplate s_config(needSubject, needRecipient)`.
7. Run `./init-source-tree && ./configure --with-server` and check that the driver appears in the build summary's NC driver list. `install-exec-hook` in `src/server/ncdrivers/Makefile.am` iterates `$(DRIVERS)` automatically — nothing to edit there.
8. **(Optional) Unit tests** — extract pure logic (no libcurl) into `mydriver_helpers.{cpp,h}` and add a `tests/test-ncd-mydriver/` target (see `webhook` for the established pattern):
   - `Makefile.am` modelled on `tests/test-libnetxms/Makefile.am` (`bin_PROGRAMS` + `@EXEC_LDFLAGS@`/`@EXEC_LIBS@`), compiling `../../src/server/ncdrivers/mydriver/mydriver_helpers.cpp`; `_CPPFLAGS` must include `-I../include` (for `<testtools.h>`); keep it libcurl-free
   - Wire via every `TEST_MODULES` assignment in `configure.ac` that lists `test-ncd-webhook`, and add `tests/test-ncd-mydriver/Makefile` to AC_CONFIG_FILES — **not** a `tests/Makefile.am` `SUBDIRS` edit
   - Register the binary in `tests/suite/netxms-test-suite.in` and `netxms-test-suite.cmd` so the documented test command runs it

Debug tags: `ncd.<driver>` for driver code, `nc` for the notification channel framework in server core.

## Generic Webhook Driver

`webhook/` is a generic, templated HTTP driver: it sends a user-supplied body template, so most REST/JSON gateways can be onboarded via config alone. Keep `webhook_helpers.{cpp,h}` libcurl-free — they are compiled into `tests/test-ncd-webhook/`. Its non-goals are listed in the `webhook.cpp` header comment.

## Built-in NXSL Driver

The NXSL driver (`NXSLDriver` in `src/server/core/notification_channel.cpp`) is a regular `NCDriver` registered as built-in driver `NXSL` by `LoadNotificationChannelDrivers()`; it lives in server core rather than a module because it needs `libnxsl` and server objects. It is registered with `xmlConfiguration = false`, so its configuration is raw NXSL script text passed unparsed as `/Configuration`.
