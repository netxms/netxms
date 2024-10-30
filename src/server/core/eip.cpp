/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
** File: eip.cpp
**/

#include "nxcore.h"
#include <ethernet_ip.h>

/**
 * Get attribute from device
 */
DataCollectionError GetEtherNetIPAttribute(const InetAddress& addr, uint16_t port, const TCHAR *symbolicPath, uint32_t timeout, TCHAR *buffer, size_t size)
{
   uint32_t classId, instance, attributeId;
   if (!CIP_ParseSymbolicPath(symbolicPath, &classId, &instance, &attributeId))
      return DCE_NOT_SUPPORTED;

   SOCKET s = ConnectToHost(addr, port, timeout);
   if (s == INVALID_SOCKET)
   {
      TCHAR errorText[256];
      nxlog_debug_tag(DEBUG_TAG_DC_EIP, 6, _T("GetEtherNetIPAttribute(%s:%d, %s): cannot establish TCP connection (%s)"),
         addr.toString().cstr(), port, symbolicPath, GetLastSocketErrorText(errorText, 256));
      return DCE_COMM_ERROR;
   }

   CIP_EPATH path;
   CIP_EncodeAttributePath(classId, instance, attributeId, &path);

   EIP_Status status;
   unique_ptr<EIP_Session> session(EIP_Session::connect(s, timeout, &status));
   if (session == nullptr)
   {
      // Connect will close socket on failure
      nxlog_debug_tag(DEBUG_TAG_DC_EIP, 6, _T("GetEtherNetIPAttribute(%s:%d, %s): session registration failed (%s)"),
         addr.toString().cstr(), port, symbolicPath, status.failureReason().cstr());
      return DCE_COMM_ERROR;
   }

   BYTE rawData[1024];
   size_t rawDataSize = sizeof(rawData);
   status = session->getAttribute(path, rawData, &rawDataSize);
   if (!status.isSuccess())
   {
      nxlog_debug_tag(DEBUG_TAG_DC_EIP, 6, _T("GetEtherNetIPAttribute(%s:%d, %s): request error (%s)"),
         addr.toString().cstr(), port, symbolicPath, status.failureReason().cstr());
      switch (status.callStatus)
      {
         case EIP_CALL_CONNECT_FAILED:
         case EIP_CALL_COMM_ERROR:
         case EIP_CALL_TIMEOUT:
            return DCE_COMM_ERROR;
         case EIP_INVALID_EPATH:
            return DCE_NOT_SUPPORTED;
         default:
            return DCE_COLLECTION_ERROR;
      }
   }

   if (status.cipGeneralStatus != 0)
   {
      nxlog_debug_tag(DEBUG_TAG_DC_EIP, 6, _T("GetEtherNetIPAttribute(%s:%d, %s): CIP General Status: %02X (%s)"),
         addr.toString().cstr(), port, symbolicPath, status.cipGeneralStatus, CIP_GeneralStatusTextFromCode(status.cipGeneralStatus));
      if ((status.cipGeneralStatus == 0x04) || (status.cipGeneralStatus == 0x05) || (status.cipGeneralStatus == 0x08) || (status.cipGeneralStatus == 0x14) || (status.cipGeneralStatus == 0x26))
         return DCE_NOT_SUPPORTED;
      if (status.cipGeneralStatus == 0x16)
         return DCE_NO_SUCH_INSTANCE;
      return DCE_COLLECTION_ERROR;
   }

   CIP_DecodeAttribute(rawData, rawDataSize, classId, attributeId, buffer, size);
   return DCE_SUCCESS;
}
