/*
** NetXMS subagent for GNU/Linux
** Copyright (C) 2004-2024 Raden Solutions
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
**/

#include "linux_subagent.h"
#include <math.h>
#include <cmath>

#define CPU_USAGE_SLOTS			900 // 60 sec * 15 min => 900 sec

/**
 * Almost RingBuffer.
 * But:
 * - not byte-based (float-based here)
 * - fixed in size (no growth requirements)
 * - data always fed in single pieces
 */
struct MeasurementsTable
{
   const uint32_t m_allocated = CPU_USAGE_SLOTS; // how big is the array? in elements, not bytes

   float m_data[CPU_USAGE_SLOTS];
   uint32_t m_size; // how many are stored? in elements, not bytes
   uint32_t m_writePos; // where to write next element? in elements, not bytes

   MeasurementsTable()
   {
      m_size = 0;
      m_writePos = 0;
      for (int i = 0; i < CPU_USAGE_SLOTS; i++)
      {
         m_data[i] = NAN;
      }
   }

   float getAverage(uint32_t nbLastItems);

   void reset()
   {
      m_size = 0;
      m_writePos = 0;
   }

   void update(float measurement);
};

float MeasurementsTable::getAverage(uint32_t nbLastItems)
{
   float total = 0.0;
   uint32_t nbElem = std::min(m_size, nbLastItems);

   assert(nbElem != 0);
   if (nbElem == 0)
   {
      return 0;
   }

   assert(m_size <= m_allocated);
   assert(m_writePos < m_allocated);

   for (uint32_t i = 0; i < nbElem; i++)
   {
      uint32_t offset = (m_writePos - i - 1) % m_allocated;
      assert(!isnan(m_data[offset]));

      total += m_data[offset];
   }
   return total / nbElem;
}

void MeasurementsTable::update(float measurement)
{
   assert(m_size <= m_allocated);
   assert(m_writePos < m_allocated);
   auto debugPrevSize = m_size;

   m_data[m_writePos] = measurement;
   m_writePos = (m_writePos + 1) % m_allocated;
   m_size = std::min(m_size + 1, m_allocated);

   assert(m_size <= m_allocated);
   assert(m_writePos < m_allocated);

   if (debugPrevSize == m_allocated)
   {
      assert(m_size == m_allocated);
   }
   else
   {
      assert(m_size == 1 + debugPrevSize);
   }
}

class CpuStats
{
private:
   bool m_on;
   bool m_havePrevMeasurements;
   uint64_t m_prevMeasurements[CPU_USAGE_NB_SOURCES];

public:
   MeasurementsTable m_tables[CPU_USAGE_NB_SOURCES];

   CpuStats()
   {
      m_on = false;
      m_havePrevMeasurements = false;
      for (int i = 0; i < CPU_USAGE_NB_SOURCES; i++)
      {
         new (&m_tables[i]) MeasurementsTable();
      }
   }

   void update(const uint64_t measurements[CPU_USAGE_NB_SOURCES]);

   bool isOn() const { return m_on; }
   void setOff()
   {
      for (int i = 0; i < CPU_USAGE_NB_SOURCES; i++)
         m_tables[i].reset();
      m_on = false;
      m_havePrevMeasurements = false;
   }
};

// Use a mutex to ensure thread safety!

struct Collector
{
   CpuStats m_total;
   std::vector<CpuStats> m_perCore;
   uint64_t m_cpuInterrupts;
   uint64_t m_cpuContextSwitches;

   Collector() : m_perCore(0)
   {
      m_cpuInterrupts = 0;
      m_cpuContextSwitches = 0;
   }

   void collect();

   float getCoreUsage(enum CpuUsageSource source, int coreIndex, int nbLastItems);

   float getTotalUsage(enum CpuUsageSource source, int nbLastItems)
   {
      return m_total.m_tables[source].getAverage(nbLastItems);
   }
};

/**
 * CPU usage data collection
 *
 * Must be called with the mutex held.
 */
void Collector::collect()
{
   FILE *hStat = fopen("/proc/stat", "r");
   if (hStat == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot open /proc/stat"));
      return;
   }

   char buffer[1024];
   std::vector<bool> coreReported(this->m_perCore.size());

   // scan for all CPUs
   while(true)
   {
      if (fgets(buffer, sizeof(buffer), hStat) == NULL)
         break;

      int ret;
      if (buffer[0] == 'c' && buffer[1] == 'p' && buffer[2] == 'u')
      {
         uint64_t user, nice, system, idle;
         uint64_t iowait = 0, irq = 0, softirq = 0; // 2.6
         uint64_t steal = 0; // 2.6.11
         uint64_t guest = 0; // 2.6.24
         if (buffer[3] == ' ')
         {
            // "cpu ..." - Overall across all cores
            ret = sscanf(buffer, "cpu " UINT64_FMTA " " UINT64_FMTA " " UINT64_FMTA " " UINT64_FMTA " " UINT64_FMTA " " UINT64_FMTA " " UINT64_FMTA " " UINT64_FMTA " " UINT64_FMTA,
                  &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal, &guest);
            if (ret == 9)
            {
               uint64_t measurements[CPU_USAGE_NB_SOURCES] = { 0, user, nice, system, idle, iowait, irq, softirq, steal, guest };
               m_total.update(measurements);
            }
         }
         else
         {
            uint32_t cpuIndex = 0;
            ret = sscanf(buffer, "cpu%u " UINT64_FMTA " " UINT64_FMTA " " UINT64_FMTA " " UINT64_FMTA " " UINT64_FMTA " " UINT64_FMTA " " UINT64_FMTA " " UINT64_FMTA " " UINT64_FMTA,
                  &cpuIndex, &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal, &guest);
            if (ret == 10) {
               if (m_perCore.size() < cpuIndex + 1)
               {
                  nxlog_debug_tag(DEBUG_TAG, 9, _T("Growing cores vector from %u to %u"), m_perCore.size(), cpuIndex + 1);
                  m_perCore.resize(cpuIndex + 1);
#ifndef NDEBUG
                  CpuStats &thisCore = m_perCore.at(cpuIndex);
                  assert(!thisCore.isOn());
                  assert(thisCore.m_tables[0].m_size == 0);
#endif
                  coreReported.resize(cpuIndex + 1);
               }
               CpuStats &thisCore = m_perCore.at(cpuIndex);
               uint64_t measurements[CPU_USAGE_NB_SOURCES] = { 0, user, nice, system, idle, iowait, irq, softirq, steal, guest };
               thisCore.update(measurements);
               coreReported[cpuIndex] = true;
            }
         }
      }
      else if (buffer[0] == 'i' && buffer[1] == 'n' && buffer[2] == 't' && buffer[3] == 'r')
      {
         ret = sscanf(buffer, "intr " UINT64_FMTA, &m_cpuInterrupts);
         assert(ret == 1); // between us and kernel we should always get this right
      }
      else if (buffer[0] == 'c' && buffer[1] == 't' && buffer[2] == 'x' && buffer[3] == 't')
      {
         ret = sscanf(buffer, "ctxt " UINT64_FMTA, &m_cpuContextSwitches);
         assert(ret == 1); // between us and kernel we should always get this right
      }
      else
      {
         continue;
      }

   } // while(true) file traversal
   fclose(hStat);
   for (uint32_t cpuIndex = 0; cpuIndex < coreReported.size(); cpuIndex++)
   {
      if (!coreReported[cpuIndex] && m_perCore[cpuIndex].isOn())
      {
         nxlog_debug_tag(DEBUG_TAG, 9, _T("Core %u was not reported this time"), m_perCore.size(), cpuIndex + 1);
         m_perCore[cpuIndex].setOff();
      }
   }
}

static inline uint64_t Delta(uint64_t x, uint64_t y)
{
   return (x > y) ? (x - y) : 0;
}

void CpuStats::update(const uint64_t measurements[CPU_USAGE_NB_SOURCES])
{
   uint64_t deltas[CPU_USAGE_NB_SOURCES] = {0,};
   uint64_t totalDelta = 0;

   if (m_havePrevMeasurements) {
      for (int i = 1 /* skip CPU_USAGE_OVERAL */; i < CPU_USAGE_NB_SOURCES; i++)
      {
         uint64_t delta = Delta(measurements[i], m_prevMeasurements[i]);
         totalDelta += delta;
         deltas[i] = delta;
      }

      float onePercent = (float)totalDelta / 100.0; // 1% of total
      if (onePercent == 0)
      {
         onePercent = 1; // TODO: why 1?
      }

      /* update detailed stats */
      for (int i = 1 /* skip CPU_USAGE_OVERAL */; i < CPU_USAGE_NB_SOURCES; i++)
      {
         uint64_t delta = deltas[i];
         m_tables[i].update(delta == 0 ? 0 : (float)delta / onePercent);
      }
      /* update overal cpu usage */
      m_tables[CPU_USAGE_OVERAL].update(totalDelta == 0 ? 0 : 100.0 - (float)deltas[CPU_USAGE_IDLE] / onePercent);

   }
   for (int i = 1 /* skip CPU_USAGE_OVERAL */; i < CPU_USAGE_NB_SOURCES; i++)
   {
      m_prevMeasurements[i] = measurements[i];
   }
   m_havePrevMeasurements = true;
   m_on = true;
}

/**
 * @param coreIndex 0-based core index
 */
float Collector::getCoreUsage(enum CpuUsageSource source, int coreIndex, int nbLastItems)
{
   if (coreIndex >= m_perCore.size())
      return 0;

   CpuStats &core = m_perCore[coreIndex];

   // If core wasn't reported, or no delta-based samples yet, we have nothing to average.
   if (!core.isOn() || (core.m_tables[source].m_size == 0))
      return 0;

   return core.m_tables[source].getAverage(nbLastItems);
}
