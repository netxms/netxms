/*
** NetXMS Asterisk subagent
** Copyright (C) 2004-2018 Victor Kirhenshtein
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
** File: events.cpp
**
**/

#include "asterisk.h"

/**
 * Setup event filters on AMI session
 */
void AsteriskSystem::setupEventFilters()
{
   ThreadSleepMs(100);

   AmiMessage *request = new AmiMessage("Events");
   request->setTag("EventMask", "call,reporting");
   sendSimpleRequest(request);

   // Try to setup filter for specific events (may fail if user does not have "system" permission)
   // https://wiki.asterisk.org/wiki/display/AST/Asterisk+16+ManagerAction_Filter
   request = new AmiMessage("Filter");
   request->setTag("Operation", "Add");
   request->setTag("Filter", "Event: Hangup");
   sendSimpleRequest(request);

   request = new AmiMessage("Filter");
   request->setTag("Operation", "Add");
   request->setTag("Filter", "Event: RTCPReceived");
   sendSimpleRequest(request);

   nxlog_debug_tag(DEBUG_TAG, 6, _T("Event filter setup complete for %s"), m_name);
}

/**
 * Get event counters for given peer
 */
const EventCounters *AsteriskSystem::getPeerEventCounters(const TCHAR *peer) const
{
   MutexLock(m_eventCounterLock);
   const EventCounters *counters = m_peerEventCounters.get(peer);
   MutexUnlock(m_eventCounterLock);
   return counters;
}

/**
 * Process hangup event
 */
void AsteriskSystem::processHangup(AmiMessage *msg)
{
   const char *channel = msg->getTag("Channel");

   TCHAR peer[128];
   PeerFromChannel(channel, peer, 128);
   EventCounters *peerEventCounters;
   if (peer[0] != 0)
   {
      MutexLock(m_eventCounterLock);
      peerEventCounters = m_peerEventCounters.get(peer);
      if (peerEventCounters == NULL)
      {
         peerEventCounters = new EventCounters;
         memset(peerEventCounters, 0, sizeof(EventCounters));
         m_peerEventCounters.set(peer, peerEventCounters);
      }
      MutexUnlock(m_eventCounterLock);
   }
   else
   {
      peerEventCounters = NULL;
   }

   switch(msg->getTagAsInt32("Cause"))
   {
      case 2:
      case 3:
         m_globalEventCounters.noRoute++;
         if (peerEventCounters != NULL)
            peerEventCounters->noRoute++;
         break;
      case 20:
         m_globalEventCounters.subscriberAbsent++;
         if (peerEventCounters != NULL)
            peerEventCounters->subscriberAbsent++;
         break;
      case 21:
         m_globalEventCounters.callRejected++;
         if (peerEventCounters != NULL)
            peerEventCounters->callRejected++;
         break;
      case 34:
      case 44:
         m_globalEventCounters.channelUnavailable++;
         if (peerEventCounters != NULL)
            peerEventCounters->channelUnavailable++;
         break;
      case 42:
         m_globalEventCounters.congestion++;
         if (peerEventCounters != NULL)
            peerEventCounters->congestion++;
         break;
      case 52:
      case 54:
         m_globalEventCounters.callBarred++;
         if (peerEventCounters != NULL)
            peerEventCounters->callBarred++;
         break;
   }

   updateRTCPStatistic(channel, peer);
}

/**
 * Handler for Asterisk.Events.* parameter
 */
LONG H_GlobalEventCounters(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   GET_ASTERISK_SYSTEM(0);
   switch(*arg)
   {
      case 'A':
         ret_uint64(value, sys->getGlobalEventCounters()->subscriberAbsent);
         break;
      case 'B':
         ret_uint64(value, sys->getGlobalEventCounters()->callBarred);
         break;
      case 'C':
         ret_uint64(value, sys->getGlobalEventCounters()->congestion);
         break;
      case 'N':
         ret_uint64(value, sys->getGlobalEventCounters()->noRoute);
         break;
      case 'R':
         ret_uint64(value, sys->getGlobalEventCounters()->callRejected);
         break;
      case 'U':
         ret_uint64(value, sys->getGlobalEventCounters()->channelUnavailable);
         break;
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Asterisk.SIP.Peer.Events.* parameter
 */
LONG H_PeerEventCounters(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   GET_ASTERISK_SYSTEM(1);

   TCHAR peer[128];
   GET_ARGUMENT(1, peer, 128);

   const EventCounters *cnt = sys->getPeerEventCounters(peer);
   switch(*arg)
   {
      case 'A':
         ret_uint64(value, (cnt != NULL) ? cnt->subscriberAbsent : 0);
         break;
      case 'B':
         ret_uint64(value, (cnt != NULL) ? cnt->callBarred : 0);
         break;
      case 'C':
         ret_uint64(value, (cnt != NULL) ? cnt->congestion : 0);
         break;
      case 'N':
         ret_uint64(value, (cnt != NULL) ? cnt->noRoute : 0);
         break;
      case 'R':
         ret_uint64(value, (cnt != NULL) ? cnt->callRejected : 0);
         break;
      case 'U':
         ret_uint64(value, (cnt != NULL) ? cnt->channelUnavailable : 0);
         break;
   }
   return SYSINFO_RC_SUCCESS;
}
