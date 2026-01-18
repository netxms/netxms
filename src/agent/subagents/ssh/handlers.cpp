/*
** NetXMS SSH subagent
** Copyright (C) 2004-2026 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: handlers.cpp
**
**/

#include "ssh_subagent.h"
#include <netxms-regex.h>

/**
 * SSH connectivity check handler
 * Returns: 0 = if SSH not available or 1 if SSH connection available
 */
LONG H_SSHConnection(const TCHAR* param, const TCHAR* arg, TCHAR* value, AbstractCommSession* session)
{
   TCHAR hostName[256], login[64], password[64];
   if (!AgentGetParameterArg(param, 1, hostName, 256) ||
       !AgentGetParameterArg(param, 2, login, 64) ||
       !AgentGetParameterArg(param, 3, password, 64))
   {
      return SYSINFO_RC_UNSUPPORTED;
   }

   uint16_t port = 22;
   TCHAR *p = _tcschr(hostName, _T(':'));
   if (p != nullptr)
   {
      *p = 0;
      p++;
      port = static_cast<uint16_t>(_tcstoul(p, nullptr, 10));
   }

   InetAddress addr = InetAddress::resolveHostName(hostName);
   if (!addr.isValidUnicast())
   {
      ret_int(value, 0);
      return SYSINFO_RC_SUCCESS;
   }

   uint32_t keyId;
   AgentGetMetricArgAsUInt32(param, 4, &keyId);
   shared_ptr<KeyPair> keys;
   if (keyId != 0)
      keys = GetSshKey(session, keyId);

   SSHSession *ssh = AcquireSession(addr, port, login, password, keys);
   if (ssh == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Failed to create SSH connection to %s:%u"), hostName, port);
      ret_int(value, 0);
      return SYSINFO_RC_SUCCESS;
   }
   ReleaseSession(ssh);

   nxlog_debug_tag(DEBUG_TAG, 8, _T("SSH connection to %s:%u created successfully"), hostName, port);
   ret_int(value, 1);
   return SYSINFO_RC_SUCCESS;
}

/**
 * SSH command mode check handler
 * Parameters: host[:port], login, password, keyId, command, pattern
 * Executes command via exec channel and matches output against regex pattern
 * Returns: 1 (success - pattern matched) or 0 (failure)
 */
LONG H_SSHCheckCommandMode(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR hostName[256], login[64], password[64];
   if (!AgentGetParameterArg(param, 1, hostName, 256) ||
       !AgentGetParameterArg(param, 2, login, 64) ||
       !AgentGetParameterArg(param, 3, password, 64))
   {
      return SYSINFO_RC_UNSUPPORTED;
   }

   uint16_t port = 22;
   TCHAR *p = _tcschr(hostName, _T(':'));
   if (p != nullptr)
   {
      *p = 0;
      p++;
      port = static_cast<uint16_t>(_tcstoul(p, nullptr, 10));
   }

   InetAddress addr = InetAddress::resolveHostName(hostName);
   if (!addr.isValidUnicast())
   {
      ret_int(value, 0);
      return SYSINFO_RC_SUCCESS;
   }

   // Get optional key ID (4th argument)
   uint32_t keyId = 0;
   AgentGetMetricArgAsUInt32(param, 4, &keyId);
   shared_ptr<KeyPair> keys;
   if (keyId != 0)
      keys = GetSshKey(session, keyId);

   // Get optional command (5th argument)
   size_t commandBufferLen = _tcslen(param);
   TCHAR *command = MemAllocString(commandBufferLen);
   if (!AgentGetParameterArg(param, 5, command, commandBufferLen) || command[0] == 0)
   {
      _tcscpy(command, _T("echo netxms_test_12345"));
   }

   // Get optional pattern (6th argument)
   TCHAR pattern[256];
   if (!AgentGetParameterArg(param, 6, pattern, 256) || pattern[0] == 0)
   {
      _tcscpy(pattern, _T("netxms_test_12345"));
   }

   int result = 0;

   for (int attempt = 0; attempt < 2; attempt++)
   {
      SSHSession *ssh = AcquireSession(addr, port, login, password, keys);
      if (ssh == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("SSH.CheckCommandMode: failed to create SSH connection to %s:%u"), hostName, port);
         break;
      }

      StringList *output = ssh->execute(command);
      if (output != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 8, _T("SSH.CheckCommandMode: command executed on %s:%u, %d output lines, checking pattern \"%s\""), hostName, port, output->size(), pattern);

         const char *errptr;
         int erroffset;
         PCRE *preg = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(pattern), PCRE_COMMON_FLAGS, &errptr, &erroffset, nullptr);
         if (preg != nullptr)
         {
            int ovector[30];
            for (int i = 0; i < output->size(); i++)
            {
               const TCHAR *line = output->get(i);
               int rc = _pcre_exec_t(preg, nullptr, reinterpret_cast<const PCRE_TCHAR*>(line),
                                     static_cast<int>(_tcslen(line)), 0, 0, ovector, 30);
               if (rc >= 0)
               {
                  result = 1;
                  nxlog_debug_tag(DEBUG_TAG, 6, _T("SSH.CheckCommandMode: pattern matched on %s:%u"), hostName, port);
                  break;
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG, 8, _T("SSH.CheckCommandMode: pattern did not match line \"%s\" on %s:%u"), line, hostName, port);
               }
            }
            _pcre_free_t(preg);
            if (result == 0)
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("SSH.CheckCommandMode: pattern \"%s\" did not match any output line"), pattern);
            }
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("SSH.CheckCommandMode: failed to compile pattern \"%s\""), pattern);
         }
         delete output;
         ReleaseSession(ssh);
         break;
      }

      // Execute failed - invalidate session and retry once
      nxlog_debug_tag(DEBUG_TAG, 6, _T("SSH.CheckCommandMode: command execution failed on %s:%u%s"),
                      hostName, port, (attempt == 0) ? _T(", retrying with new session") : _T(" (command channel may not be supported)"));
      ReleaseSession(ssh, true);
   }

   MemFree(command);
   ret_int(value, result);
   return SYSINFO_RC_SUCCESS;
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
 * This handles devices that echo each character on a new line
 */
static void CollapseCharacterByCharacterOutput(char *buffer)
{
   char *src = buffer;
   char *dst = buffer;

   while (*src != 0)
   {
      // Check if this looks like char-by-char output: single ASCII printable char followed by CR or LF
      if (IsAsciiPrintable((unsigned char)*src) && (*(src + 1) == '\r' || *(src + 1) == '\n'))
      {
         // Check if next non-CRLF char is also a single char followed by CRLF
         // This indicates char-by-char mode
         char *next = src + 1;
         while (*next == '\r' || *next == '\n')
            next++;

         if (*next != 0 && IsAsciiPrintable((unsigned char)*next) && (*(next + 1) == '\r' || *(next + 1) == '\n'))
         {
            // Looks like char-by-char mode - copy char without trailing CRLF
            *dst++ = *src++;
            // Skip the CRLF between chars
            while (*src == '\r' || *src == '\n')
               src++;
            continue;
         }
      }

      // Normal character (including CR/LF) - preserve as-is
      *dst++ = *src++;
   }
   *dst = 0;
}

/**
 * Skip CSI sequence parameters and final byte
 * src should point to first byte after CSI introducer (ESC[ or 0x9B)
 * Returns pointer to byte after the sequence
 */
static char *SkipCSISequence(char *src)
{
   // Skip parameter bytes (0x30-0x3F) and intermediate bytes (0x20-0x2F)
   while (*src != 0 && (unsigned char)*src >= 0x20 && (unsigned char)*src < 0x40)
      src++;
   // Skip final byte (0x40-0x7E)
   if (*src != 0 && (unsigned char)*src >= 0x40 && (unsigned char)*src <= 0x7E)
      src++;
   return src;
}

/**
 * Skip OSC sequence content and terminator
 * src should point to first byte after OSC introducer (ESC] or 0x9D)
 * Returns pointer to byte after the sequence
 */
static char *SkipOSCSequence(char *src)
{
   while (*src != 0)
   {
      if (*src == 0x07)  // BEL terminates OSC
      {
         src++;
         break;
      }
      if (*src == 0x1B && *(src + 1) == '\\')  // ESC \ (ST) terminates OSC
      {
         src += 2;
         break;
      }
      if ((unsigned char)*src == 0x9C)  // 8-bit ST terminates OSC
      {
         src++;
         break;
      }
      src++;
   }
   return src;
}

/**
 * Remove control characters from buffer, including ANSI escape sequences
 */
static int RemoveControlCharacters(char *buffer)
{
   int outLen = 0;
   char *src = buffer;
   char *dst = buffer;

   while (*src != 0)
   {
      unsigned char c = (unsigned char)*src;

      if (c == 0x1B)  // ESC - start of 7-bit escape sequence
      {
         src++;
         if (*src == 0)
            break;

         if (*src == '[')  // CSI sequence: ESC [
         {
            src = SkipCSISequence(src + 1);
         }
         else if (*src == ']')  // OSC sequence: ESC ]
         {
            src = SkipOSCSequence(src + 1);
         }
         else if (*src == '(' || *src == ')' || *src == '*' || *src == '+')  // Character set designation
         {
            src++;
            if (*src != 0)
               src++;
         }
         else if (*src == '#')  // DEC line attributes: ESC # <digit>
         {
            src++;
            if (*src != 0)
               src++;
         }
         else if (*src >= 0x40 && *src <= 0x5F)  // Two-byte Fe sequences (ESC + 0x40-0x5F)
         {
            src++;
         }
         else if (*src >= 0x20 && *src <= 0x2F)  // nF escape sequences
         {
            // Skip intermediate bytes
            while (*src != 0 && (unsigned char)*src >= 0x20 && (unsigned char)*src <= 0x2F)
               src++;
            // Skip final byte
            if (*src != 0 && (unsigned char)*src >= 0x30 && (unsigned char)*src <= 0x7E)
               src++;
         }
         else  // Unknown escape, skip the character after ESC
         {
            src++;
         }
      }
      else if (c == 0x9B)  // 8-bit CSI
      {
         src = SkipCSISequence(src + 1);
      }
      else if (c == 0x9D)  // 8-bit OSC
      {
         src = SkipOSCSequence(src + 1);
      }
      else if (c >= 0x80 && c <= 0x9F)  // Other C1 control codes - skip
      {
         src++;
      }
      else if (c >= 32 || c == '\n' || c == '\r' || c == '\t')
      {
         // Printable characters and common whitespace
         *dst++ = *src++;
         outLen++;
      }
      else
      {
         // Skip other control characters (C0: 0x00-0x1F except handled above)
         src++;
      }
   }

   *dst = 0;
   return outLen;
}

/**
 * Respond to terminal queries in the buffer
 * Detects ESC[6n (cursor position) and ESC Z (identify) and sends appropriate responses
 * Returns true if any response was sent
 */
static bool RespondToTerminalQueries(ssh_channel channel, const char *buffer, int len)
{
   bool responded = false;

   for (int i = 0; i < len - 1; i++)
   {
      if (buffer[i] == 0x1B)  // ESC
      {
         // Check for ESC[6n (Device Status Report - cursor position query)
         if (i + 3 < len && buffer[i + 1] == '[' && buffer[i + 2] == '6' && buffer[i + 3] == 'n')
         {
            // Respond with cursor at row 1, column 1
            const char *response = "\x1B[1;1R";
            ssh_channel_write(channel, response, strlen(response));
            nxlog_debug_tag(DEBUG_TAG, 7, _T("SSH.CheckShellChannel: responded to cursor position query (ESC[6n)"));
            responded = true;
         }
         // Check for ESC Z (Identify Terminal)
         else if (buffer[i + 1] == 'Z')
         {
            // Respond as VT100: ESC[?1;0c (VT100 with no options)
            const char *response = "\x1B[?1;0c";
            ssh_channel_write(channel, response, strlen(response));
            nxlog_debug_tag(DEBUG_TAG, 7, _T("SSH.CheckShellChannel: responded to terminal identify query (ESC Z)"));
            responded = true;
         }
         // Check for ESC[c or ESC[0c (Device Attributes query)
         else if (i + 2 < len && buffer[i + 1] == '[' && (buffer[i + 2] == 'c' || (buffer[i + 2] == '0' && i + 3 < len && buffer[i + 3] == 'c')))
         {
            // Respond as VT100
            const char *response = "\x1B[?1;0c";
            ssh_channel_write(channel, response, strlen(response));
            nxlog_debug_tag(DEBUG_TAG, 7, _T("SSH.CheckShellChannel: responded to device attributes query (ESC[c)"));
            responded = true;
         }
      }
   }

   return responded;
}

/**
 * SSH shell channel check handler
 * Parameters: host[:port], login, password, keyId, promptPattern, terminalType
 * Opens PTY+shell and matches initial output against regex pattern
 * Returns: 1 (success - prompt pattern matched) or 0 (failure)
 */
LONG H_SSHCheckShellChannel(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR hostName[256], login[64], password[64];
   if (!AgentGetParameterArg(param, 1, hostName, 256) ||
       !AgentGetParameterArg(param, 2, login, 64) ||
       !AgentGetParameterArg(param, 3, password, 64))
   {
      return SYSINFO_RC_UNSUPPORTED;
   }

   uint16_t port = 22;
   TCHAR *p = _tcschr(hostName, _T(':'));
   if (p != nullptr)
   {
      *p = 0;
      p++;
      port = static_cast<uint16_t>(_tcstoul(p, nullptr, 10));
   }

   InetAddress addr = InetAddress::resolveHostName(hostName);
   if (!addr.isValidUnicast())
   {
      ret_int(value, 0);
      return SYSINFO_RC_SUCCESS;
   }

   // Get optional key ID (4th argument)
   uint32_t keyId = 0;
   AgentGetMetricArgAsUInt32(param, 4, &keyId);
   shared_ptr<KeyPair> keys;
   if (keyId != 0)
      keys = GetSshKey(session, keyId);

   // Get prompt pattern (5th argument)
   TCHAR pattern[256];
   if (!AgentGetParameterArg(param, 5, pattern, 256) || pattern[0] == 0)
   {
      _tcscpy(pattern, _T("[>$#]\\s*$"));  // Default: common prompt endings
   }

   // Get optional terminal type (6th argument)
   char terminalType[32];
   if (!AgentGetMetricArgA(param, 6, terminalType, 32) || terminalType[0] == 0)
   {
      strcpy(terminalType, "vt100");
   }

   int result = 0;

   // Always create new session for interactive channel test because some devices
   // (like Cisco) don't support multiple channels per session
   SSHSession *ssh = AcquireSession(addr, port, login, password, keys, true);
   if (ssh != nullptr)
   {
      ssh_channel channel = ssh->openInteractiveChannel(terminalType);
      if (channel != nullptr)
      {
         // Compile prompt pattern
         const char *errptr;
         int erroffset;
         PCRE *preg = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(pattern), PCRE_COMMON_FLAGS, &errptr, &erroffset, nullptr);
         if (preg != nullptr)
         {
            // Read initial output with timeout (wait for prompt)
            char buffer[4096];
            StringBuffer output;
            int totalBytesRead = 0;
            int64_t startTime = GetCurrentTimeMs();

            while ((GetCurrentTimeMs() - startTime) < 2000)
            {
               int nbytes = ssh_channel_read_timeout(channel, buffer, sizeof(buffer) - 1, 0, 1000);
               if (nbytes > 0)
               {
                  buffer[nbytes] = 0;

                  // Respond to terminal queries before processing
                  RespondToTerminalQueries(channel, buffer, nbytes);

                  // Remove control characters and collapse char-by-char output
                  RemoveControlCharacters(buffer);
                  CollapseCharacterByCharacterOutput(buffer);
                  nbytes = static_cast<int>(strlen(buffer));
                  totalBytesRead += nbytes;
#ifdef UNICODE
                  WCHAR wbuffer[4096];
                  mb_to_wchar(buffer, -1, wbuffer, 4096);
                  output.append(wbuffer);
#else
                  output.append(buffer);
#endif

                  // Check if output ends with prompt pattern
                  // Get the last line from buffer
                  const TCHAR *lastNewline = _tcsrchr(output.cstr(), _T('\n'));
                  const TCHAR *lastLine = (lastNewline != nullptr) ? lastNewline + 1 : output.cstr();

                  int ovector[30];
                  int rc = _pcre_exec_t(preg, nullptr, reinterpret_cast<const PCRE_TCHAR*>(lastLine),
                                        static_cast<int>(_tcslen(lastLine)), 0, 0, ovector, 30);
                  if (rc >= 0)
                  {
                     result = 1;
                     nxlog_debug_tag(DEBUG_TAG, 6, _T("SSH.CheckShellChannel: prompt pattern matched on %s:%u"), hostName, port);
                     break;
                  }
                  nxlog_debug_tag(DEBUG_TAG, 8, _T("SSH.CheckShellChannel: prompt pattern did not match line \"%s\" on %s:%u"), lastLine, hostName, port);
               }
               else if (nbytes == 0)
               {
                  // No data available, check if we have enough to test
                  if (totalBytesRead > 0)
                  {
                     // Try matching current output
                     const TCHAR *lastNewline = _tcsrchr(output.cstr(), _T('\n'));
                     const TCHAR *lastLine = (lastNewline != nullptr) ? lastNewline + 1 : output.cstr();

                     int ovector[30];
                     int rc = _pcre_exec_t(preg, nullptr, reinterpret_cast<const PCRE_TCHAR*>(lastLine),
                                           static_cast<int>(_tcslen(lastLine)), 0, 0, ovector, 30);
                     if (rc >= 0)
                     {
                        result = 1;
                        nxlog_debug_tag(DEBUG_TAG, 6, _T("SSH.CheckShellChannel: prompt pattern matched on %s:%u (after wait)"), hostName, port);
                        break;
                     }
                  }
               }
               else if (nbytes == SSH_ERROR)
               {
                  nxlog_debug_tag(DEBUG_TAG, 6, _T("SSH.CheckShellChannel: read error on %s:%u"), hostName, port);
                  break;
               }

               if (ssh_channel_is_eof(channel) || ssh_channel_is_closed(channel))
               {
                  nxlog_debug_tag(DEBUG_TAG, 6, _T("SSH.CheckShellChannel: channel closed on %s:%u"), hostName, port);
                  break;
               }
            }

            if (result == 0)
            {
               // Remove empty lines and spaces from the end of output and retry full output match
               // Some devices may add new lines after prompt
               output.trim();
               int ovector[30];
               int rc = _pcre_exec_t(preg, nullptr, reinterpret_cast<const PCRE_TCHAR*>(output.cstr()),
                                     static_cast<int>(output.length()), 0, 0, ovector, 30);
               if (rc >= 0)
               {
                  result = 1;
                  nxlog_debug_tag(DEBUG_TAG, 6, _T("SSH.CheckShellChannel: prompt pattern matched on %s:%u (on full output after trim)"), hostName, port);
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG, 4, _T("SSH.CheckShellChannel: prompt pattern \"%s\" did not match output on %s:%u"), pattern, hostName, port);
                  nxlog_debug_tag(DEBUG_TAG, 8, _T("SSH.CheckShellChannel: full output:\n%s"), output.cstr());
               }
            }
            _pcre_free_t(preg);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("SSH.CheckShellChannel: failed to compile pattern \"%s\""), pattern);
         }

         ssh_channel_close(channel);
         ssh_channel_free(channel);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("SSH.CheckShellChannel: failed to open interactive channel on %s:%u"), hostName, port);
      }
      ReleaseSession(ssh);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("SSH.CheckShellChannel: failed to create SSH connection to %s:%u"), hostName, port);
   }

   ret_int(value, result);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Generic handler to execute command on any host
 */
LONG H_SSHCommand(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   size_t commandBufferLen = _tcslen(param);
   TCHAR *command = MemAllocString(commandBufferLen);
   TCHAR hostName[256], login[64], password[64];
   if (!AgentGetMetricArg(param, 1, hostName, 256) ||
       !AgentGetMetricArg(param, 2, login, 64) ||
       !AgentGetMetricArg(param, 3, password, 64) ||
       !AgentGetMetricArg(param, 4, command, commandBufferLen))
   {
      MemFree(command);
      return SYSINFO_RC_UNSUPPORTED;
   }

   uint16_t port = 22;
   TCHAR *p = _tcschr(hostName, _T(':'));
   if (p != nullptr)
   {
      *p = 0;
      p++;
      port = static_cast<uint16_t>(_tcstoul(p, nullptr, 10));
   }

   InetAddress addr = InetAddress::resolveHostName(hostName);
   if (!addr.isValidUnicast())
   {
      MemFree(command);
      return SYSINFO_RC_NO_SUCH_INSTANCE;
   }

   shared_ptr<KeyPair> keys;
   uint32_t keyId;
   if (AgentGetMetricArgAsUInt32(param, 6, &keyId))
   {
      keys = GetSshKey(session, keyId);
   }

   LONG rc = SYSINFO_RC_ERROR;
   for (int attempt = 0; attempt < 2; attempt++)
   {
      SSHSession *ssh = AcquireSession(addr, port, login, password, keys);
      if (ssh == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("Failed to create SSH connection to %s:%u"), hostName, port);
         break;
      }

      StringList *output = ssh->execute(command);
      if (output != nullptr)
      {
         if (output->size() > 0)
         {
            TCHAR pattern[256] = _T("");
            AgentGetParameterArg(param, 5, pattern, 256);
            if (pattern[0] != 0)
            {
               bool match = false;
               const char *errptr;
               int erroffset;
               PCRE *preg = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(pattern), PCRE_COMMON_FLAGS, &errptr, &erroffset, nullptr);
               if (preg != nullptr)
               {
                  int ovector[60];
                  for(int i = 0; i < output->size(); i++)
                  {
                     const TCHAR *line = output->get(i);
                     int cgcount = _pcre_exec_t(preg, NULL, reinterpret_cast<const PCRE_TCHAR*>(line), static_cast<int>(_tcslen(line)), 0, 0, ovector, 60);
                     if (cgcount >= 0) // MATCH
                     {
                        match = true;
                        if ((cgcount > 1) && (ovector[2] != -1))
                        {
                           int len = ovector[3] - ovector[2];
                           _tcslcpy(value, &line[ovector[2]], std::min(MAX_RESULT_LENGTH, len + 1));
                        }
                        else
                        {
                           ret_string(value, line);
                        }
                        break;
                     }
                  }
                  _pcre_free_t(preg);
               }
               if (!match)
                  ret_string(value, _T(""));
            }
            else
            {
               ret_string(value, output->get(0));
            }
            rc = SYSINFO_RC_SUCCESS;
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 6, _T("SSH output is empty"));
         }
         delete output;
         ReleaseSession(ssh);
         break;
      }

      // Execute failed - invalidate session and retry once
      nxlog_debug_tag(DEBUG_TAG, 6, _T("SSH command execution on %s failed%s"), hostName,
                      (attempt == 0) ? _T(", retrying with new session") : _T(""));
      ReleaseSession(ssh, true);
   }
   MemFree(command);
   return rc;
}

/**
 * Generic handler to execute command on any host (arguments as list
 */
LONG H_SSHCommandList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   size_t commandBufferLen = _tcslen(param);
   TCHAR *command = MemAllocString(commandBufferLen);
   TCHAR hostName[256], login[64], password[64];
   if (!AgentGetParameterArg(param, 1, hostName, 256) ||
       !AgentGetParameterArg(param, 2, login, 64) ||
       !AgentGetParameterArg(param, 3, password, 64) ||
       !AgentGetParameterArg(param, 4, command, commandBufferLen))
   {
      MemFree(command);
      return SYSINFO_RC_UNSUPPORTED;
   }

   uint16_t port = 22;
   TCHAR *p = _tcschr(hostName, _T(':'));
   if (p != nullptr)
   {
      *p = 0;
      p++;
      port = static_cast<uint16_t>(_tcstoul(p, nullptr, 10));
   }

   InetAddress addr = InetAddress::resolveHostName(hostName);
   if (!addr.isValidUnicast())
   {
      MemFree(command);
      return SYSINFO_RC_NO_SUCH_INSTANCE;
   }

   shared_ptr<KeyPair> key;
   TCHAR keyId[16] = _T("");
   AgentGetParameterArg(param, 5, keyId, 16);
   if (keyId[0] != 0)
   {
      TCHAR *end;
      uint32_t id = _tcstoul(keyId, &end, 0);
      key = GetSshKey(session, id);
   }

   LONG rc = SYSINFO_RC_ERROR;
   for (int attempt = 0; attempt < 2; attempt++)
   {
      SSHSession *ssh = AcquireSession(addr, port, login, password, key);
      if (ssh == nullptr)
         break;

      StringList *output = ssh->execute(command);
      if (output != nullptr)
      {
         value->addAll(output);
         rc = SYSINFO_RC_SUCCESS;
         delete output;
         ReleaseSession(ssh);
         break;
      }

      // Execute failed - invalidate session and retry once
      ReleaseSession(ssh, true);
   }
   MemFree(command);
   return rc;
}

/**
 * Generic handler to execute command on host by SSH
 */
uint32_t H_SSHCommandAction(const shared_ptr<ActionExecutionContext>& context)
{
   if (context->getArgCount() < 6)
      return ERR_MALFORMED_COMMAND;

   InetAddress addr = InetAddress::resolveHostName(context->getArg(0));
   if (!addr.isValidUnicast())
      return ERR_BAD_ARGUMENTS;

   uint16_t port = (uint16_t)_tcstoul(context->getArg(1), nullptr, 10);
   uint32_t id = _tcstoul(context->getArg(5), nullptr, 10);
   shared_ptr<KeyPair> key = GetSshKey(context->getSession().get(), id);

   uint32_t rc = ERR_EXEC_FAILED;
   for (int attempt = 0; attempt < 2; attempt++)
   {
      SSHSession *ssh = AcquireSession(addr, port, context->getArg(2), context->getArg(3), key);
      if (ssh == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("Failed to create SSH connection to %s:%u"), context->getArg(0), port);
         break;
      }

      if (ssh->execute(context->getArg(4), context))
      {
         rc = ERR_SUCCESS;
         nxlog_debug_tag(DEBUG_TAG, 6, _T("SSH command execution on %s successful"), context->getArg(0));
         context->sendEndOfOutputMarker();
         ReleaseSession(ssh);
         break;
      }

      // Execute failed - invalidate session and retry once
      nxlog_debug_tag(DEBUG_TAG, 6, _T("SSH command execution on %s failed%s"), context->getArg(0),
                      (attempt == 0) ? _T(", retrying with new session") : _T(""));
      ReleaseSession(ssh, true);
   }
   return rc;
}

/**
 * Execute SSH command
 */
void ExecuteSSHCommand(const NXCPMessage& request, NXCPMessage *response, AbstractCommSession *session)
{
   InetAddress addr = request.getFieldAsInetAddress(VID_IP_ADDRESS);
   uint16_t port = request.getFieldAsUInt16(VID_PORT);
   if (port == 0)
      port = SSH_PORT;

   TCHAR user[256], password[256];
   request.getFieldAsString(VID_USER_NAME, user, 256);
   request.getFieldAsString(VID_PASSWORD, password, 256);

   uint32_t keyId = request.getFieldAsUInt32(VID_SSH_KEY_ID);
   shared_ptr<KeyPair> keys;
   if (keyId != 0)
   {
      keys = GetSshKey(session, keyId);
   }

   char command[4096];
   request.getFieldAsUtf8String(VID_COMMAND, command, 4096);

   TCHAR ipAddrText[64];
   nxlog_debug_tag(DEBUG_TAG, 5, _T("ExecuteSSHCommand: executing \"%hs\" on %s:%u"), command, addr.toString(ipAddrText), port);

   for (int attempt = 0; attempt < 2; attempt++)
   {
      SSHSession *sshSession = AcquireSession(addr, port, user, password, keys);
      if (sshSession == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("ExecuteSSHCommand: cannot create SSH session to %s:%u"), addr.toString(ipAddrText), port);
         response->setField(VID_RCC, ERR_REMOTE_CONNECT_FAILED);
         return;
      }

      ByteStream output;
      if (sshSession->execute(command, &output))
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("ExecuteSSHCommand: command executed successfully on %s:%u, %d bytes of output"),
            addr.toString(ipAddrText), port, static_cast<int>(output.size()));
         response->setField(VID_RCC, ERR_SUCCESS);
         response->setField(VID_OUTPUT, output.buffer(), output.size());
         ReleaseSession(sshSession);
         return;
      }

      // Execute failed - invalidate session and retry once
      nxlog_debug_tag(DEBUG_TAG, 5, _T("ExecuteSSHCommand: command execution failed on %s:%u%s"),
                      addr.toString(ipAddrText), port, (attempt == 0) ? _T(", retrying with new session") : _T(""));
      ReleaseSession(sshSession, true);
   }

   response->setField(VID_RCC, ERR_EXEC_FAILED);
}
