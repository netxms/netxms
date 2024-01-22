/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Raden Solutions
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
** File: sender.cpp
**
**/

#include "leef.h"
#include <nms_users.h>

/**
 * Custom destructor for queue elements
 */
static void QueueElementDestructor(void *element, Queue *queue)
{
   MemFree(element);
}

/**
 * Sender queue
 */
static ObjectQueue<char> s_senderQueue(256, Ownership::True, QueueElementDestructor);

/**
 * Enqueue audit record
 */
static void EncodeCommonLeefFields(ByteStream *builder, const char *hostname, const char *vendor, const char *product, const char *version, time_t appTimestamp, bool isSuccess, char *text)
{
   char buffer[256];
   time_t now = time(nullptr);
   if (g_leefRFC5424Timestamp)
   {
      sprintf(buffer, "<%d>1 ", (g_leefSyslogFacility << 3) + g_leefSyslogSeverity);
      builder->write(buffer, strlen(buffer));
      strftime(buffer, 256, "%Y-%m-%dT%H:%M:%S.000Z ", gmtime(&now));
   }
   else
   {
      strftime(buffer, 256, "%b %d %H:%M:%S ", gmtime(&now));
   }
   builder->write(buffer, strlen(buffer));
   builder->write(hostname, strlen(hostname));
   builder->write(" LEEF:2.0|", 10);
   builder->write(vendor, strlen(vendor));
   builder->write('|');
   builder->write(product, strlen(product));
   builder->write('|');
   builder->write(version, strlen(version));
   builder->write('|');

   sprintf(buffer, "%u|", g_leefEventCode);
   builder->write(buffer, strlen(buffer));

   sprintf(buffer, "x%02X|devTimeFormat=yyyy-MM-dd HH:mm:ss z%cdevTime=", g_leefSeparatorChar, g_leefSeparatorChar);
   builder->write(buffer, strlen(buffer));

   strftime(buffer, 256, "%Y-%m-%d %H:%M:%S Z", gmtime((appTimestamp != 0) ? &appTimestamp : &now));
   builder->write(buffer, strlen(buffer));

   sprintf(buffer, "%cresult=%s", g_leefSeparatorChar, isSuccess ? "Success" : "Failed");
   builder->write(buffer, strlen(buffer));

   sprintf(buffer, "%cdescription=", g_leefSeparatorChar);
   builder->write(buffer, strlen(buffer));
   builder->write(text, strlen(text));
   MemFree(text);
}

/**
 * Enqueue audit record from internal NetXMS audit call
 */
void EnqueueLeefAuditRecord(const TCHAR *subsys, bool isSuccess, uint32_t userId, const TCHAR *workstation,
         session_id_t sessionId, uint32_t objectId, const TCHAR *oldValue, const TCHAR *newValue, char valueType, const TCHAR *text)
{
   ByteStream builder;

   EncodeCommonLeefFields(&builder, g_leefHostname, g_leefVendor, g_leefProduct, g_leefProductVersion, 0, isSuccess, UTF8StringFromTString(text));

   char buffer[256];
   *reinterpret_cast<BYTE*>(buffer) = g_leefSeparatorChar;

   TCHAR tbuffer[256];
   if (ResolveUserId(userId, tbuffer, false) != nullptr)
   {
      strcpy(&buffer[1], "usrName=");
      tchar_to_utf8(tbuffer, -1, &buffer[9], 247);
      builder.write(buffer, strlen(buffer));
   }

   if ((workstation != nullptr) && (*workstation != 0))
   {
      strcpy(&buffer[1], "identSrc=");
      tchar_to_utf8(workstation, -1, &buffer[10], 246);
      builder.write(buffer, strlen(buffer));
   }

   if (objectId != 0)
   {
      shared_ptr<NetObj> object = FindObjectById(objectId);
      if (object != nullptr)
      {
         strcpy(&buffer[1], "object=");
         tchar_to_utf8(object->getName(), -1, &buffer[8], 248);
         builder.write(buffer, strlen(buffer));
      }
   }

   if (g_leefExtraData != nullptr)
      builder.write(g_leefExtraData, strlen(g_leefExtraData));

   builder.write(static_cast<BYTE>(0));
   s_senderQueue.put(builder.takeBuffer());
}

/**
 * Encode field from NXCP message into LEEF message
 */
static void EncodeFieldFomMessage(ByteStream *builder, const NXCPMessage& msg, uint32_t fieldId, const char *tag)
{
   char buffer[256] = "";
   msg.getFieldAsUtf8String(fieldId, buffer, sizeof(buffer));
   if (buffer[0] != 0)
   {
      builder->write(g_leefSeparatorChar);
      builder->write(tag, strlen(tag));
      builder->write('=');
      builder->write(buffer, strlen(buffer));
   }
}

/**
 * Enqueue audit record from external application.
 * Audit record structure:
 *    timestamp
 *    success/failure
 *    user name
 *    workstation IP
 *    message
 *    offsets 5-8: reserved
 *    offset 9: number of additional key/value pairs
 *    offset 10+: key/value pairs
 */
void EnqueueLeefAuditRecord(const NXCPMessage& msg, uint32_t nodeId)
{
   char vendor[32] = "", product[32] = "", productVersion[32] = "", hostname[256] = "";

   shared_ptr<NetObj> node = FindObjectById(nodeId, OBJECT_NODE);
   if (node != nullptr)
      tchar_to_utf8(node->getName(), -1, hostname, sizeof(hostname));

   msg.getFieldAsUtf8String(VID_VENDOR, vendor, sizeof(vendor));
   msg.getFieldAsUtf8String(VID_PRODUCT_NAME, product, sizeof(product));
   msg.getFieldAsUtf8String(VID_PRODUCT_VERSION, productVersion, sizeof(productVersion));

   ByteStream builder;

   int recordCount = msg.getFieldAsInt32(VID_NUM_ELEMENTS);
   uint32_t baseId = VID_ELEMENT_LIST_BASE;
   for(int i = 0; i < recordCount; i++, baseId += 1024)
   {
      EncodeCommonLeefFields(&builder, (hostname[0] != 0) ? hostname : g_leefHostname, (vendor[0] != 0) ? vendor : g_leefVendor,
               (product[0] != 0) ? product : g_leefProduct, (productVersion[0] != 0) ? productVersion : g_leefProductVersion,
               msg.getFieldAsTime(baseId), msg.getFieldAsBoolean(baseId + 1), msg.getFieldAsUtf8String(baseId + 4));

      EncodeFieldFomMessage(&builder, msg, baseId + 2, "usrName");
      EncodeFieldFomMessage(&builder, msg, baseId + 3, "identSrc");

      // Encode extra fields
      int fieldCount = msg.getFieldAsInt32(baseId + 9);
      uint32_t fieldId = baseId + 10;
      char tag[256];
      for(int j = 0; j < fieldCount; j++)
      {
         msg.getFieldAsUtf8String(fieldId++, tag, sizeof(tag));
         if (tag[0] != 0)
         {
            EncodeFieldFomMessage(&builder, msg, fieldId++, tag);
         }
      }

      builder.write(static_cast<BYTE>(0));
      s_senderQueue.put(builder.takeBuffer());
   }
}

/**
 * Sender thread
 */
static void Sender()
{
   nxlog_debug_tag(DEBUG_TAG_LEEF, 3, _T("LEEF sender thread started"));

   SOCKET hSocket = CreateSocket(g_leefServer.getFamily(), SOCK_DGRAM, 0);
   time_t socketCreateTime = time(nullptr);
   while(true)
   {
      char *msg = s_senderQueue.getOrBlock();
      if (msg == INVALID_POINTER_VALUE)
         break;

      int len = static_cast<int>(strlen(msg));

      SockAddrBuffer addr;
      g_leefServer.fillSockAddr(&addr, g_leefPort);
      if ((hSocket == INVALID_SOCKET) || (sendto(hSocket, msg, len, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) != len))
      {
         // On failure, try to re-create socket, not faster than once per 10 minutes
         if (time(nullptr) - socketCreateTime > 600)
         {
            TCHAR errorText[1024];
            nxlog_debug_tag(DEBUG_TAG_LEEF, 6, _T("Send failure: %s (destination is %s:%u), re-create socket"),
                     GetLastSocketErrorText(errorText, 1024), g_leefServer.toString().cstr(), g_leefPort);
            closesocket(hSocket);
            hSocket = CreateSocket(g_leefServer.getFamily(), SOCK_DGRAM, 0);
            if (hSocket != INVALID_SOCKET)
               sendto(hSocket, msg, len, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
            socketCreateTime = time(nullptr);
         }
      }

      MemFree(msg);
   }
   nxlog_debug_tag(DEBUG_TAG_LEEF, 3, _T("LEEF sender thread stopped"));
}

/**
 * Sender thread handle
 */
static THREAD s_senderThread = INVALID_THREAD_HANDLE;

/**
 * Start LEEF sender
 */
void StartLeefSender()
{
   s_senderThread = ThreadCreateEx(Sender);
}

/**
 * Stop LEEF sender
 */
void StopLeefSender()
{
   s_senderQueue.put(INVALID_POINTER_VALUE);
   ThreadJoin(s_senderThread);
}
