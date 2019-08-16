/*
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Victor Kirhenshtein
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
** File: icmpstat.cpp
**
**/

#include "nxcore.h"

#define DEBUG_TAG_ICMP_POLL   _T("poll.icmp")

/**
 * Constructor
 */
IcmpStatCollector::IcmpStatCollector(int period)
{
   m_lastResponseTime = 0;
   m_minResponseTime = 0;
   m_maxResponseTime = 0;
   m_avgResponseTime = 0;
   m_packetLoss = 0;
   m_rawResponseTimes = MemAllocArrayNoInit<UINT16>(period);
   memset(m_rawResponseTimes, 0xFF, period * sizeof(UINT16));
   m_writePos = 0;
   m_bufferSize = period;
}

/**
 * Destructor
 */
IcmpStatCollector::~IcmpStatCollector()
{
   MemFree(m_rawResponseTimes);
}

/**
 * Recalculate statistics
 */
void IcmpStatCollector::recalculate()
{
   m_minResponseTime = 0;
   m_maxResponseTime = 0;

   int sampleCount = 0;
   int responseCount = 0;
   UINT64 totalTime = 0;
   for(int i = 0; i < m_bufferSize; i++)
   {
      if (m_rawResponseTimes[i] == 0xFFFF)
         continue;
      sampleCount++;
      if (m_rawResponseTimes[i] != 0xFFF)
      {
         responseCount++;
         totalTime += m_rawResponseTimes[i];
         if (responseCount == 1)
         {
            m_minResponseTime = m_maxResponseTime = m_rawResponseTimes[i];
         }
         else
         {
            if (m_minResponseTime > m_rawResponseTimes[i])
               m_minResponseTime = m_rawResponseTimes[i];
            if (m_maxResponseTime < m_rawResponseTimes[i])
               m_maxResponseTime = m_rawResponseTimes[i];
         }
      }
   }

   if (responseCount > 0)
   {
      m_avgResponseTime = static_cast<UINT32>(totalTime / responseCount);
      m_packetLoss = (sampleCount - responseCount) * 100 / sampleCount;
   }
   else
   {
      m_avgResponseTime = 0;
      m_packetLoss = 100;
   }
}

/**
 * Update collector
 */
void IcmpStatCollector::update(UINT32 responseTime)
{
   if (responseTime == 10000)
   {
      m_rawResponseTimes[m_writePos++] = 0xFFF;
   }
   else
   {
      m_lastResponseTime = (responseTime > 0xFFE) ? 0xFFE : responseTime;
      m_rawResponseTimes[m_writePos++] = m_lastResponseTime;
   }
   if (m_writePos == m_bufferSize)
      m_writePos = 0;
   recalculate();
}

/**
 * Resize collector
 */
void IcmpStatCollector::resize(int period)
{
   if (m_bufferSize == period)
      return;

   UINT16 *responseTimes = MemAllocArrayNoInit<UINT16>(period);
   if (period > m_bufferSize)
   {
      memcpy(responseTimes, &m_rawResponseTimes[m_writePos], (m_bufferSize - m_writePos) * sizeof(UINT16));
      memcpy(&responseTimes[m_bufferSize - m_writePos], m_rawResponseTimes, m_writePos * sizeof(UINT16));
      memset(&responseTimes[m_bufferSize], 0xFF, (period - m_bufferSize) * sizeof(UINT16));
      m_writePos = m_bufferSize;
   }
   else
   {
      int pos = m_writePos;
      for(int i = period - 1; i >= 0; i--)
      {
         pos--;
         if (pos < 0)
            pos = m_bufferSize;
         responseTimes[i] = m_rawResponseTimes[pos];
      }
      m_writePos = 0;
   }
   m_bufferSize = period;
   MemFree(m_rawResponseTimes);
   m_rawResponseTimes = responseTimes;

   recalculate();
}

/**
 * Save to database
 */
bool IcmpStatCollector::saveToDatabase(DB_HANDLE hdb, UINT32 objectId, const TCHAR *target) const
{
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT sample_count FROM icmp_statistics WHERE object_id=? AND poll_target=?"));
   if (hStmt == NULL)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, objectId);
   DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, target, DB_BIND_STATIC);

   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult == NULL)
   {
      DBFreeStatement(hStmt);
      return false;
   }

   bool recordExists = (DBGetNumRows(hResult) != 0);
   DBFreeResult(hResult);
   DBFreeStatement(hStmt);

   hStmt = recordExists ?
            DBPrepare(hdb, _T("UPDATE icmp_statistics SET min_response_time=?,max_response_time=?,avg_response_time=?,last_response_time=?,sample_count=?,raw_response_times=? WHERE object_id=? AND poll_target=?")) :
            DBPrepare(hdb, _T("INSERT INTO icmp_statistics (min_response_time,max_response_time,avg_response_time,last_response_time,sample_count,raw_response_times,object_id,poll_target) VALUES (?,?,?,?,?,?,?,?)"));
   if (hStmt == NULL)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_minResponseTime);
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_maxResponseTime);
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_avgResponseTime);
   DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_lastResponseTime);

   int sampleCount = 0;
   String serializedResponseTimes;
   for(int i = m_writePos, j = 0; j < m_bufferSize; j++)
   {
      if (m_rawResponseTimes[i] != 0xFFFF)
      {
         TCHAR buffer[4];
         _sntprintf(buffer, 4, _T("%03X"), m_rawResponseTimes[i]);
         serializedResponseTimes.append(buffer);
         sampleCount++;
      }
      i++;
      if (i == m_bufferSize)
         i = 0;
   }
   DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, sampleCount);
   DBBind(hStmt, 6, DB_SQLTYPE_TEXT, serializedResponseTimes, DB_BIND_STATIC);

   DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, objectId);
   DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, target, DB_BIND_STATIC);

   bool success = DBExecute(hStmt);
   DBFreeStatement(hStmt);
   return success;
}

/**
 * Create collector from database record
 */
IcmpStatCollector *IcmpStatCollector::loadFromDatabase(DB_HANDLE hdb, UINT32 objectId, const TCHAR *target, int period)
{
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT min_response_time,max_response_time,avg_response_time,last_response_time,sample_count,raw_response_times FROM icmp_statistics WHERE object_id=? AND poll_target=?"));
   if (hStmt == NULL)
      return NULL;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, objectId);
   DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, target, DB_BIND_STATIC);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult == NULL)
   {
      DBFreeStatement(hStmt);
      return NULL;
   }

   if (DBGetNumRows(hResult) == 0)
   {
      DBFreeResult(hResult);
      DBFreeStatement(hStmt);
      return NULL;
   }

   IcmpStatCollector *collector = new IcmpStatCollector(period);
   collector->m_minResponseTime = DBGetFieldULong(hResult, 0, 0);
   collector->m_maxResponseTime = DBGetFieldULong(hResult, 0, 1);
   collector->m_avgResponseTime = DBGetFieldULong(hResult, 0, 2);
   collector->m_lastResponseTime = DBGetFieldULong(hResult, 0, 3);

   int sampleCount = DBGetFieldLong(hResult, 0, 4);
   if (sampleCount > 0)
   {
      TCHAR *data = DBGetField(hResult, 0, 5, NULL, 0);
      if ((data != NULL) && (_tcslen(data) == sampleCount * 3))
      {
         for(int i = 0, pos = 0; i < sampleCount; i++, pos += 3)
         {
            TCHAR value[4];
            memcpy(value, &data[pos], 3 * sizeof(TCHAR));
            value[3] = 0;
            collector->m_rawResponseTimes[i] = static_cast<UINT16>(_tcstoul(value, NULL, 16));
         }
         if (sampleCount < collector->m_bufferSize)
            collector->m_writePos = sampleCount;
      }
      else
      {
         nxlog_debug_tag(_T("poll.icmp"), 6, _T("Sample count and raw data length mismatch for collector %s at node [%u] (count %d, len %d)"),
                  target, objectId, sampleCount, (int)_tcslen(CHECK_NULL_EX(data)));
      }
      MemFree(data);
   }

   DBFreeResult(hResult);
   DBFreeStatement(hStmt);
   return collector;
}
