/*
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
UINT32 DataCollectionController::pushData(ObjectArray<NXCPushData> *data, time_t timestamp, UINT32 *failedIndex)
{
   NXCPMessage msg;
   msg.setCode(CMD_PUSH_DCI_DATA);
   msg.setId(m_session->createMessageId());
   msg.setFieldFromTime(VID_TIMESTAMP, timestamp);
   msg.setField(VID_NUM_ITEMS, (INT32)data->size());

   UINT32 id = VID_PUSH_DCI_DATA_BASE;
   for(int i = 0; i < data->size(); i++)
   {
      NXCPushData *d = data->get(i);
      msg.setField(id++, d->nodeId);
      if (d->nodeId == 0)
      {
         msg.setField(id++, d->nodeName);
      }

      msg.setField(id++, d->dciId);
      if (d->dciId == 0)
      {
         msg.setField(id++, d->dciName);
      }

      msg.setField(id++, d->value);
   }

   m_session->sendMessage(&msg);

   UINT32 rcc;
   NXCPMessage *response = m_session->waitForMessage(CMD_REQUEST_COMPLETED, msg.getId());
   if (response != NULL)
   {
      rcc = response->getFieldAsUInt32(VID_RCC);
      if (rcc != RCC_SUCCESS)
      {
         if (failedIndex != NULL)
            *failedIndex = response->getFieldAsUInt32(VID_FAILED_DCI_INDEX);
      }
      delete response;
   }
   else
   {
      rcc = RCC_TIMEOUT;
      if (failedIndex != NULL)
         *failedIndex = 0;
   }
   return rcc;
}
