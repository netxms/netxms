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
src/ncdrivers/
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
├── portech/        # Portech GSM gateway
├── mymobile/       # MyMobile SMS
├── text2reach/     # Text2Reach SMS
├── shell/          # Shell command execution
├── dbtable/        # Database table logging
├── textfile/       # Text file logging
├── snmptrap/       # SNMP trap sending
├── nxagent/        # Agent notification
└── dummy/          # Testing driver
```

## Driver Interface

All notification channel drivers implement the `NCDriver` interface defined in `src/server/include/nms_core.h`.

### Basic Structure

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

   virtual int send(const TCHAR *recipient, const TCHAR *subject,
                    const TCHAR *body) override;
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

int MyNotificationDriver::send(const TCHAR *recipient, const TCHAR *subject,
                                const TCHAR *body)
{
   // Implement notification sending
   // Return 0 on success, error code on failure
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
int SMTPDriver::send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body)
{
   SOCKET sock = ConnectToHost(m_server, m_port, m_timeout);
   if (sock == INVALID_SOCKET)
      return -1;

   // SMTP conversation
   SendCommand(sock, _T("EHLO %s\r\n"), m_localHost);
   ReadResponse(sock);

   SendCommand(sock, _T("MAIL FROM:<%s>\r\n"), m_from);
   ReadResponse(sock);

   SendCommand(sock, _T("RCPT TO:<%s>\r\n"), recipient);
   ReadResponse(sock);

   SendCommand(sock, _T("DATA\r\n"));
   ReadResponse(sock);

   // Send message content
   SendMessage(sock, subject, body);

   SendCommand(sock, _T("QUIT\r\n"));
   closesocket(sock);

   return 0;
}
```

### Webhook-Based Driver (Slack, Teams, etc.)

```cpp
int SlackDriver::send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body)
{
   // Build JSON payload
   json_t *root = json_object();
   json_object_set_new(root, "channel", json_string_t(recipient));

   json_t *attachments = json_array();
   json_t *attachment = json_object();
   json_object_set_new(attachment, "title", json_string_t(subject));
   json_object_set_new(attachment, "text", json_string_t(body));
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
int ShellDriver::send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body)
{
   // Build command line with escaped arguments
   TCHAR command[4096];
   _sntprintf(command, 4096, _T("%s %s %s %s"),
              m_command,
              EscapeShellArg(recipient),
              EscapeShellArg(subject),
              EscapeShellArg(body));

   // Execute command
   return ExecuteCommand(command, nullptr, nullptr);
}
```

## Configuration Template

Each driver defines a configuration template:

```cpp
static NCConfigurationTemplate s_config =
{
   _T("MyDriver"),                          // Driver name
   _T("<config>\n")
   _T("  <server>api.example.com</server>\n")
   _T("  <apiKey></apiKey>\n")
   _T("  <timeout>30</timeout>\n")
   _T("</config>\n"),                       // Default config
   true                                     // Supports recipient customization
};
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
| -1 | Connection failed |
| -2 | Authentication failed |
| -3 | Send failed |
| -4 | Timeout |
| -5 | Configuration error |

## Adding a New Driver

1. Create directory: `src/ncdrivers/mydriver/`
2. Create files:
   - `mydriver.cpp` - Implementation
   - `Makefile.am` - Build configuration
3. Update `src/ncdrivers/Makefile.am` to include new driver
4. Implement the `NCDriver` interface
5. Add `DECLARE_NCD_ENTRY_POINT` macro

## Build Configuration

In `mydriver/Makefile.am`:

```makefile
pkglib_LTLIBRARIES = ncd_mydriver.la

ncd_mydriver_la_SOURCES = mydriver.cpp
ncd_mydriver_la_LDFLAGS = -module -avoid-version
ncd_mydriver_la_LIBADD = ../../../libnetxms/libnetxms.la
```

## Debugging

```bash
# Debug tags
nxlog_debug_tag(_T("ncd.mydriver"), level, ...)    # Driver-specific
nxlog_debug_tag(_T("notification"), level, ...)    # Notification system
```

## Testing

Use the dummy driver (`ncdrivers/dummy/`) as a template for testing new implementations.

```cpp
// Dummy driver logs all notifications to debug output
int DummyDriver::send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body)
{
   nxlog_debug(1, _T("DummyDriver: to=%s subject=%s"), recipient, subject);
   return 0;
}
```

## Related Components

- [Server](../server/CLAUDE.md) - Server loads and uses notification drivers
- [libnetxms](../libnetxms/CLAUDE.md) - Core library (networking, JSON, etc.)
