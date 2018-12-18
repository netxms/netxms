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
** File: rtcp.cpp
**
**/

#include "asterisk.h"

/**
 * Process "RTCP Received" event
 */
void AsteriskSystem::processRTCP(AmiMessage *msg)
{
   const char *channel = msg->getTag("Channel");
   if ((channel == NULL) || (*channel == 0))
      return;

   if (msg->getTagAsInt32("ReportCount") == 0)
      return;

#ifdef UNICODE
   WCHAR channelKey[256];
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, channel, -1, channelKey, 256);
#else
#define channelKey channel
#endif

   RTCPData *data = m_rtcpData.get(channelKey);
   if (data != NULL)
   {
      data->jitter = msg->getTagAsUInt32("Report0IAJitter");
      UINT64 packetLoss = static_cast<UINT64>(data->packetLoss) * data->count + msg->getTagAsUInt32("Report0FractionLost");
      UINT64 rtt = static_cast<UINT64>(data->rtt) * data->count + static_cast<UINT64>(msg->getTagAsDouble("RTT") * 1000);
      data->count++;
      data->packetLoss = static_cast<UINT32>(packetLoss / data->count);
      data->rtt = static_cast<UINT32>(rtt / data->count);
   }
   else
   {
      data = new RTCPData;
      data->count = 1;
      data->jitter = msg->getTagAsUInt32("Report0IAJitter");
      data->packetLoss = msg->getTagAsUInt32("Report0FractionLost");
      data->rtt = static_cast<UINT32>(msg->getTagAsDouble("RTT") * 1000);
      m_rtcpData.set(channelKey, data);
   }
#undef channelKey
}

/**
 * Initialize RTCP statistic entry
 */
inline void InitRTCPStatisticEntry(UINT32 curr, UINT64 *stat)
{
   stat[RTCP_LAST] = curr;
   stat[RTCP_MIN] = curr;
   stat[RTCP_MAX] = curr;
   stat[RTCP_AVG] = static_cast<UINT64>(curr) << EMA_FP_SHIFT;
}

/**
 * Update RTCP statistic entry
 */
inline void UpdateRTCPStatisticEntry(UINT32 curr, UINT64 *stat)
{
   stat[RTCP_LAST] = curr;
   if (curr < stat[RTCP_MIN])
      stat[RTCP_MIN] = curr;
   if (curr > stat[RTCP_MAX])
      stat[RTCP_MAX] = curr;
   UpdateExpMovingAverage(stat[RTCP_AVG], EMA_EXP_15, static_cast<UINT64>(curr) << EMA_FP_SHIFT);
}

/**
 * Update RTCP statistic
 */
void AsteriskSystem::updateRTCPStatistic(const char *channel, const TCHAR *peer)
{
#ifdef UNICODE
   WCHAR channelKey[256];
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, channel, -1, channelKey, 256);
#else
#define channelKey channel
#endif
   RTCPData *data = m_rtcpData.get(channelKey);

   if (data == NULL)
      return;  // No RTCP data for this channel

   MutexLock(m_rtcpLock);
   RTCPStatistic *stat = m_peerRTCPStatistic.get(peer);
   if (stat == NULL)
   {
      stat = new RTCPStatistic;
      InitRTCPStatisticEntry(data->jitter, stat->jitter);
      InitRTCPStatisticEntry(data->packetLoss * 100 / 256, stat->packetLoss);
      InitRTCPStatisticEntry(data->rtt, stat->rtt);
      m_peerRTCPStatistic.set(peer, stat);
   }
   else
   {
      UpdateRTCPStatisticEntry(data->jitter, stat->jitter);
      UpdateRTCPStatisticEntry(data->packetLoss * 100 / 256, stat->packetLoss);
      UpdateRTCPStatisticEntry(data->rtt, stat->rtt);
   }
   nxlog_debug_tag(DEBUG_TAG, 8, _T("RTCP: %s jitter=%llu/%llu/%llu/%llu loss=%llu/%llu/%llu/%llu rtt=%llu/%llu/%llu/%llu"), peer,
            stat->jitter[RTCP_MIN], stat->jitter[RTCP_MAX], stat->jitter[RTCP_AVG] >> EMA_FP_SHIFT, stat->jitter[RTCP_LAST],
            stat->packetLoss[RTCP_MIN], stat->packetLoss[RTCP_MAX], stat->packetLoss[RTCP_AVG] >> EMA_FP_SHIFT, stat->packetLoss[RTCP_LAST],
            stat->rtt[RTCP_MIN], stat->rtt[RTCP_MAX], stat->rtt[RTCP_AVG] >> EMA_FP_SHIFT, stat->rtt[RTCP_LAST]);
   MutexUnlock(m_rtcpLock);

   m_rtcpData.remove(channelKey);
#undef channelKey
}

/**
 * Get RTCP statistic for given peer
 */
RTCPStatistic *AsteriskSystem::getPeerRTCPStatistic(const TCHAR *peer, RTCPStatistic *buffer) const
{
   MutexLock(m_rtcpLock);
   const RTCPStatistic *stats = m_peerRTCPStatistic.get(peer);
   if (stats != NULL)
      memcpy(buffer, stats, sizeof(RTCPStatistic));
   MutexUnlock(m_rtcpLock);
   return (stats != NULL) ? buffer : NULL;
}

/**
 * Get value from RTCP statistics
 */
inline void GetRTCPValue(UINT64 *elements, TCHAR key, TCHAR *value)
{
   RTCPStatisticElementIndex index;
   switch(key)
   {
      case 'A':
         index = RTCP_AVG;
         break;
      case 'L':
         index = RTCP_LAST;
         break;
      case 'm':
         index = RTCP_MIN;
         break;
      case 'M':
         index = RTCP_MAX;
         break;
      default:
         return;
   }
   ret_uint64(value, (index == RTCP_AVG) ? elements[index] >> EMA_FP_SHIFT : elements[index]);
}

/**
 * Handler for Asterisk.Peer.RTCP.* parameters
 */
LONG H_PeerRTCPStats(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   GET_ASTERISK_SYSTEM(1);

   TCHAR peerId[128];
   GET_ARGUMENT(1, peerId, 128);

   RTCPStatistic stats;
   if (sys->getPeerRTCPStatistic(peerId, &stats) == NULL)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   switch(arg[0])
   {
      case 'J':
         GetRTCPValue(stats.jitter, arg[1], value);
         break;
      case 'P':
         GetRTCPValue(stats.packetLoss, arg[1], value);
         break;
      case 'R':
         GetRTCPValue(stats.rtt, arg[1], value);
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }
   return SYSINFO_RC_SUCCESS;
}
