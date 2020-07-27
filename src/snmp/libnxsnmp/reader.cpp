/*
** NetXMS - Network Management System
** SNMP support library
** Copyright (C) 2003-2020 Victor Kirhenshtein
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
** File: reader.cpp
**
**/

#include "libnxsnmp.h"

/**
 * Maximum number of requests per packet
 */
#define MAX_REQUESTS_PER_PACKET  16

/**
 * Queue request
 */
void SNMP_Reader::queueRequest(const SNMP_ObjectId& oid, void (*callback)(const SNMP_ObjectId&, const SNMP_Variable*, void*), void *context)
{
   SNMP_ReaderRequest *request = m_memoryPool.allocate();
   request->oid = oid;
   request->callback = callback;
   request->context = context;
   m_queue.put(request);
}

/**
 * Worker thread
 */
void SNMP_Reader::worker()
{
   SNMP_ReaderRequest *requests[MAX_REQUESTS_PER_PACKET];

   while(true)
   {
      SNMP_ReaderRequest *request = m_queue.getOrBlock();
      if (request == INVALID_POINTER_VALUE)
         break;

      requests[0] = request;
      int count = 1;
      while(count < MAX_REQUESTS_PER_PACKET)
      {
         request = m_queue.get();
         if (request == nullptr)
            break;
         requests[count++] = request;
      }

      SNMP_PDU snmpRequest(SNMP_GET_REQUEST, SnmpNewRequestId(), m_transport->getSnmpVersion());
      for(int i = 0; i < count; i++)
         snmpRequest.bindVariable(new SNMP_Variable(requests[i]->oid));

      SNMP_PDU *snmpResponse;
      if (m_transport->doRequest(&snmpRequest, &snmpResponse, SnmpGetDefaultTimeout(), 3) == SNMP_ERR_SUCCESS)
      {
         if (snmpResponse->getErrorCode() != SNMP_PDU_ERR_SUCCESS)
            delete_and_null(snmpResponse);
      }

      for(int i = 0; i < count; i++)
      {
         request = requests[i];
         const SNMP_Variable *varbind = (snmpResponse != nullptr) ? snmpResponse->getVariable(i) : nullptr;
         if ((varbind != nullptr) && ((varbind->getType() == ASN_NO_SUCH_OBJECT) || (varbind->getType() == ASN_NO_SUCH_INSTANCE) || (varbind->getType() == ASN_END_OF_MIBVIEW)))
            varbind = nullptr;
         request->callback(request->oid, varbind, request->context);
         m_memoryPool.free(request);
      }

      delete snmpResponse;
   }

   ConditionSet(m_workerCompleted);
}
