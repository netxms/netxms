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
 * Parse value from key=value list entry
 */
static const TCHAR *GetListValue(const StringList *list, const TCHAR *key)
{
   size_t keyLen = _tcslen(key);
   for (int i = 0; i < list->size(); i++)
   {
      const TCHAR *entry = list->get(i);
      if (!_tcsncmp(entry, key, keyLen) && (entry[keyLen] == _T('=')))
         return entry + keyLen + 1;
   }
   return nullptr;
}

/**
 * Parse CIP_Identity from agent list response
 */
CIP_Identity *ParseCIPIdentityFromList(const StringList *list)
{
   const TCHAR *productName = GetListValue(list, _T("productName"));
   if (productName == nullptr)
      productName = _T("");

   auto identity = static_cast<CIP_Identity*>(MemAlloc(sizeof(CIP_Identity) + (_tcslen(productName) + 1) * sizeof(TCHAR)));
   identity->productName = reinterpret_cast<TCHAR*>(reinterpret_cast<uint8_t*>(identity) + sizeof(CIP_Identity));
   _tcscpy(identity->productName, productName);

   const TCHAR *v;

   v = GetListValue(list, _T("vendor"));
   identity->vendor = (v != nullptr) ? static_cast<uint16_t>(_tcstoul(v, nullptr, 10)) : 0;

   v = GetListValue(list, _T("deviceType"));
   identity->deviceType = (v != nullptr) ? static_cast<uint16_t>(_tcstoul(v, nullptr, 10)) : 0;

   v = GetListValue(list, _T("productCode"));
   identity->productCode = (v != nullptr) ? static_cast<uint16_t>(_tcstoul(v, nullptr, 10)) : 0;

   v = GetListValue(list, _T("productRevisionMajor"));
   identity->productRevisionMajor = (v != nullptr) ? static_cast<uint8_t>(_tcstoul(v, nullptr, 10)) : 0;

   v = GetListValue(list, _T("productRevisionMinor"));
   identity->productRevisionMinor = (v != nullptr) ? static_cast<uint8_t>(_tcstoul(v, nullptr, 10)) : 0;

   v = GetListValue(list, _T("serialNumber"));
   identity->serialNumber = (v != nullptr) ? _tcstoul(v, nullptr, 10) : 0;

   v = GetListValue(list, _T("ipAddress"));
   identity->ipAddress = (v != nullptr) ? InetAddress::parse(v) : InetAddress();

   v = GetListValue(list, _T("tcpPort"));
   identity->tcpPort = (v != nullptr) ? static_cast<uint16_t>(_tcstoul(v, nullptr, 10)) : 0;

   v = GetListValue(list, _T("protocolVersion"));
   identity->protocolVersion = (v != nullptr) ? static_cast<uint16_t>(_tcstoul(v, nullptr, 10)) : 0;

   v = GetListValue(list, _T("status"));
   identity->status = (v != nullptr) ? static_cast<uint16_t>(_tcstoul(v, nullptr, 10)) : 0;

   v = GetListValue(list, _T("state"));
   identity->state = (v != nullptr) ? static_cast<uint8_t>(_tcstoul(v, nullptr, 10)) : 0;

   return identity;
}

/**
 * Get EtherNet/IP identity via proxy agent
 */
CIP_Identity *EIP_ListIdentityViaProxy(const shared_ptr<AgentConnectionEx>& conn, const InetAddress& addr, uint16_t port, EIP_Status *status)
{
   TCHAR metric[256], ipAddrText[64];
   _sntprintf(metric, 256, _T("EtherNetIP.ListIdentity(%s,%u)"), addr.toString(ipAddrText), port);

   StringList *list;
   uint32_t rcc = conn->getList(metric, &list);
   if (rcc != ERR_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG_DC_EIP, 5, _T("EIP_ListIdentityViaProxy(%s:%d): agent request failed (error %u)"), ipAddrText, port, rcc);
      *status = EIP_Status::callFailure((rcc == ERR_REQUEST_TIMEOUT) ? EIP_CALL_TIMEOUT : EIP_CALL_COMM_ERROR);
      return nullptr;
   }

   CIP_Identity *identity = ParseCIPIdentityFromList(list);
   delete list;

   *status = EIP_Status::success();
   nxlog_debug_tag(DEBUG_TAG_DC_EIP, 5, _T("EIP_ListIdentityViaProxy(%s:%d): identity retrieved successfully"), ipAddrText, port);
   return identity;
}

/**
 * Get EtherNet/IP attribute via proxy agent
 */
DataCollectionError GetEtherNetIPAttributeViaProxy(const shared_ptr<AgentConnectionEx>& conn, const InetAddress& addr, uint16_t port, const TCHAR *symbolicPath, TCHAR *buffer, size_t size)
{
   TCHAR metric[512], ipAddrText[64];
   _sntprintf(metric, 512, _T("EtherNetIP.Attribute(%s,%u,%s)"), addr.toString(ipAddrText), port, symbolicPath);

   TCHAR result[MAX_RESULT_LENGTH];
   uint32_t rcc = conn->getParameter(metric, result, MAX_RESULT_LENGTH);
   if (rcc != ERR_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG_DC_EIP, 5, _T("GetEtherNetIPAttributeViaProxy(%s:%d, %s): agent request failed (error %u)"), ipAddrText, port, symbolicPath, rcc);
      return (rcc == ERR_REQUEST_TIMEOUT) ? DCE_COMM_ERROR : DCE_COMM_ERROR;
   }

   _tcslcpy(buffer, result, size);
   return DCE_SUCCESS;
}

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
