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
 * unique_ptr style wrapper for socket
 */
class SocketHandle
{
private:
   SOCKET m_socket;

public:
   SocketHandle(SOCKET s)
   {
      m_socket = s;
   }

   ~SocketHandle()
   {
      if (m_socket != INVALID_SOCKET)
      {
         shutdown(m_socket, 2);
         closesocket(m_socket);
      }
   }

   operator SOCKET()
   {
      return m_socket;
   }

   bool isValid()
   {
      return m_socket != INVALID_SOCKET;
   }
};

/**
 * Read command response
 */
static EIP_Message *ReadResponse(EIP_MessageReceiver *receiver, EIP_Command command, size_t cpfStartOffset, time_t timeout)
{
   EIP_Message *response = receiver->readMessage(timeout);
   if (response == nullptr)
      return nullptr;

   if (response->getCommand() != command)
   {
      delete response;
      return nullptr;
   }

   if (response->getStatus() != EIP_STATUS_SUCCESS)
   {
      delete response;
      return nullptr;
   }

   response->prepareCPFRead(cpfStartOffset);
   return response;
}

/**
 * Get attribute from device
 */
DataCollectionError GetEtherNetIPAttribute(const InetAddress& addr, uint16_t port, const TCHAR *symbolicPath, uint32_t timeout, TCHAR *buffer, size_t size)
{
   uint32_t classId, instance, attributeId;
   if (!CIP_ParseSymbolicPath(symbolicPath, &classId, &instance, &attributeId))
      return DCE_NOT_SUPPORTED;

   SocketHandle s = ConnectToHost(addr, port, timeout);
   if (!s.isValid())
   {
      TCHAR errorText[256];
      nxlog_debug_tag(DEBUG_TAG_DC_EIP, 6, _T("GetEthernetIPAttribute(%s:%d, %s): cannot establish TCP connection (%s)"),
         addr.toString().cstr(), port, symbolicPath, GetLastSocketErrorText(errorText, 256));
      return DCE_COMM_ERROR;
   }

   CIP_EPATH path;
   CIP_EncodeAttributePath(classId, instance, attributeId, &path);

   EIP_Status status;
   unique_ptr<EIP_Session> session(EIP_Session::connect(s, timeout, &status));
   if (session == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_DC_EIP, 6, _T("GetEthernetIPAttribute(%s:%d, %s): session registration failed (%s)"),
         addr.toString().cstr(), port, symbolicPath, status.failureReason().cstr());
      return DCE_COMM_ERROR;
   }

   EIP_Message request(EIP_SEND_RR_DATA, 1024, session->getHandle());
   request.advanceWritePosition(6); // Interface ID and timeout left as 0
   request.writeDataAsUInt16(2);    // Item count
   request.advanceWritePosition(4); // Item type 0 followed by length 0 - NULL address
   request.writeDataAsUInt16(0xB2); // Type B2 - UCMM message
   request.writeDataAsUInt16(static_cast<uint16_t>(path.size + 2));
   request.writeDataAsUInt8(CIP_Get_Attribute_Single);
   request.writeDataAsUInt8(static_cast<uint8_t>(path.size / 2));
   request.writeData(path.value, path.size);
   request.completeDataWrite();

   size_t bytes = request.getSize();
   if (SendEx(s, request.getBytes(), bytes, 0, nullptr) != bytes)
   {
      TCHAR errorText[256];
      nxlog_debug_tag(DEBUG_TAG_DC_EIP, 6, _T("GetEthernetIPAttribute(%s:%d, %s): error sending request (%s)"),
         addr.toString().cstr(), port, symbolicPath, GetLastSocketErrorText(errorText, 256));
      return DCE_COMM_ERROR;
   }

   unique_ptr<EIP_MessageReceiver> receiver = make_unique<EIP_MessageReceiver>(s);
   EIP_Message *response = ReadResponse(receiver.get(), EIP_SEND_RR_DATA, 6, timeout);
   if (response == nullptr)
   {
      TCHAR errorText[256];
      nxlog_debug_tag(DEBUG_TAG_DC_EIP, 6, _T("GetEthernetIPAttribute(%s:%d, %s): error sending request (%s)"),
         addr.toString().cstr(), port, symbolicPath, GetLastSocketErrorText(errorText, 256));
      return DCE_COMM_ERROR;
   }

   DataCollectionError rc;
   CPF_Item item;
   if (response->findItem(0xB2, &item))
   {
      CIP_GeneralStatus generalStatus = response->readDataAsUInt8(item.offset + 2);
      if (generalStatus == 0)
      {
         uint16_t additionalStatusSize = response->readDataAsUInt8(item.offset + 3) * 2;
         CIP_DecodeAttribute(response->getRawData() + item.offset + additionalStatusSize + 4,
                  item.length - additionalStatusSize - 4, classId, attributeId, buffer, size);
         rc = DCE_SUCCESS;
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_DC_EIP, 6, _T("GetEthernetIPAttribute(%s:%d, %s): CIP General Status: %02X (%s)"),
            addr.toString().cstr(), port, symbolicPath, generalStatus, CIP_GeneralStatusTextFromCode(generalStatus));
         rc = DCE_COLLECTION_ERROR;
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_DC_EIP, 6, _T("GetEthernetIPAttribute(%s:%d, %s): missing UCMM message data"),
         addr.toString().cstr(), port, symbolicPath, port);
      rc = DCE_NOT_SUPPORTED;
   }

   delete response;
   return rc;
}
