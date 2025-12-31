# SSH Interactive Sessions Design

## Overview

Extend the SSH subagent with PTY/shell support to enable interactive CLI sessions with network devices. The agent acts as a pure proxy forwarding raw terminal I/O, while the server handles all parsing (prompt detection, pagination, ANSI stripping).

**Parent design:** `doc/AI_SSH_Integration_Design.md`

## Context

This is Phase 2 of the AI SSH Integration project. It can be developed in parallel with Phase 1 (Agent-User Sync Interaction).

### Current State

The existing SSH subagent (`src/agent/subagents/ssh/`) uses the **exec channel** model:

```cpp
// Current implementation in session.cpp
ssh_channel_open_session(channel);
ssh_channel_request_exec(channel, command);  // Execute single command
// Read output until channel closes
```

This works for Linux/Unix servers but **does not work** for most network devices.

### The Problem

Network devices (Cisco IOS, Juniper, etc.) typically:
- Don't support the exec channel type
- Require interactive PTY (pseudo-terminal)
- Need shell mode with prompt detection
- Require enable/privilege escalation within session
- Have pagination ("--More--") that needs handling
- Don't return until prompt appears

## Architecture

### Design Principles

1. **Agent as pure proxy** - SSH subagent forwards raw bytes, no parsing
2. **Server-side intelligence** - All prompt detection, pagination handling, ANSI stripping on server
3. **Single session class** - Extend existing `SSHSession` with interactive channel support (no inheritance needed)
4. **TCP proxy pattern** - Use `CMD_SSH_CHANNEL_DATA` similar to existing `CMD_TCP_PROXY_DATA`

### Component Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│  SERVER (src/server/libnxsrv/)                                          │
│  ┌───────────────────────────────────────────────────────────────────┐  │
│  │  SSHInteractiveChannel                                            │  │
│  │  - Prompt detection (line-based + timeout)                        │  │
│  │  - Pagination handling (upfront disable + runtime fallback)       │  │
│  │  - ANSI stripping (always)                                        │  │
│  │  - Command echo stripping (detect & strip)                        │  │
│  │  - Uses driver hints + node config override                       │  │
│  └───────────────────────────────────────────────────────────────────┘  │
│                           │                                             │
│                           │ via AgentConnection                         │
│  ┌───────────────────────────────────────────────────────────────────┐  │
│  │  AgentConnection (extended)                                       │  │
│  │  - openSSHChannel(), sendSSHChannelData(), closeSSHChannel()      │  │
│  │  - Callback for incoming CMD_SSH_CHANNEL_DATA                     │  │
│  └───────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ NXCP (CMD_SSH_CHANNEL_DATA)
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  AGENT - SSH Subagent (src/agent/subagents/ssh/)                        │
│  ┌───────────────────────────────────────────────────────────────────┐  │
│  │  SSHChannelProxy                                                  │  │
│  │  - Dedicated reader thread per channel                            │  │
│  │  - Forwards raw bytes: server ↔ SSH PTY channel                   │  │
│  │  - No parsing (pure proxy)                                        │  │
│  └───────────────────────────────────────────────────────────────────┘  │
│                           │                                             │
│  ┌───────────────────────────────────────────────────────────────────┐  │
│  │  SSHSession (extended)                                            │  │
│  │  - Existing exec methods (unchanged)                              │  │
│  │  - New: openInteractiveChannel() - creates PTY + shell            │  │
│  └───────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ SSH PTY channel (libssh)
                                    ▼
                          ┌─────────────────┐
                          │  Network Device │
                          └─────────────────┘
```

### SSH Protocol Note

SSH supports multiple channels within a single session (RFC 4254). This means:
- One `ssh_session` can have both exec and interactive channels
- Channels are multiplexed over a single encrypted connection
- Session pool serves both exec and interactive requests

## NXCP Protocol

### New Commands

Following the TCP proxy pattern (`CMD_TCP_PROXY_DATA`):

```cpp
// New command codes (in include/nms_cscp.h)
#define CMD_OPEN_SSH_CHANNEL     0x0180   // Open interactive SSH channel
#define CMD_SSH_CHANNEL_DATA     0x0181   // Bidirectional raw data
#define CMD_CLOSE_SSH_CHANNEL    0x0182   // Close channel
```

### CMD_OPEN_SSH_CHANNEL

Request from server to agent to open interactive SSH channel.

```cpp
// Request fields
VID_IP_ADDRESS        // Target device IP
VID_PORT              // Target SSH port (default 22)
VID_USER_NAME         // SSH username
VID_PASSWORD          // SSH password (encrypted)
VID_SSH_KEY_ID        // SSH key ID (alternative to password)
VID_CHANNEL_ID        // Channel ID assigned by server

// Response fields
VID_RCC               // Result code
VID_CHANNEL_ID        // Confirmed channel ID
```

### CMD_SSH_CHANNEL_DATA

Bidirectional raw terminal data. Uses message ID for channel identification (same pattern as TCP proxy).

```cpp
// Message structure (both directions)
header->code = CMD_SSH_CHANNEL_DATA
header->id = channelId        // Channel ID in message ID field
header->flags = MF_BINARY
header->numFields = payloadSize
// Binary payload follows header
```

### CMD_CLOSE_SSH_CHANNEL

Close the interactive channel.

```cpp
// Request fields
VID_CHANNEL_ID        // Channel to close
VID_ERROR_INDICATOR   // true if closing due to error
```

## Agent Implementation

### SSHSession Extension

Extend existing `SSHSession` class in `src/agent/subagents/ssh/session.cpp`:

```cpp
class SSHSession
{
private:
   // Existing members...
   ssh_session m_session;

public:
   // Existing exec methods (unchanged)
   StringList* execute(const TCHAR *command);
   bool execute(const TCHAR *command, const shared_ptr<ActionExecutionContext>& context);

   // New interactive channel support
   ssh_channel openInteractiveChannel();
};
```

### SSHChannelProxy Class

Class in `src/agent/subagents/ssh/channel_proxy.cpp`:

```cpp
/**
 * SSH channel proxy - forwards raw data between server and SSH PTY channel
 */
class SSHChannelProxy
{
private:
   uint32_t m_channelId;
   ssh_channel m_sshChannel;
   AbstractCommSession *m_commSession;
   THREAD m_readerThread;
   bool m_running;

   static void readerThreadStarter(SSHChannelProxy *proxy);
   void readerThreadInternal();

public:
   SSHChannelProxy(uint32_t channelId, ssh_channel channel, AbstractCommSession *session);
   ~SSHChannelProxy();

   void start();      // Start reader thread
   void stop();       // Stop reader thread

   uint32_t getChannelId() const { return m_channelId; }

   // Called when CMD_SSH_CHANNEL_DATA received from server
   void writeToChannel(const BYTE *data, size_t size);
};
```

### Reader Thread

Dedicated thread per channel for low-latency forwarding:

```cpp
void SSHChannelProxy::readerThreadInternal()
{
   BYTE buffer[65536];

   while(m_running)
   {
      // Read with 100ms timeout for responsive shutdown
      int nbytes = ssh_channel_read_timeout(m_sshChannel,
                                            buffer + NXCP_HEADER_SIZE,
                                            sizeof(buffer) - NXCP_HEADER_SIZE,
                                            0, 100);
      if (nbytes > 0)
      {
         // Build and send NXCP message (same pattern as TcpProxy)
         NXCP_MESSAGE *header = reinterpret_cast<NXCP_MESSAGE*>(buffer);
         header->code = htons(CMD_SSH_CHANNEL_DATA);
         header->id = htonl(m_channelId);
         header->flags = htons(MF_BINARY);
         header->numFields = htonl(static_cast<uint32_t>(nbytes));

         uint32_t size = NXCP_HEADER_SIZE + nbytes;
         if ((size % 8) != 0)
            size += 8 - size % 8;
         header->size = htonl(size);

         m_commSession->postRawMessage(header);
      }
      else if (nbytes < 0)
      {
         // Error or channel closed
         if (!ssh_channel_is_eof(m_sshChannel))
         {
            nxlog_debug_tag(DEBUG_TAG, 6, _T("SSH channel proxy %u read error"), m_channelId);
         }
         break;
      }
      // nbytes == 0 means timeout, continue loop
   }

   // Notify server of channel closure
   if (m_running)
   {
      NXCPMessage msg(CMD_CLOSE_SSH_CHANNEL, 0, m_commSession->getProtocolVersion());
      msg.setField(VID_CHANNEL_ID, m_channelId);
      msg.setField(VID_ERROR_INDICATOR, !ssh_channel_is_eof(m_sshChannel));
      m_commSession->postMessage(&msg);
   }
}
```

### PTY/Shell Setup

```cpp
ssh_channel SSHSession::openInteractiveChannel()
{
   if (!isConnected())
      return nullptr;

   ssh_channel channel = ssh_channel_new(m_session);
   if (channel == nullptr)
      return nullptr;

   if (ssh_channel_open_session(channel) != SSH_OK)
   {
      ssh_channel_free(channel);
      return nullptr;
   }

   // Request PTY
   if (ssh_channel_request_pty(channel) != SSH_OK)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Failed to request PTY: %hs"),
                      ssh_get_error(m_session));
      ssh_channel_close(channel);
      ssh_channel_free(channel);
      return nullptr;
   }

   // Set terminal size
   ssh_channel_change_pty_size(channel, 200, 24);

   // Start shell
   if (ssh_channel_request_shell(channel) != SSH_OK)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Failed to start shell: %hs"),
                      ssh_get_error(m_session));
      ssh_channel_close(channel);
      ssh_channel_free(channel);
      return nullptr;
   }

   return channel;
}
```

## Server Implementation

### SSHDriverHints Structure

Defined in `src/server/include/nxsrvapi.h`:

```cpp
/**
 * SSH driver hints for interactive CLI sessions
 */
struct SSHDriverHints
{
   const char *promptPattern;           // Regex for command prompt
   const char *enabledPromptPattern;    // Regex for privileged prompt
   const char *enableCommand;           // "enable" or equivalent
   const char *enablePromptPattern;     // "Password:" pattern
   const char *paginationDisableCmd;    // "terminal length 0"
   const char *paginationPrompt;        // "--More--" pattern
   const char *paginationContinue;      // " " (space)
   const char *exitCommand;             // "exit"
   uint32_t commandTimeout;             // Default timeout (ms)
   uint32_t connectTimeout;             // Connection timeout (ms)

   SSHDriverHints()
   {
      promptPattern = "[>$]\\s*$";
      enabledPromptPattern = "#\\s*$";
      enableCommand = nullptr;
      enablePromptPattern = nullptr;
      paginationDisableCmd = nullptr;
      paginationPrompt = "-+[Mm][Oo][Rr][Ee]-+";
      paginationContinue = " ";
      exitCommand = "exit";
      commandTimeout = 30000;
      connectTimeout = 10000;
   }
};
```

### SSHInteractiveChannel Class

Class in `src/server/libnxsrv/ssh_channel.cpp`:

```cpp
/**
 * Interactive SSH channel with server-side parsing
 */
class LIBNXSRV_EXPORTABLE SSHInteractiveChannel : public enable_shared_from_this<SSHInteractiveChannel>
{
private:
   shared_ptr<AgentConnection> m_agentConn;
   uint32_t m_channelId;
   SSHDriverHints m_hints;

   // Prompt detection (uses void* for PCRE/PCREW)
   void *m_promptRegex;
   void *m_promptRegexW;       // Wide char version for parseOutput
   void *m_enabledPromptRegex;
   void *m_paginationRegex;

   // State
   bool m_connected;
   bool m_privileged;
   uint32_t m_lastError;
   MutableString m_lastErrorMessage;

   // Data buffer (receives async data from agent)
   ByteStream m_buffer;
   Mutex m_bufferLock;
   Condition m_dataReceived;

   // Internal methods
   bool waitForPrompt(uint32_t timeout);
   bool checkPromptMatch();
   void processIncomingData();
   void stripAnsiCodes();
   bool handlePagination();   // Returns true if pagination detected and handled
   StringList *parseOutput(const char *sentCommand);
   bool isCommandEcho(const wchar_t *line, const char *command);

public:
   SSHInteractiveChannel(const shared_ptr<AgentConnection>& conn, uint32_t channelId,
                         const SSHDriverHints& hints);
   ~SSHInteractiveChannel();

   /**
    * Callback for incoming data from agent
    */
   void onDataReceived(const BYTE *data, size_t size, bool errorIndicator);

   /**
    * Wait for initial prompt after connection
    */
   bool waitForInitialPrompt();

   /**
    * Disable pagination (best effort)
    */
   void disablePagination();

   /**
    * Execute command and return output (uses UTF-8 command)
    */
   StringList *execute(const char *command, uint32_t timeout = 0);

   /**
    * Escalate to privileged mode
    */
   bool escalatePrivilege(const TCHAR *enablePassword);

   /**
    * Check if in privileged mode
    */
   bool isPrivileged() const { return m_privileged; }

   /**
    * Close channel
    */
   void close();

   /**
    * Error information
    */
   uint32_t getLastError() const { return m_lastError; }
   const TCHAR *getLastErrorMessage() const { return m_lastErrorMessage.cstr(); }
};
```

### SSHChannelCallbackIndex Class

Helper class in `src/server/libnxsrv/ssh_channel.cpp` for efficient callback management using uthash:

```cpp
/**
 * Entry for channel handler map
 */
struct SSHChannelCallbackIndexEntry
{
   UT_hash_handle hh;
   uint32_t id;
   SSHChannelDataCallback handler;
};

/**
 * Index for SSH channel data callbacks (uses uthash for O(1) lookup)
 */
class SSHChannelCallbackIndex
{
private:
   SSHChannelCallbackIndexEntry *m_data = nullptr;

public:
   ~SSHChannelCallbackIndex();
   void add(uint32_t id, SSHChannelDataCallback handler);
   void remove(uint32_t id);
   SSHChannelDataCallback get(uint32_t id);
};
```

### Opening a Channel

Channel opening is done via `Node::openInteractiveSSHChannel()` in `src/server/core/node.cpp`:

```cpp
shared_ptr<SSHInteractiveChannel> Node::openInteractiveSSHChannel(const TCHAR *login, const TCHAR *password, uint32_t keyId)
{
   // Check node supports SSH
   if (!(m_capabilities & NC_IS_SSH))
      return nullptr;

   // Get SSH proxy node (may be this node or a designated proxy)
   shared_ptr<Node> proxyNode = getEffectiveSshProxy();
   if (proxyNode == nullptr)
      return nullptr;

   // Get agent connection to the proxy
   shared_ptr<AgentConnectionEx> agentConn = proxyNode->acquireProxyConnection(SSH_PROXY);
   if (agentConn == nullptr)
      return nullptr;

   // Generate unique channel ID
   uint32_t channelId = InterlockedIncrement(&s_sshChannelId);

   // Open channel via agent
   uint32_t rcc = agentConn->openSSHChannel(m_ipAddress, getSshPort(),
      (login != nullptr) ? login : m_sshLogin,
      (password != nullptr) ? password : m_sshPassword,
      keyId, &channelId);

   if (rcc != ERR_SUCCESS)
      return nullptr;

   // Get driver hints
   SSHDriverHints hints;
   if (m_driver != nullptr)
      m_driver->getSSHDriverHints(&hints);

   // Create channel object
   auto channel = make_shared<SSHInteractiveChannel>(agentConn, channelId, hints);

   // Register data callback using weak_ptr to avoid circular reference
   weak_ptr<SSHInteractiveChannel> weakChannel = channel;
   agentConn->setSSHChannelDataHandler(channelId,
      [weakChannel](const BYTE *data, size_t size, bool errorIndicator) {
         auto ch = weakChannel.lock();
         if (ch != nullptr)
            ch->onDataReceived(data, size, errorIndicator);
      });

   return channel;
}
```

The caller is then responsible for calling `waitForInitialPrompt()` and `disablePagination()`.

### Prompt Detection Algorithm

Line-based detection with timeout as secondary signal:

```cpp
bool SSHInteractiveChannel::waitForPrompt(uint32_t timeout)
{
   uint64_t startTime = GetCurrentTimeMs();
   uint64_t lastDataTime = startTime;

   while (GetCurrentTimeMs() - startTime < timeout)
   {
      m_bufferLock.lock();
      bool hasData = m_buffer.size() > 0;
      m_bufferLock.unlock();

      if (hasData)
      {
         lastDataTime = GetCurrentTimeMs();
         processIncomingData();

         if (checkPromptMatch())
            return true;
      }
      else
      {
         // 500ms silence might indicate prompt with non-standard format
         if (GetCurrentTimeMs() - lastDataTime > 500)
         {
            if (checkPromptMatch())
               return true;
         }
      }

      // Wait for more data
      m_dataReceived.wait(100);
   }

   m_lastError = ERR_REQUEST_TIMEOUT;
   m_lastErrorMessage = _T("Timeout waiting for prompt");
   return false;
}

bool SSHInteractiveChannel::checkPromptMatch()
{
   m_bufferLock.lock();

   // Get last line from buffer
   const char *data = reinterpret_cast<const char*>(m_buffer.buffer());
   size_t len = m_buffer.size();

   // Find last newline
   const char *lastLine = data;
   for (size_t i = len; i > 0; i--)
   {
      if (data[i-1] == '\n')
      {
         lastLine = data + i;
         break;
      }
   }

   // Check against prompt pattern
   int rc = pcre_exec(m_promptRegex, nullptr, lastLine, strlen(lastLine),
                      0, 0, nullptr, 0);

   m_bufferLock.unlock();
   return rc >= 0;
}
```

### Processing Incoming Data

```cpp
void SSHInteractiveChannel::processIncomingData()
{
   m_bufferLock.lock();

   // Strip ANSI escape codes
   stripAnsiCodes();

   // Check for pagination prompt and handle it
   handlePagination();

   m_bufferLock.unlock();
}

void SSHInteractiveChannel::handlePagination()
{
   if (m_paginationRegex == nullptr)
      return;

   const char *data = reinterpret_cast<const char*>(m_buffer.buffer());
   size_t len = m_buffer.size();

   // Check last ~100 bytes for pagination prompt
   size_t checkStart = (len > 100) ? len - 100 : 0;

   if (pcre_exec(m_paginationRegex, nullptr, data + checkStart, len - checkStart,
                 0, 0, nullptr, 0) >= 0)
   {
      // Send continue character
      const char *cont = m_hints.paginationContinue ? m_hints.paginationContinue : " ";
      m_agentConn->sendSSHChannelData(m_channelId,
                                       reinterpret_cast<const BYTE*>(cont),
                                       strlen(cont));

      // Strip pagination prompt from buffer
      // ...
   }
}
```

### Command Execution

```cpp
StringList *SSHInteractiveChannel::execute(const char *command, uint32_t timeout)
{
   if (!m_connected)
   {
      m_lastError = ERR_NOT_CONNECTED;
      m_lastErrorMessage = L"Channel not connected";
      return nullptr;
   }

   size_t len = strlen(command);
   if (len == 0)
   {
      m_lastError = ERR_MALFORMED_COMMAND;
      m_lastErrorMessage = L"Command is empty";
      return nullptr;
   }

   if (timeout == 0)
      timeout = m_hints.commandTimeout;

   // Clear buffer
   m_bufferLock.lock();
   m_buffer.clear();
   m_bufferLock.unlock();

   // Send command with newline
   uint32_t rcc;
   if (command[len - 1] != '\n')
   {
      Buffer<char, 4096> cmdWithNewline(len + 2);
      strcpy(cmdWithNewline, command);
      cmdWithNewline[len] = '\n';
      cmdWithNewline[len + 1] = 0;
      rcc = m_agentConn->sendSSHChannelData(m_channelId,
                                            reinterpret_cast<const BYTE*>(cmdWithNewline.buffer()),
                                            len + 1);
   }
   else
   {
      rcc = m_agentConn->sendSSHChannelData(m_channelId,
                                            reinterpret_cast<const BYTE*>(command), len);
   }

   if (rcc != ERR_SUCCESS)
   {
      m_lastError = rcc;
      m_lastErrorMessage = _T("Failed to send command");
      return nullptr;
   }

   // Wait for prompt
   if (!waitForPrompt(timeout))
      return nullptr;

   // Parse output
   return parseOutput(command);
}

StringList *SSHInteractiveChannel::parseOutput(const char *sentCommand)
{
   m_bufferLock.lock();

   // Convert buffer to string (UTF-8 -> wide)
   String output(reinterpret_cast<const char*>(m_buffer.buffer()), m_buffer.size(), "UTF-8");

   m_bufferLock.unlock();

   StringList lines = output.split(L"\n", false);

   // Strip command echo (first line if it matches sent command)
   if ((lines.size() > 0) && isCommandEcho(lines.get(0), sentCommand))
   {
      lines.remove(0);
   }

   // Strip prompt (last line) using wide char regex
   if (lines.size() > 0 && m_promptRegexW != nullptr)
   {
      const wchar_t *lastLine = lines.get(lines.size() - 1);
      if (_pcre_exec_w(m_promptRegexW, nullptr, lastLine, wcslen(lastLine), 0, 0, nullptr, 0) >= 0)
      {
         lines.remove(lines.size() - 1);
      }
   }

   // Remove trailing empty lines
   while(lines.size() > 0)
   {
      const wchar_t *lastLine = lines.get(lines.size() - 1);
      if (lastLine[0] == 0 || (lastLine[0] == '\r' && lastLine[1] == 0))
         lines.remove(lines.size() - 1);
      else
         break;
   }

   return new StringList(std::move(lines));
}
```

### AgentConnection Extension

Extended in `src/server/libnxsrv/agent.cpp`:

```cpp
// Callback type for SSH channel data
typedef std::function<void(const BYTE*, size_t, bool)> SSHChannelDataCallback;

class LIBNXSRV_EXPORTABLE AgentConnection
{
   // ... existing members ...

   // SSH channel support (uses SSHChannelCallbackIndex for O(1) lookup)
   SSHChannelCallbackIndex m_sshChannelHandlers;
   Mutex m_sshChannelLock;

public:
   // ... existing methods ...

   // New SSH channel methods
   uint32_t openSSHChannel(const InetAddress& target, uint16_t port,
                           const TCHAR *user, const TCHAR *password,
                           uint32_t keyId, uint32_t *channelId);

   uint32_t sendSSHChannelData(uint32_t channelId, const BYTE *data, size_t size);

   uint32_t closeSSHChannel(uint32_t channelId);

   void setSSHChannelDataHandler(uint32_t channelId, SSHChannelDataCallback handler);

   void removeSSHChannelDataHandler(uint32_t channelId);

   // Called by message receiver when CMD_SSH_CHANNEL_DATA or CMD_CLOSE_SSH_CHANNEL arrives
   void processSSHChannelData(uint32_t channelId, const BYTE *data, size_t size, bool errorIndicator);
};
```

Note: The callback includes an `errorIndicator` parameter to signal channel closure or errors.

### NetworkDeviceDriver Extension

Add to driver interface:

```cpp
class LIBNXSRV_EXPORTABLE NetworkDeviceDriver
{
public:
   // ... existing methods ...

   /**
    * Get SSH driver hints for interactive CLI sessions
    */
   virtual void getSSHDriverHints(SSHDriverHints *hints) const;
};
```

## Default Driver Hints

### Cisco IOS/IOS-XE

Implemented in `src/server/drivers/cisco/cisco.cpp`:

```cpp
void CiscoDeviceDriver::getSSHDriverHints(SSHDriverHints *hints) const
{
   // Cisco IOS prompt patterns:
   // - User mode: hostname>
   // - Privileged mode: hostname#
   // - Config mode: hostname(config)#, hostname(config-if)#, etc.
   hints->promptPattern = "^[\\w.-]+(\\([\\w-]+\\))?[>#]\\s*$";
   hints->enabledPromptPattern = "^[\\w.-]+(\\([\\w-]+\\))?#\\s*$";
   hints->enableCommand = "enable";
   hints->enablePromptPattern = "[Pp]assword:\\s*$";
   hints->paginationDisableCmd = "terminal length 0";
   hints->paginationPrompt = " --[Mm]ore-- |<--- More --->|--More--";
   hints->paginationContinue = " ";
   hints->exitCommand = "exit";
   hints->commandTimeout = 30000;
   hints->connectTimeout = 15000;
}
```

### Cisco NX-OS (Nexus)

Implemented in `src/server/drivers/cisco/nexus.cpp`:

```cpp
void CiscoNexusDriver::getSSHDriverHints(SSHDriverHints *hints) const
{
   // NX-OS prompt patterns (may include VDC context)
   hints->promptPattern = "^[\\w.-]+(\\([\\w-]+\\))?[>#]\\s*$";
   hints->enabledPromptPattern = "^[\\w.-]+(\\([\\w-]+\\))?#\\s*$";
   hints->enableCommand = "enable";
   hints->enablePromptPattern = "[Pp]assword:\\s*$";
   hints->paginationDisableCmd = "terminal length 0";
   hints->paginationPrompt = " --[Mm]ore-- ";
   hints->paginationContinue = " ";
   hints->exitCommand = "exit";
   hints->commandTimeout = 30000;
   hints->connectTimeout = 15000;
}
```

### Juniper JunOS

Implemented in `src/server/drivers/juniper/juniper.cpp`:

```cpp
void JuniperDriver::getSSHDriverHints(SSHDriverHints *hints) const
{
   // JunOS prompt patterns:
   // - Operational mode: user@hostname>
   // - Configuration mode: user@hostname#
   // - May include routing instance: user@hostname:instance>
   hints->promptPattern = "^[\\w.-]+@[\\w.-]+(:[\\w.-]+)?[>%#]\\s*$";
   hints->enabledPromptPattern = "^[\\w.-]+@[\\w.-]+(:[\\w.-]+)?#\\s*$";

   // JunOS uses class-based authorization, not enable/disable model
   hints->enableCommand = nullptr;
   hints->enablePromptPattern = nullptr;

   // Pagination control
   hints->paginationDisableCmd = "set cli screen-length 0";
   hints->paginationPrompt = "---\\([Mm]ore\\s*\\d*%?\\)---|---\\(more\\)---";
   hints->paginationContinue = " ";

   hints->exitCommand = "exit";
   hints->commandTimeout = 60000;   // JunOS may be slower
   hints->connectTimeout = 20000;
}
```

### MikroTik RouterOS

Implemented in `src/server/drivers/mikrotik/mikrotik.cpp`:

```cpp
void MikrotikDriver::getSSHDriverHints(SSHDriverHints *hints) const
{
   // RouterOS prompt patterns:
   // - Root menu: [admin@MikroTik] >
   // - Submenu: [admin@MikroTik] /ip address>
   // - With safe mode: [admin@MikroTik] <SAFE>>
   hints->promptPattern = "^\\[[\\w.-]+@[\\w.-]+\\]\\s*(<SAFE>)?\\s*(/[\\w/ -]*)?[>]\\s*$";
   hints->enabledPromptPattern = nullptr;  // RouterOS doesn't have enable mode

   // No enable/privilege escalation - uses group-based permissions
   hints->enableCommand = nullptr;
   hints->enablePromptPattern = nullptr;

   // RouterOS doesn't paginate SSH output by default
   hints->paginationDisableCmd = nullptr;
   hints->paginationPrompt = "-- \\[Q quit";
   hints->paginationContinue = " ";

   hints->exitCommand = "/quit";
   hints->commandTimeout = 30000;
   hints->connectTimeout = 10000;
}
```

### Huawei VRP

Implemented in `src/server/drivers/huawei/lansw.cpp`:

```cpp
void HuaweiSWDriver::getSSHDriverHints(SSHDriverHints *hints) const
{
   // Huawei VRP prompt patterns:
   // - User view: <hostname>
   // - System view: [hostname]
   // - Interface view: [hostname-GigabitEthernet0/0/1]
   hints->promptPattern = "^[<\\[][\\w.-]+([-][\\w/.-]+)?[>\\]]\\s*$";
   hints->enabledPromptPattern = "^\\[[\\w.-]+([-][\\w/.-]+)?\\]\\s*$";

   // Privilege escalation uses "super" command
   hints->enableCommand = "super";
   hints->enablePromptPattern = "[Pp]assword:\\s*$";

   // Pagination control
   hints->paginationDisableCmd = "screen-length 0 temporary";
   hints->paginationPrompt = "---- More ----|  ---- More ----|^--More--";
   hints->paginationContinue = " ";

   hints->exitCommand = "quit";
   hints->commandTimeout = 30000;
   hints->connectTimeout = 15000;
}
```

### Hirschmann HiOS / Classic

Implemented in `src/server/drivers/hirschmann/hios.cpp` and `classic.cpp`:

```cpp
void HirschmannHiOSDriver::getSSHDriverHints(SSHDriverHints *hints) const
{
   // HiOS prompt patterns (Cisco IOS-like):
   // - User EXEC mode: hostname>
   // - Privileged EXEC mode: hostname#
   // - Configuration mode: hostname(config)#, hostname(config-if-1/1)#
   hints->promptPattern = "^[\\w.-]+(\\([\\w/-]+\\))?[>#]\\s*$";
   hints->enabledPromptPattern = "^[\\w.-]+(\\([\\w/-]+\\))?#\\s*$";

   hints->enableCommand = "enable";
   hints->enablePromptPattern = "[Pp]assword:\\s*$";

   // HiOS uses "terminal datadump" to disable paging
   hints->paginationDisableCmd = "terminal datadump";
   hints->paginationPrompt = "--More--|Press any key";
   hints->paginationContinue = " ";

   hints->exitCommand = "exit";
   hints->commandTimeout = 30000;
   hints->connectTimeout = 15000;
}
```

### Extreme EXOS

Implemented in `src/server/drivers/extreme/extreme.cpp`:

```cpp
void ExtremeDriver::getSSHDriverHints(SSHDriverHints *hints) const
{
   // EXOS prompt patterns:
   // - Normal: hostname.slot #
   // - Unsaved config: * hostname.slot #
   // - May have slot number or not: hostname #
   hints->promptPattern = "^\\*?\\s*[\\w.-]+(\\.[0-9]+)?\\s*[#>]\\s*$";
   hints->enabledPromptPattern = nullptr;  // EXOS doesn't have enable mode

   // EXOS uses role-based access, no enable/privilege escalation
   hints->enableCommand = nullptr;
   hints->enablePromptPattern = nullptr;

   // Pagination control
   hints->paginationDisableCmd = "disable clipaging";
   hints->paginationPrompt = "Press <SPACE> to continue or <Q> to quit:";
   hints->paginationContinue = " ";

   hints->exitCommand = "exit";
   hints->commandTimeout = 30000;
   hints->connectTimeout = 15000;
}
```

### Generic Default

```cpp
void NetworkDeviceDriver::getSSHDriverHints(SSHDriverHints *hints) const
{
   hints->promptPattern = "^[\\w@.-]+[>#$%]\\s*$";
   hints->enabledPromptPattern = "^[\\w@.-]+[#]\\s*$";
   hints->enableCommand = nullptr;
   hints->enablePromptPattern = nullptr;
   hints->paginationDisableCmd = nullptr;
   hints->paginationPrompt = nullptr;
   hints->paginationContinue = " ";
   hints->exitCommand = "exit";
   hints->commandTimeout = 30000;
   hints->connectTimeout = 10000;
}
```

## NXSL Bindings

NXSL bindings are implemented in `src/server/core/nxsl_classes.cpp`:

### Node::openSSHSession Method

```nxsl
// Open SSH session (uses node's credentials if not specified)
session = $node->openSSHSession();

// Or with explicit credentials
session = $node->openSSHSession("admin", "password123");

// Or with SSH key
session = $node->openSSHSession("admin", null, keyId);
```

### SSHSession Class

**Methods:**
- `execute(command, [timeout])` - Execute command, returns array of output lines
- `escalatePrivilege(password)` - Enter privileged mode, returns boolean
- `close()` - Close the session

**Attributes:**
- `connected` - Boolean, true if session is active
- `privileged` - Boolean, true if in privileged mode
- `lastError` - Integer error code
- `lastErrorMessage` - Error description string
- `nodeId` - Node ID this session is connected to

### NXSL Usage Example

```nxsl
// Open SSH session to network device
session = $node->openSSHSession();
if (session == null) {
   trace(1, "Failed to open SSH session");
   return null;
}

// Escalate privileges if needed
if (!session->escalatePrivilege("enable_password")) {
   trace(1, "Failed to escalate: " . session->lastErrorMessage);
}

// Execute commands
version = session->execute("show version");
for (line : version) {
   trace(3, line);
}

interfaces = session->execute("show interfaces brief");
config = session->execute("show running-config", 60000);  // 60 second timeout

// Session auto-closes when script ends, or explicitly:
session->close();
```

## Usage Example (C++)

```cpp
// Server-side code using SSHInteractiveChannel
void executeNetworkCommands(const shared_ptr<Node>& node)
{
   // Open interactive channel (uses node's credentials)
   auto channel = node->openInteractiveSSHChannel(nullptr, nullptr, 0);
   if (channel == nullptr)
   {
      nxlog_debug(_T("Failed to open SSH channel"));
      return;
   }

   // Wait for initial prompt
   if (!channel->waitForInitialPrompt())
   {
      nxlog_debug(_T("Failed waiting for prompt: %s"), channel->getLastErrorMessage());
      return;
   }

   // Disable pagination (best effort)
   channel->disablePagination();

   // Escalate privileges if needed
   if (!channel->escalatePrivilege(node->getEnablePassword()))
   {
      nxlog_debug(_T("Failed to escalate: %s"), channel->getLastErrorMessage());
   }

   // Execute multiple commands (channel stays open)
   // Note: commands use UTF-8 (const char*)
   auto version = channel->execute("show version");
   auto config = channel->execute("show running-config");
   auto interfaces = channel->execute("show interfaces");

   // Process outputs...
   delete version;
   delete config;
   delete interfaces;

   // Channel closed automatically by destructor, or explicitly:
   channel->close();
}
```

## File Locations

```
src/agent/subagents/ssh/
├── ssh_subagent.h       (SSHChannelProxy declaration, manager functions)
├── session.cpp          (SSHSession::openInteractiveChannel)
├── channel_proxy.cpp    (SSHChannelProxy implementation + command handlers)
└── main.cpp             (register new commands)

src/server/libnxsrv/
├── agent.cpp            (AgentConnection SSH channel methods)
├── ssh_channel.cpp      (SSHInteractiveChannel, SSHChannelCallbackIndex)
└── ndd.cpp              (NetworkDeviceDriver::getSSHDriverHints)

src/server/core/
└── node.cpp             (Node::openInteractiveSSHChannel)

include/
├── nms_cscp.h           (CMD_SETUP_SSH_CHANNEL, CMD_SSH_CHANNEL_DATA, CMD_CLOSE_SSH_CHANNEL)
├── nxsrvapi.h           (SSHInteractiveChannel, SSHDriverHints, SSHChannelDataCallback)
└── nddrv.h              (NetworkDeviceDriver::getSSHDriverHints declaration)
```

## Error Handling

| Error | Handling |
|-------|----------|
| Agent connection fails | Return nullptr from open(), set lastError |
| PTY request fails | Agent returns error code, open() fails |
| Prompt detection timeout | Return nullptr from execute(), set ERR_REQUEST_TIMEOUT |
| Channel closed unexpectedly | Set error state, subsequent calls fail |
| Pagination not disabled | Runtime fallback handles it transparently |
| Privilege escalation fails | Return false, channel remains in user mode |

## Testing Considerations

### Unit Tests

1. ANSI code stripping
2. Prompt pattern matching
3. Pagination detection
4. Command echo detection
5. Buffer parsing

### Integration Tests

1. Real Cisco device (or GNS3/EVE-NG emulator)
2. Real Juniper device (or vSRX)
3. Linux server with SSH
4. Session pool behavior
5. Privilege escalation
6. Long output handling (pagination)

## Implementation Checklist

### Agent Side (SSH Subagent)

- [x] Extend `SSHSession` with `openInteractiveChannel()`
- [x] Implement `SSHChannelProxy` class with reader thread
- [x] Raw data forwarding using `CMD_SSH_CHANNEL_DATA`
- [x] Command handlers for `CMD_SETUP_SSH_CHANNEL`, `CMD_SSH_CHANNEL_DATA`, `CMD_CLOSE_SSH_CHANNEL`
- [x] Channel proxy management (create, lookup, destroy)
- [x] Debug logging with `DEBUG_TAG`

### Server Side (libnxsrv + core)

- [x] `SSHInteractiveChannel` class with `enable_shared_from_this`
- [x] `SSHDriverHints` structure with defaults in constructor
- [x] `SSHChannelCallbackIndex` for efficient callback lookup (uthash)
- [x] Extend `AgentConnection` with SSH channel methods
- [x] `Node::openInteractiveSSHChannel()` factory method
- [x] Prompt detection (line-based + 500ms timeout fallback)
- [x] Pagination handling (`disablePagination()` + runtime `handlePagination()`)
- [x] ANSI code stripping with comprehensive regex pattern
- [x] Command echo stripping
- [x] Wide char regex support (`m_promptRegexW`) for output parsing
- [x] Extend `NetworkDeviceDriver` interface with `getSSHDriverHints()`

### Protocol

- [x] Define `CMD_SETUP_SSH_CHANNEL` (0x01F3)
- [x] Define `CMD_SSH_CHANNEL_DATA` (0x01F4)
- [x] Define `CMD_CLOSE_SSH_CHANNEL` (0x01F5)
- [x] VID codes reused from existing fields

### Driver Hints

- [x] Generic default implementation in `SSHDriverHints` constructor
- [x] Cisco IOS/IOS-XE driver hints (`CiscoDeviceDriver::getSSHDriverHints`)
- [x] Cisco NX-OS driver hints (`CiscoNexusDriver::getSSHDriverHints`)
- [x] Juniper JunOS driver hints (`JuniperDriver::getSSHDriverHints`)
- [x] MikroTik RouterOS driver hints (`MikrotikDriver::getSSHDriverHints`)
- [x] Huawei VRP driver hints (`HuaweiSWDriver::getSSHDriverHints`)
- [x] Hirschmann HiOS driver hints (`HirschmannHiOSDriver::getSSHDriverHints`)
- [x] Hirschmann Classic driver hints (`HirschmannClassicDriver::getSSHDriverHints`)
- [x] Extreme EXOS driver hints (`ExtremeDriver::getSSHDriverHints`)
- [ ] Other vendor-specific hints (Arista, HP/Aruba, etc.)
- [ ] Node-level override support for SSH hints

### Integration

- [x] NXSL bindings (`Node::openSSHSession()`, `SSHSession` class)
- [ ] Web API exposure for interactive SSH sessions
- [ ] Java client UI for interactive terminal
- [ ] Session pooling on server side

### Documentation

- [x] Update this design document
- [ ] Configuration reference
- [ ] Troubleshooting guide

## Future Considerations

- Concurrent channel limit configuration
- Connection multiplexing (multiple interactive channels per SSH session)
- Terminal size negotiation from client
- Session recording/audit logging
- Macro/script execution for common device operations
