/*
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2003-2022 Victor Kirhenshtein
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
** File: dc.cpp
**
**/

#include "libnxclient.h"

/**
 * Push data
 */
uint32_t DataCollectionController::pushData(ObjectArray<NXCPushData> *data, Timestamp timestamp, uint32_t *failedIndex)
{
   NXCPMessage request(CMD_PUSH_DCI_DATA, m_session->createMessageId());
   request.setField(VID_TIMESTAMP_MS, timestamp);
   request.setField(VID_NUM_ITEMS, (INT32)data->size());

   uint32_t id = VID_PUSH_DCI_DATA_BASE;
   for(int i = 0; i < data->size(); i++)
   {
      NXCPushData *d = data->get(i);
      request.setField(id++, d->nodeId);
      if (d->nodeId == 0)
      {
         request.setField(id++, d->nodeName);
      }

      request.setField(id++, d->dciId);
      if (d->dciId == 0)
      {
         request.setField(id++, d->dciName);
      }

      request.setField(id++, d->value);
   }

   m_session->sendMessage(&request);

   uint32_t rcc;
   NXCPMessage *response = m_session->waitForMessage(CMD_REQUEST_COMPLETED, request.getId());
   if (response != nullptr)
   {
      rcc = response->getFieldAsUInt32(VID_RCC);
      if (rcc != RCC_SUCCESS)
      {
         if (failedIndex != nullptr)
            *failedIndex = response->getFieldAsUInt32(VID_FAILED_DCI_INDEX);
      }
      delete response;
   }
   else
   {
      rcc = RCC_TIMEOUT;
      if (failedIndex != nullptr)
         *failedIndex = 0;
   }
   return rcc;
}
