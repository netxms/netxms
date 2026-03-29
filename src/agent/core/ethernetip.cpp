/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2026 Raden Solutions
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
** File: ethernetip.cpp
**
**/

#include "nxagentd.h"
#include <ethernet_ip.h>

#define DEBUG_TAG _T("ethernetip.proxy")

/**
 * Handler for EtherNetIP.Attribute(*) metric
 */
LONG H_EtherNetIPAttribute(const TCHAR *metric, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   if (!(g_dwFlags & AF_ENABLE_ETHERNET_IP_PROXY))
      return SYSINFO_RC_ACCESS_DENIED;

   InetAddress addr = AgentGetMetricArgAsInetAddress(metric, 1);
   if (!addr.isValidUnicast() && !addr.isLoopback())
      return SYSINFO_RC_UNSUPPORTED;

   uint16_t port;
   if (!AgentGetMetricArgAsUInt16(metric, 2, &port, ETHERNET_IP_DEFAULT_PORT))
      return SYSINFO_RC_UNSUPPORTED;

   TCHAR symbolicPath[256];
   if (!AgentGetMetricArg(metric, 3, symbolicPath, 256) || (symbolicPath[0] == 0))
      return SYSINFO_RC_UNSUPPORTED;

   uint32_t classId, instance, attributeId;
   if (!CIP_ParseSymbolicPath(symbolicPath, &classId, &instance, &attributeId))
      return SYSINFO_RC_UNSUPPORTED;

   SOCKET s = ConnectToHost(addr, port, 5000);
   if (s == INVALID_SOCKET)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("H_EtherNetIPAttribute(%s:%d, %s): cannot establish TCP connection"),
         addr.toString().cstr(), port, symbolicPath);
      return SYSINFO_RC_ERROR;
   }

   EIP_Status status;
   unique_ptr<EIP_Session> eipSession(EIP_Session::connect(s, 5000, &status));
   if (eipSession == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("H_EtherNetIPAttribute(%s:%d, %s): session registration failed (%s)"),
         addr.toString().cstr(), port, symbolicPath, status.failureReason().cstr());
      return SYSINFO_RC_ERROR;
   }

   CIP_EPATH path;
   CIP_EncodeAttributePath(classId, instance, attributeId, &path);

   BYTE rawData[1024];
   size_t rawDataSize = sizeof(rawData);
   status = eipSession->getAttribute(path, rawData, &rawDataSize);
   if (!status.isSuccess())
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("H_EtherNetIPAttribute(%s:%d, %s): request error (%s)"),
         addr.toString().cstr(), port, symbolicPath, status.failureReason().cstr());
      return SYSINFO_RC_ERROR;
   }

   if (status.cipGeneralStatus != 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("H_EtherNetIPAttribute(%s:%d, %s): CIP General Status: %02X (%s)"),
         addr.toString().cstr(), port, symbolicPath, status.cipGeneralStatus, CIP_GeneralStatusTextFromCode(status.cipGeneralStatus));
      return SYSINFO_RC_ERROR;
   }

   CIP_DecodeAttribute(rawData, rawDataSize, classId, attributeId, value, MAX_RESULT_LENGTH);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for EtherNetIP.ListIdentity(*) list
 */
LONG H_EtherNetIPListIdentity(const TCHAR *metric, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   if (!(g_dwFlags & AF_ENABLE_ETHERNET_IP_PROXY))
      return SYSINFO_RC_ACCESS_DENIED;

   InetAddress addr = AgentGetMetricArgAsInetAddress(metric, 1);
   if (!addr.isValidUnicast() && !addr.isLoopback())
      return SYSINFO_RC_UNSUPPORTED;

   uint16_t port;
   if (!AgentGetMetricArgAsUInt16(metric, 2, &port, ETHERNET_IP_DEFAULT_PORT))
      return SYSINFO_RC_UNSUPPORTED;

   EIP_Status status;
   CIP_Identity *identity = EIP_ListIdentity(addr, port, 5000, &status);
   if (identity == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("H_EtherNetIPListIdentity(%s:%d): call failed (%s)"),
         addr.toString().cstr(), port, status.failureReason().cstr());
      return SYSINFO_RC_ERROR;
   }

   TCHAR buffer[256];

   _sntprintf(buffer, 256, _T("vendor=%u"), identity->vendor);
   value->add(buffer);

   _sntprintf(buffer, 256, _T("deviceType=%u"), identity->deviceType);
   value->add(buffer);

   _sntprintf(buffer, 256, _T("productCode=%u"), identity->productCode);
   value->add(buffer);

   _sntprintf(buffer, 256, _T("productRevisionMajor=%u"), static_cast<unsigned int>(identity->productRevisionMajor));
   value->add(buffer);

   _sntprintf(buffer, 256, _T("productRevisionMinor=%u"), static_cast<unsigned int>(identity->productRevisionMinor));
   value->add(buffer);

   _sntprintf(buffer, 256, _T("serialNumber=%u"), identity->serialNumber);
   value->add(buffer);

   _sntprintf(buffer, 256, _T("productName=%s"), identity->productName);
   value->add(buffer);

   _sntprintf(buffer, 256, _T("ipAddress=%s"), identity->ipAddress.toString().cstr());
   value->add(buffer);

   _sntprintf(buffer, 256, _T("tcpPort=%u"), identity->tcpPort);
   value->add(buffer);

   _sntprintf(buffer, 256, _T("protocolVersion=%u"), identity->protocolVersion);
   value->add(buffer);

   _sntprintf(buffer, 256, _T("status=%u"), identity->status);
   value->add(buffer);

   _sntprintf(buffer, 256, _T("state=%u"), static_cast<unsigned int>(identity->state));
   value->add(buffer);

   MemFree(identity);
   return SYSINFO_RC_SUCCESS;
}
