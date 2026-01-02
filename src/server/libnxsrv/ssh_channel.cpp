/*
** NetXMS - Network Management System
** Server Library
** Copyright (C) 2003-2026 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: ssh_channel.cpp
**
**/

#include "libnxsrv.h"
#include <netxms-regex.h>
#include <uthash.h>

#define DEBUG_TAG L"ssh.channel"

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
 * Destructor
 */
SSHChannelCallbackIndex::~SSHChannelCallbackIndex()
{
   SSHChannelCallbackIndexEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      HASH_DEL(m_data, entry);
      delete entry;
   }
}

/**
 * Add handler with given ID
 */
void SSHChannelCallbackIndex::add(uint32_t id, SSHChannelDataCallback handler)
{
   SSHChannelCallbackIndexEntry *entry = new SSHChannelCallbackIndexEntry();
   entry->id = id;
   entry->handler = handler;
   HASH_ADD_INT(m_data, id, entry);
}

/**
 * Remove handler by ID
 */
void SSHChannelCallbackIndex::remove(uint32_t id)
{
   SSHChannelCallbackIndexEntry *entry;
   HASH_FIND_INT(m_data, &id, entry);
   if (entry != nullptr)
   {
      HASH_DEL(m_data, entry);
      delete entry;
   }
}

/**
 * Get handler by ID
 */
SSHChannelDataCallback SSHChannelCallbackIndex::get(uint32_t id)
{
   SSHChannelCallbackIndexEntry *entry;
   HASH_FIND_INT(m_data, &id, entry);
   return (entry != nullptr) ? entry->handler : nullptr;
}

/**
 * SSHInteractiveChannel constructor
 */
SSHInteractiveChannel::SSHInteractiveChannel(const shared_ptr<AgentConnection>& conn, uint32_t channelId, const SSHDriverHints& hints)
   : m_agentConn(conn), m_hints(hints), m_buffer(65536), m_dataReceived(true)
{
   m_channelId = channelId;
   m_promptRegex = nullptr;
   m_promptRegexW = nullptr;
   m_enabledPromptRegex = nullptr;
   m_enabledPromptRegexW = nullptr;
   m_paginationRegex = nullptr;
   m_connected = true;
   m_privileged = false;
   m_lastError = ERR_SUCCESS;

   // Compile prompt regex
   if (hints.promptPattern != nullptr)
   {
      const char *errptr;
      int erroffset;
      m_promptRegex = pcre_compile(hints.promptPattern, PCRE_COMMON_FLAGS_A, &errptr, &erroffset, nullptr);
      if (m_promptRegex == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("SSHInteractiveChannel: failed to compile prompt pattern: %hs"), errptr);
      }

      wchar_t wPromptPattern[256];
      utf8_to_wchar(hints.promptPattern, -1, wPromptPattern, 256);
      m_promptRegexW = _pcre_compile_w(reinterpret_cast<const PCRE_WCHAR*>(wPromptPattern), PCRE_COMMON_FLAGS_W, &errptr, &erroffset, nullptr);
      if (m_promptRegexW == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("SSHInteractiveChannel: failed to compile prompt pattern: %hs"), errptr);
      }
   }

   // Compile enabled prompt regex
   if (hints.enabledPromptPattern != nullptr)
   {
      const char *errptr;
      int erroffset;
      m_enabledPromptRegex = pcre_compile(hints.enabledPromptPattern, PCRE_COMMON_FLAGS_A, &errptr, &erroffset, nullptr);
      if (m_enabledPromptRegex == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("SSHInteractiveChannel: failed to compile enabled prompt pattern: %hs"), errptr);
      }

      wchar_t wEnabledPromptPattern[256];
      utf8_to_wchar(hints.enabledPromptPattern, -1, wEnabledPromptPattern, 256);
      m_enabledPromptRegexW = _pcre_compile_w(reinterpret_cast<const PCRE_WCHAR*>(wEnabledPromptPattern), PCRE_COMMON_FLAGS_W, &errptr, &erroffset, nullptr);
      if (m_enabledPromptRegexW == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("SSHInteractiveChannel: failed to compile enabled prompt pattern (wide): %hs"), errptr);
      }
   }

   // Compile pagination regex
   if (hints.paginationPrompt != nullptr)
   {
      const char *errptr;
      int erroffset;
      m_paginationRegex = pcre_compile(hints.paginationPrompt, PCRE_COMMON_FLAGS_A, &errptr, &erroffset, nullptr);
      if (m_paginationRegex == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("SSHInteractiveChannel: failed to compile pagination pattern: %hs"), errptr);
      }
   }
}

/**
 * SSHInteractiveChannel destructor
 */
SSHInteractiveChannel::~SSHInteractiveChannel()
{
   close();

   if (m_promptRegex != nullptr)
      pcre_free(static_cast<pcre*>(m_promptRegex));
   if (m_promptRegexW != nullptr)
      _pcre_free_w(static_cast<PCREW*>(m_promptRegexW));
   if (m_enabledPromptRegex != nullptr)
      pcre_free(static_cast<pcre*>(m_enabledPromptRegex));
   if (m_enabledPromptRegexW != nullptr)
      _pcre_free_w(static_cast<PCREW*>(m_enabledPromptRegexW));
   if (m_paginationRegex != nullptr)
      pcre_free(static_cast<pcre*>(m_paginationRegex));
}

/**
 * Callback for incoming data from agent
 */
void SSHInteractiveChannel::onDataReceived(const BYTE *data, size_t size, bool errorIndicator)
{
   if (errorIndicator)
   {
      m_connected = false;
      m_lastError = ERR_CONNECTION_BROKEN;
      m_lastErrorMessage = L"SSH channel closed unexpectedly";
      m_dataReceived.set();
      return;
   }

   if (data != nullptr && size > 0)
   {
      if (nxlog_get_debug_level_tag(DEBUG_TAG) >= 7)
      {
         Buffer<char, 4096> buffer(size + 1);
         memcpy(buffer.buffer(), data, size);
         buffer[size] = 0;
         nxlog_debug_tag(DEBUG_TAG, 7, L"SSHInteractiveChannel::onDataReceived: received %u bytes: %hs", static_cast<uint32_t>(size), buffer.buffer());
      }

      // Respond to terminal queries before buffering
      respondToTerminalQueries(data, size);

      m_bufferLock.lock();
      m_buffer.write(data, size);
      m_bufferLock.unlock();
      m_dataReceived.set();
   }
}

/**
 * Wait for initial prompt after connection
 */
bool SSHInteractiveChannel::waitForInitialPrompt()
{
   return waitForPrompt(m_hints.connectTimeout);
}

/**
 * Disable pagination (best effort)
 */
void SSHInteractiveChannel::disablePagination()
{
   if (m_hints.paginationDisableCmd != nullptr)
   {
      StringList *result = execute(m_hints.paginationDisableCmd, 5000);
      delete result;
   }
}

/**
 * Wait for prompt with timeout
 */
bool SSHInteractiveChannel::waitForPrompt(uint32_t timeout)
{
   uint64_t startTime = GetCurrentTimeMs();
   uint64_t lastDataTime = startTime;

   while(GetCurrentTimeMs() - startTime < timeout)
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

/**
 * Check if buffer ends with prompt
 */
bool SSHInteractiveChannel::checkPromptMatch()
{
   if ((m_promptRegex == nullptr) && (m_enabledPromptRegex == nullptr))
      return false;

   m_bufferLock.lock();

   const char *data = reinterpret_cast<const char*>(m_buffer.buffer());
   size_t len = m_buffer.size();

   if (len == 0)
   {
      m_bufferLock.unlock();
      return false;
   }

   // Skip trailing whitespace and newlines (some devices add newlines after prompt)
   while (len > 0 && (data[len - 1] == '\n' || data[len - 1] == '\r' || data[len - 1] == ' ' || data[len - 1] == '\t'))
      len--;

   if (len == 0)
   {
      m_bufferLock.unlock();
      return false;
   }

   // Find last line (after trimming)
   const char *lastLine = data;
   for (size_t i = len; i > 0; i--)
   {
      if (data[i - 1] == '\n')
      {
         lastLine = data + i;
         break;
      }
   }

   size_t lastLineLen = len - (lastLine - data);

   int rc = -1;

   // If already privileged, check enabled prompt first
   if (m_privileged && m_enabledPromptRegex != nullptr)
   {
      rc = pcre_exec(static_cast<pcre*>(m_enabledPromptRegex), nullptr, lastLine, static_cast<int>(lastLineLen), 0, 0, nullptr, 0);
      if (rc >= 0)
      {
         m_bufferLock.unlock();
         return true;
      }
   }

   // Check normal prompt pattern
   if (m_promptRegex != nullptr)
   {
      rc = pcre_exec(static_cast<pcre*>(m_promptRegex), nullptr, lastLine, static_cast<int>(lastLineLen), 0, 0, nullptr, 0);
   }

   // If normal prompt didn't match, try enabled prompt (handles initial privileged connection)
   if (rc < 0 && m_enabledPromptRegex != nullptr)
   {
      rc = pcre_exec(static_cast<pcre*>(m_enabledPromptRegex), nullptr, lastLine, static_cast<int>(lastLineLen), 0, 0, nullptr, 0);
      if (rc >= 0)
         m_privileged = true;
   }

   m_bufferLock.unlock();
   return rc >= 0;
}

/**
 * Process incoming data - strip control characters, collapse char-by-char output, handle pagination
 */
void SSHInteractiveChannel::processIncomingData()
{
   m_bufferLock.lock();
   removeControlCharacters();
   collapseCharacterByCharacterOutput();
   m_bufferLock.unlock();

   // Check for pagination prompt and handle it
   while(handlePagination())
   {
      // Wait a bit for more data after sending pagination continue
      m_dataReceived.wait(200);
      m_bufferLock.lock();
      removeControlCharacters();
      collapseCharacterByCharacterOutput();
      m_bufferLock.unlock();
   }
}


/**
 * Respond to terminal queries in the data
 * Detects ESC[6n (cursor position), ESC Z (identify), ESC[c (device attributes) and sends appropriate responses
 */
void SSHInteractiveChannel::respondToTerminalQueries(const BYTE *data, size_t len)
{
   for (size_t i = 0; i + 1 < len; i++)
   {
      if (data[i] == 0x1B)  // ESC
      {
         // Check for ESC[6n (Device Status Report - cursor position query)
         if (i + 3 < len && data[i + 1] == '[' && data[i + 2] == '6' && data[i + 3] == 'n')
         {
            // Respond with cursor at row 1, column 1
            const char *response = "\x1B[1;1R";
            m_agentConn->sendSSHChannelData(m_channelId, reinterpret_cast<const BYTE*>(response), strlen(response));
            nxlog_debug_tag(DEBUG_TAG, 7, L"SSHInteractiveChannel %u: responded to cursor position query (ESC[6n)", m_channelId);
         }
         // Check for ESC Z (Identify Terminal)
         else if (data[i + 1] == 'Z')
         {
            // Respond as VT100: ESC[?1;0c (VT100 with no options)
            const char *response = "\x1B[?1;0c";
            m_agentConn->sendSSHChannelData(m_channelId, reinterpret_cast<const BYTE*>(response), strlen(response));
            nxlog_debug_tag(DEBUG_TAG, 7, L"SSHInteractiveChannel %u: responded to terminal identify query (ESC Z)", m_channelId);
         }
         // Check for ESC[c or ESC[0c (Device Attributes query)
         else if (i + 2 < len && data[i + 1] == '[' && (data[i + 2] == 'c' || (data[i + 2] == '0' && i + 3 < len && data[i + 3] == 'c')))
         {
            // Respond as VT100
            const char *response = "\x1B[?1;0c";
            m_agentConn->sendSSHChannelData(m_channelId, reinterpret_cast<const BYTE*>(response), strlen(response));
            nxlog_debug_tag(DEBUG_TAG, 7, L"SSHInteractiveChannel %u: responded to device attributes query (ESC[c)", m_channelId);
         }
      }
   }
}

/**
 * Check if character is ASCII printable (0x20-0x7E)
 */
static inline bool IsAsciiPrintable(unsigned char c)
{
   return c >= 0x20 && c <= 0x7E;
}

/**
 * Collapse character-by-character output with CR/LF between each character
 * This handles devices like MikroTik that echo each character on a new line
 */
void SSHInteractiveChannel::collapseCharacterByCharacterOutput()
{
   if (m_buffer.size() == 0)
      return;

   char *data = reinterpret_cast<char*>(const_cast<BYTE*>(m_buffer.buffer()));
   size_t len = m_buffer.size();

   // Work in-place (always writing to positions <= reading position)
   char *src = data;
   char *dst = data;
   char *end = data + len;

   while (src < end)
   {
      // Check if this looks like char-by-char output: single ASCII printable char followed by newline
      if (IsAsciiPrintable((unsigned char)*src) && (src + 1 < end && *(src + 1) == '\n'))
      {
         // Check if next non-newline char is also a single char followed by newline
         // This indicates char-by-char mode
         char *next = src + 1;
         while (next < end && *next == '\n')
            next++;

         if (next < end && IsAsciiPrintable((unsigned char)*next) && (next + 1 < end && *(next + 1) == '\n'))
         {
            // Looks like char-by-char mode - copy char without trailing newline
            *dst++ = *src++;
            // Skip the newlines between chars
            while (src < end && *src == '\n')
               src++;
            continue;
         }
      }

      // Normal character (including newlines) - preserve as-is
      *dst++ = *src++;
   }

   m_buffer.truncate(dst - data);
}

/**
 * Handle pagination prompt - returns true if pagination was detected and handled
 */
bool SSHInteractiveChannel::handlePagination()
{
   if (m_paginationRegex == nullptr)
      return false;

   m_bufferLock.lock();

   const char *data = reinterpret_cast<const char*>(m_buffer.buffer());
   size_t len = m_buffer.size();

   if (len == 0)
   {
      m_bufferLock.unlock();
      return false;
   }

   // Check last ~100 bytes for pagination prompt
   size_t checkStart = (len > 100) ? len - 100 : 0;
   int ovector[30];

   int rc = pcre_exec(static_cast<pcre*>(m_paginationRegex), nullptr, data + checkStart, static_cast<int>(len - checkStart), 0, 0, ovector, 30);
   if (rc < 0)
   {
      m_bufferLock.unlock();
      return false;
   }

   // Remove pagination prompt from buffer
   size_t promptStart = checkStart + ovector[0];
   size_t promptEnd = checkStart + ovector[1];
   ByteStream newBuffer(len);
   newBuffer.write(data, promptStart);
   if (promptEnd < len)
      newBuffer.write(data + promptEnd, len - promptEnd);
   m_buffer.clear();
   m_buffer.write(newBuffer.buffer(), newBuffer.size());

   m_bufferLock.unlock();

   // Send continue character
   const char *cont = m_hints.paginationContinue ? m_hints.paginationContinue : " ";
   m_agentConn->sendSSHChannelData(m_channelId, reinterpret_cast<const BYTE*>(cont), strlen(cont));

   nxlog_debug_tag(DEBUG_TAG, 6, L"SSHInteractiveChannel %u: pagination detected, sent continue", m_channelId);
   return true;
}

/**
 * Execute command and return output
 */
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
      rcc = m_agentConn->sendSSHChannelData(m_channelId, reinterpret_cast<const BYTE*>(cmdWithNewline.buffer()), len + 1);
   }
   else
   {
      rcc = m_agentConn->sendSSHChannelData(m_channelId, reinterpret_cast<const BYTE*>(command), len);
   }
   if (rcc != ERR_SUCCESS)
   {
      m_lastError = rcc;
      m_lastErrorMessage = _T("Failed to send command");
      return nullptr;
   }

   nxlog_debug_tag(DEBUG_TAG, 6, _T("SSHInteractiveChannel %u: sent command '%hs'"), m_channelId, command);

   // Wait for prompt
   if (!waitForPrompt(timeout))
      return nullptr;

   // Parse output
   return parseOutput(command);
}

/**
 * Parse command output from buffer
 */
StringList *SSHInteractiveChannel::parseOutput(const char *sentCommand)
{
   m_bufferLock.lock();

   // Process any remaining control characters (data may have arrived after last processIncomingData call)
   removeControlCharacters();
   collapseCharacterByCharacterOutput();

   // Convert buffer to string
   String output(reinterpret_cast<const char*>(m_buffer.buffer()), m_buffer.size(), "UTF-8");

   m_bufferLock.unlock();

   StringList lines = output.split(L"\n", false);

   // Strip command echo (first line if it matches sent command)
   if ((lines.size() > 0) && isCommandEcho(lines.get(0), sentCommand))
   {
      lines.remove(0);
   }

   // Strip prompt (last line)
   if (lines.size() > 0)
   {
      const wchar_t *lastLine = lines.get(lines.size() - 1);
      int lastLineLen = static_cast<int>(wcslen(lastLine));
      bool promptMatched = false;

      // If privileged, check enabled prompt first
      if (m_privileged && m_enabledPromptRegexW != nullptr)
      {
         promptMatched = _pcre_exec_w(static_cast<PCREW*>(m_enabledPromptRegexW), nullptr, reinterpret_cast<const PCRE_WCHAR*>(lastLine), lastLineLen, 0, 0, nullptr, 0) >= 0;
      }

      // Check normal prompt if not matched
      if (!promptMatched && m_promptRegexW != nullptr)
      {
         promptMatched = _pcre_exec_w(static_cast<PCREW*>(m_promptRegexW), nullptr, reinterpret_cast<const PCRE_WCHAR*>(lastLine), lastLineLen, 0, 0, nullptr, 0) >= 0;
      }

      if (promptMatched)
         lines.remove(lines.size() - 1);
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

/**
 * Check if line is command echo
 */
bool SSHInteractiveChannel::isCommandEcho(const wchar_t *line, const char *command)
{
   wchar_t wcCommand[1024];
   utf8_to_wchar(command, -1, wcCommand, 1024);
   return wcsstr(line, wcCommand) != nullptr;
}

/**
 * Escalate to privileged mode
 */
bool SSHInteractiveChannel::escalatePrivilege(const TCHAR *enablePassword)
{
   if (!m_connected)
   {
      m_lastError = ERR_NOT_CONNECTED;
      m_lastErrorMessage = _T("Channel not connected");
      return false;
   }

   if (m_privileged)
      return true;  // Already privileged

   if (m_hints.enableCommand == nullptr)
   {
      m_lastError = ERR_NOT_IMPLEMENTED;
      m_lastErrorMessage = _T("Enable command not configured for this device type");
      return false;
   }

   // Clear buffer
   m_bufferLock.lock();
   m_buffer.clear();
   m_bufferLock.unlock();

   // Send enable command
   char cmdBuffer[256];
   strlcpy(cmdBuffer, m_hints.enableCommand, sizeof(cmdBuffer) - 2);
   strlcat(cmdBuffer, "\n", sizeof(cmdBuffer));

   uint32_t rcc = m_agentConn->sendSSHChannelData(m_channelId, reinterpret_cast<const BYTE*>(cmdBuffer), strlen(cmdBuffer));
   if (rcc != ERR_SUCCESS)
   {
      m_lastError = rcc;
      m_lastErrorMessage = _T("Failed to send enable command");
      return false;
   }

   // Wait for password prompt or privileged prompt
   uint64_t startTime = GetCurrentTimeMs();
   while(GetCurrentTimeMs() - startTime < 10000)
   {
      m_dataReceived.wait(100);
      processIncomingData();

      // Check if we got privileged prompt directly (no password required)
      if (checkPromptMatch() && m_privileged)
         return true;

      // Check for password prompt
      if (m_hints.enablePromptPattern != nullptr)
      {
         m_bufferLock.lock();
         const char *data = reinterpret_cast<const char*>(m_buffer.buffer());
         size_t len = m_buffer.size();

         const char *errptr;
         int erroffset;
         pcre *pwdRegex = pcre_compile(m_hints.enablePromptPattern, PCRE_COMMON_FLAGS_A, &errptr, &erroffset, nullptr);
         if (pwdRegex != nullptr)
         {
            int rc = pcre_exec(pwdRegex, nullptr, data, static_cast<int>(len), 0, 0, nullptr, 0);
            pcre_free(pwdRegex);

            if (rc >= 0)
            {
               m_bufferLock.unlock();

               // Send password
               char pwdBuffer[256];
#ifdef UNICODE
               wchar_to_utf8(enablePassword, -1, pwdBuffer, sizeof(pwdBuffer) - 2);
#else
               strlcpy(pwdBuffer, enablePassword, sizeof(pwdBuffer) - 2);
#endif
               strlcat(pwdBuffer, "\n", sizeof(pwdBuffer));

               m_buffer.clear();
               rcc = m_agentConn->sendSSHChannelData(m_channelId, reinterpret_cast<const BYTE*>(pwdBuffer), strlen(pwdBuffer));
               if (rcc != ERR_SUCCESS)
               {
                  m_lastError = rcc;
                  m_lastErrorMessage = _T("Failed to send enable password");
                  return false;
               }

               // Wait for privileged prompt
               if (waitForPrompt(10000) && m_privileged)
                  return true;

               m_lastError = ERR_ACCESS_DENIED;
               m_lastErrorMessage = _T("Enable password rejected");
               return false;
            }
         }
         m_bufferLock.unlock();
      }
   }

   m_lastError = ERR_REQUEST_TIMEOUT;
   m_lastErrorMessage = _T("Timeout during privilege escalation");
   return false;
}

/**
 * Close channel
 */
void SSHInteractiveChannel::close()
{
   if (!m_connected)
      return;

   m_connected = false;
   m_agentConn->removeSSHChannelDataHandler(m_channelId);
   m_agentConn->closeSSHChannel(m_channelId);

   nxlog_debug_tag(DEBUG_TAG, 5, _T("SSHInteractiveChannel %u: closed"), m_channelId);
}
