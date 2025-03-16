/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
** File: vnc.cpp
**
**/

#include "nxcore.h"

/**
 * Check VNC connection using given communication settings
 */
bool VNCCheckConnection(Node *proxyNode, const InetAddress& addr, uint16_t port)
{
   StringBuffer request(_T("VNC.ServerState("));
   TCHAR ipAddr[64];
   request
      .append(addr.toString(ipAddr))
      .append(_T(','))
      .append(port)
      .append(_T(')'));

   TCHAR response[2];
   return proxyNode->getMetricFromAgent(request, response, 2) == DCE_SUCCESS && response[0] == _T('1');
}

/**
 * Check VNC connection using given communication settings
 */
bool VNCCheckConnection(uint32_t proxyNodeId, const InetAddress& addr, uint16_t port)
{
   shared_ptr<NetObj> proxyNode = FindObjectById(proxyNodeId, OBJECT_NODE);
   return (proxyNode != nullptr) ? VNCCheckConnection(static_cast<Node*>(proxyNode.get()), addr, port) : false;
}

/**
 * Determine VNC communication settings for node
 * On success, returns true and fills selectedPort
 */
bool VNCCheckCommSettings(uint32_t proxyNodeId, const InetAddress& addr, int32_t zoneUIN, uint16_t *selectedPort)
{
   TCHAR ipAddrText[64];
   addr.toString(ipAddrText);

   shared_ptr<NetObj> proxyNode = FindObjectById(proxyNodeId, OBJECT_NODE);
   if (proxyNode == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_VNC, 5, _T("VNCCheckCommSettings(%s): invalid proxy node ID %u"), ipAddrText, proxyNodeId);
      return false;
   }

   IntegerArray<uint16_t> ports = GetWellKnownPorts(_T("vnc"), zoneUIN);

   bool success = false;
   for (int i = 0; i < ports.size(); i++)
   {
      uint16_t port = ports.get(i);
      nxlog_debug_tag(DEBUG_TAG_VNC, 5, _T("VNCCheckCommSettings(%s): trying port %d"), ipAddrText, port);
      success = VNCCheckConnection(static_cast<Node*>(proxyNode.get()), addr, port);
      if (success)
      {
         if (selectedPort != nullptr)
            *selectedPort = port;
         break;
      }
   }

   nxlog_debug_tag(DEBUG_TAG_VNC, 5, _T("VNCCheckCommSettings(%s): %s"), ipAddrText, success ? _T("success") : _T("failure"));
   return success;
}
