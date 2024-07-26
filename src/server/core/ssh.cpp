/*
** NetXMS - Network Management System
** Copyright (C) 2022-2024 Raden Solutions
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
bool SSHCheckConnection(const shared_ptr<Node>& proxyNode, const InetAddress& addr, uint16_t port, const TCHAR *login, const TCHAR *password, uint32_t keyId)
{
   StringBuffer request(_T("SSH.CheckConnection("));
   TCHAR ipAddr[64];
   request
      .append(addr.toString(ipAddr))
      .append(_T(':'))
      .append(port)
      .append(_T(",\""))
      .append(EscapeStringForAgent(login).cstr())
      .append(_T("\",\""))
      .append(EscapeStringForAgent(password).cstr())
      .append(_T("\","))
      .append(keyId)
      .append(_T(')'));

   TCHAR response[2];
   return proxyNode->getMetricFromAgent(request, response, 2) == DCE_SUCCESS && response[0] == _T('1');
}

/**
 * Check SSH connection using given communication settings
 */
bool SSHCheckConnection(uint32_t proxyNodeId, const InetAddress& addr, uint16_t port, const TCHAR *login, const TCHAR *password, uint32_t keyId)
{
   shared_ptr<Node> proxyNode = static_pointer_cast<Node>(FindObjectById(proxyNodeId, OBJECT_NODE));
   return (proxyNode != nullptr) ? SSHCheckConnection(proxyNode, addr, port, login, password, keyId) : false;
}

/**
 * Determine SSH communication settings for node
 * On success, returns true and fills selectedCredentials and selectedPort
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
