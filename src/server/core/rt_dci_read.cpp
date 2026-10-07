/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: rt_dci_read.cpp
**/

#include "nxcore.h"

#define DEBUG_TAG DEBUG_TAG_DC_RTREAD

/**
 * Data collector thread pool
 */
extern ThreadPool *g_dataCollectorThreadPool;

/**
 * Create feed for given DCI. Working copy of the DCI starts with clean delta calculation state.
 */
RealTimeDciFeed::RealTimeDciFeed(uint32_t id, session_id_t sessionId, const DCItem& dci, uint32_t interval) :
   m_id(id), m_sessionId(sessionId), m_objectId(dci.getOwnerId()), m_dciId(dci.getId()), m_dci(new DCItem(&dci, false, false)),
   m_interval(interval * 1000), m_cancelled(0), m_firstSample(true)
{
}

/**
 * Destructor
 */
RealTimeDciFeed::~RealTimeDciFeed()
{
   delete m_dci;
}

/**
 * Schedule next poll
 */
void RealTimeDciFeed::schedule(const shared_ptr<RealTimeDciFeed>& feed, uint32_t delay)
{
   if (delay == 0)
      ThreadPoolExecute(g_dataCollectorThreadPool, RealTimeDciFeed::poll, feed);
   else
      ThreadPoolScheduleRelative(g_dataCollectorThreadPool, delay, RealTimeDciFeed::poll, feed);
}

/**
 * Poll DCI once and push result to client session. Reschedules itself after poll completion
 * unless feed was cancelled or terminal error (object or DCI no longer exists) occurred.
 */
void RealTimeDciFeed::poll(const shared_ptr<RealTimeDciFeed>& feed)
{
   if (feed->isCancelled() || IsShutdownInProgress())
      return;

   NXCPMessage msg(CMD_RT_DCI_DATA, feed->m_id);
   bool terminal = false;
   bool send = true;

   shared_ptr<NetObj> object = FindObjectById(feed->m_objectId);
   if ((object != nullptr) && object->isDataCollectionTarget())
   {
      shared_ptr<DataCollectionTarget> owner = static_pointer_cast<DataCollectionTarget>(object);
      shared_ptr<DCObject> liveDci = owner->getDCObjectById(feed->m_dciId, 0);
      if ((liveDci != nullptr) && (liveDci->getType() == DCO_TYPE_ITEM))
      {
         // Resolve effective target the same way as data collector does
         shared_ptr<DataCollectionTarget> target = owner;
         uint32_t sourceNodeId = owner->getEffectiveSourceNode(feed->m_dci);
         if (sourceNodeId != 0)
         {
            shared_ptr<Node> sourceNode = static_pointer_cast<Node>(FindObjectById(sourceNodeId, OBJECT_NODE));
            if ((sourceNode != nullptr) &&
                (((owner->getObjectClass() == OBJECT_CHASSIS) && (static_cast<Chassis&>(*owner).getControllerId() == sourceNodeId)) ||
                 sourceNode->isTrustedObject(owner->getId())))
            {
               target = sourceNode;
            }
            else
            {
               target.reset();
            }
         }

         if (target != nullptr)
         {
            Timestamp timestamp = Timestamp::now();
            wchar_t buffer[MAX_RESULT_LENGTH];
            uint32_t error;
            GetItemData(target.get(), *feed->m_dci, buffer, &error);
            if (error == DCE_SUCCESS)
            {
               ItemValue value(buffer, timestamp, false);
               DataCollectionError rc = feed->m_dci->recalculateValue(value);
               if (rc == DCE_SUCCESS)
               {
                  if (feed->m_firstSample && (feed->m_dci->getDeltaCalculationMethod() != DCM_ORIGINAL_VALUE))
                  {
                     send = false;  // First sample of delta-calculated DCI is always zero, it only sets baseline
                  }
                  else
                  {
                     msg.setField(VID_RCC, RCC_SUCCESS);
                     msg.setField(VID_VALUE, value.getString());
                     msg.setField(VID_TIMESTAMP, timestamp);
                  }
                  feed->m_firstSample = false;
               }
               else
               {
                  msg.setField(VID_RCC, RCCFromDCIError(rc));
               }
            }
            else
            {
               msg.setField(VID_RCC, RCCFromDCIError(static_cast<DataCollectionError>(error)));
            }
         }
         else
         {
            msg.setField(VID_RCC, RCC_DCI_NOT_SUPPORTED);  // Source node does not exist or does not trust DCI owner
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_INVALID_DCI_ID);
         terminal = true;
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
      terminal = true;
   }

   if (feed->isCancelled())
      return;

   if (send)
      NotifyClientSession(feed->m_sessionId, &msg);

   if (terminal)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"Real-time read feed [%u] for DCI [%u] on object [%u] terminated: DCI or object no longer exists",
            feed->m_id, feed->m_dciId, feed->m_objectId);
      feed->cancel();
   }
   else if (!IsShutdownInProgress())
   {
      schedule(feed, feed->m_interval);
   }
}
