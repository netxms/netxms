/*
** NetXMS - Network Management System
** Copyright (C) 2022-2026 Raden Solutions
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
** File: ssh.cpp
**
**/

#include "nxcore.h"

/**
 * Check SSH connection using given communication settings
 */
bool SSHCheckConnection(const shared_ptr<Node>& proxyNode, const InetAddress& addr, uint16_t port, const wchar_t *login, const wchar_t *password, uint32_t keyId)
{
   StringBuffer request(_T("SSH.CheckConnection("));
   wchar_t ipAddr[64];
   request
      .append(addr.toString(ipAddr))
      .append(L':')
      .append(port)
      .append(L",\"")
      .append(EscapeStringForAgent(login).cstr())
      .append(L"\",\"")
      .append(EscapeStringForAgent(password).cstr())
      .append(L"\",")
      .append(keyId)
      .append(L')');

   wchar_t response[64];
   if (proxyNode->getMetricFromAgent(request, response, 64) != DCE_SUCCESS)
      return false;
   return wcstol(response, nullptr, 10) != 0;
}

/**
 * Check SSH connection using given communication settings
 */
bool SSHCheckConnection(uint32_t proxyNodeId, const InetAddress& addr, uint16_t port, const wchar_t *login, const wchar_t *password, uint32_t keyId)
{
   shared_ptr<Node> proxyNode = static_pointer_cast<Node>(FindObjectById(proxyNodeId, OBJECT_NODE));
   return (proxyNode != nullptr) ? SSHCheckConnection(proxyNode, addr, port, login, password, keyId) : false;
}

/**
 * Determine SSH communication settings for node
 * On success, returns true and fills selectedCredentials, selectedPort, and sshCapabilities (if not null)
 */
bool SSHCheckCommSettings(uint32_t proxyNodeId, const InetAddress& addr, int32_t zoneUIN, SSHCredentials *selectedCredentials, uint16_t *selectedPort)
{
   TCHAR ipAddrText[64];
   addr.toString(ipAddrText);

   shared_ptr<Node> proxyNode = static_pointer_cast<Node>(FindObjectById(proxyNodeId, OBJECT_NODE));
   if (proxyNode == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_SSH, 5, _T("SSHCheckCommSettings(%s): invalid proxy node ID %u"), ipAddrText, proxyNodeId);
      return false;
   }

   IntegerArray<uint16_t> ports = GetWellKnownPorts(_T("ssh"), zoneUIN);
   StructArray<SSHCredentials> credentials = GetSSHCredentials(zoneUIN);

   bool success = false;
   for (int i = 0; i < ports.size(); i++)
   {
      uint16_t port = ports.get(i);
      for (int j = 0; j < credentials.size(); j++)
      {
         SSHCredentials *c = credentials.get(j);
         nxlog_debug_tag(DEBUG_TAG_SSH, 5, _T("SSHCheckCommSettings(%s): trying port %d login name %s"), ipAddrText, port, c->login);
         success = SSHCheckConnection(proxyNode, addr, port, c->login, c->password, c->keyId);
         if (success)
         {
            if (selectedPort != nullptr)
               *selectedPort = port;
            if (selectedCredentials != nullptr)
               memcpy(selectedCredentials, c, sizeof(SSHCredentials));
            break;
         }
      }
   }

   nxlog_debug_tag(DEBUG_TAG_SSH, 5, _T("SSHCheckCommSettings(%s): %s"), ipAddrText, success ? _T("success") : _T("failure"));
   return success;
}

/**
 * Check SSH command channel support
 * Uses driver hints testCommand/testCommandPattern if provided, else uses default
 */
bool SSHCheckCommandChannel(const shared_ptr<Node>& proxyNode, const InetAddress& addr, uint16_t port,
   const wchar_t *login, const wchar_t *password, uint32_t keyId,
   const char *testCommand, const char *testPattern)
{
   StringBuffer request(_T("SSH.CheckCommandMode("));
   wchar_t ipAddr[64];
   request
      .append(addr.toString(ipAddr))
      .append(L':')
      .append(port)
      .append(L",\"")
      .append(EscapeStringForAgent(login).cstr())
      .append(L"\",\"")
      .append(EscapeStringForAgent(password).cstr())
      .append(L"\",")
      .append(keyId)
      .append(L",\"");

   // Add test command (use driver hints or default)
   if (testCommand != nullptr && testCommand[0] != '\0')
   {
#ifdef UNICODE
      WCHAR wbuf[256];
      mb_to_wchar(testCommand, -1, wbuf, 256);
      request.append(EscapeStringForAgent(wbuf).cstr());
#else
      request.append(EscapeStringForAgent(testCommand).cstr());
#endif
   }
   else
   {
      request.append(_T("echo netxms_test_12345"));
   }

   request.append(L"\",\"");

   // Add test pattern (use driver hints or default)
   if (testPattern != nullptr && testPattern[0] != '\0')
   {
#ifdef UNICODE
      WCHAR wbuf[256];
      mb_to_wchar(testPattern, -1, wbuf, 256);
      request.append(EscapeStringForAgent(wbuf).cstr());
#else
      request.append(EscapeStringForAgent(testPattern).cstr());
#endif
   }
   else
   {
      request.append(_T("netxms_test_12345"));
   }

   request.append(L"\")");

   wchar_t response[64];
   if (proxyNode->getMetricFromAgent(request, response, 64) != DCE_SUCCESS)
      return false;
   return wcstol(response, nullptr, 10) != 0;
}

/**
 * Check SSH command channel support (by proxy node ID)
 */
bool SSHCheckCommandChannel(uint32_t proxyNodeId, const InetAddress& addr, uint16_t port,
   const wchar_t *login, const wchar_t *password, uint32_t keyId,
   const char *testCommand, const char *testPattern)
{
   shared_ptr<Node> proxyNode = static_pointer_cast<Node>(FindObjectById(proxyNodeId, OBJECT_NODE));
   return (proxyNode != nullptr) ? SSHCheckCommandChannel(proxyNode, addr, port, login, password, keyId, testCommand, testPattern) : false;
}

/**
 * Check SSH interactive channel support
 * Opens PTY+shell and matches initial output against promptPattern
 */
bool SSHCheckInteractiveChannel(const shared_ptr<Node>& proxyNode, const InetAddress& addr, uint16_t port,
   const wchar_t *login, const wchar_t *password, uint32_t keyId,
   const char *promptPattern, const char *terminalType)
{
   StringBuffer request(_T("SSH.CheckShellChannel("));
   wchar_t ipAddr[64];
   request
      .append(addr.toString(ipAddr))
      .append(L':')
      .append(port)
      .append(L",\"")
      .append(EscapeStringForAgent(login).cstr())
      .append(L"\",\"")
      .append(EscapeStringForAgent(password).cstr())
      .append(L"\",")
      .append(keyId)
      .append(L",\"");

   // Add prompt pattern
   if (promptPattern != nullptr && promptPattern[0] != '\0')
   {
#ifdef UNICODE
      WCHAR wbuf[256];
      mb_to_wchar(promptPattern, -1, wbuf, 256);
      request.append(EscapeStringForAgent(wbuf).cstr());
#else
      request.append(EscapeStringForAgent(promptPattern).cstr());
#endif
   }
   else
   {
      request.append(_T("[>$#]\\s*$"));  // Default: common prompt endings
   }

   request.append(L"\",\"");

   // Add terminal type
   if (terminalType != nullptr && terminalType[0] != '\0')
   {
#ifdef UNICODE
      WCHAR wbuf[32];
      mb_to_wchar(terminalType, -1, wbuf, 32);
      request.append(EscapeStringForAgent(wbuf).cstr());
#else
      request.append(EscapeStringForAgent(terminalType).cstr());
#endif
   }
   else
   {
      request.append(L"vt100");
   }
   request.append(L"\")");

   wchar_t response[64];
   if (proxyNode->getMetricFromAgent(request, response, 64) != DCE_SUCCESS)
      return false;
   return wcstol(response, nullptr, 10) != 0;
}

/**
 * Check SSH interactive channel support (by proxy node ID)
 */
bool SSHCheckInteractiveChannel(uint32_t proxyNodeId, const InetAddress& addr, uint16_t port,
   const wchar_t *login, const wchar_t *password, uint32_t keyId,
   const char *promptPattern, const char *terminalType)
{
   shared_ptr<Node> proxyNode = static_pointer_cast<Node>(FindObjectById(proxyNodeId, OBJECT_NODE));
   return (proxyNode != nullptr) ? SSHCheckInteractiveChannel(proxyNode, addr, port, login, password, keyId, promptPattern, terminalType) : false;
}
