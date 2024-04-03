#include <math.h>
#include <cmath>

#include "linux_subagent.h"

#define CPU_USAGE_SLOTS			900 // 60 sec * 15 min => 900 sec

/**
 * Almost RingBuffer.
 * But:
 * - not byte-based (float-based here)
 * - fixed in size (no growth requirements)
 * - data always fed in single pieces
 */
class MeasurementsTable
{
public:
   float m_data[CPU_USAGE_SLOTS];
   const uint32_t m_allocated = CPU_USAGE_SLOTS; // how big is the array? in elements, not bytes
   uint32_t m_size; // how many are stored? in elements, not bytes
   uint32_t m_writePos; // where to write next element? in elements, not bytes
   float GetAverage(uint32_t nbLastItems);
   void Reset();
   void Update(float measurement);
   MeasurementsTable();
};

MeasurementsTable::MeasurementsTable():
   m_size(0),
   m_writePos(0)
{
   for (int i = 0; i < CPU_USAGE_SLOTS; i++)
   {
      m_data[i] = NAN;
   }
}

float MeasurementsTable::GetAverage(uint32_t nbLastItems)
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

void MeasurementsTable::Reset()
{
   m_size = 0;
   m_writePos = 0;
}

void MeasurementsTable::Update(float measurement)
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
public:
   MeasurementsTable m_tables[CPU_USAGE_NB_SOURCES];
   void Update(uint64_t measurements[CPU_USAGE_NB_SOURCES]);
   bool IsOn();
   void SetOff();
   CpuStats();
private:
   bool m_on;
   bool m_havePrevMeasurements;
   uint64_t m_prevMeasurements[CPU_USAGE_NB_SOURCES];
   static inline uint64_t Delta(uint64_t x, uint64_t y);
};

CpuStats::CpuStats():
   m_on(false),
   m_havePrevMeasurements(false)
{
   for (int i = 0; i < CPU_USAGE_NB_SOURCES; i++)
   {
      new (&m_tables[i]) MeasurementsTable();
   }
}

void CpuStats::SetOff()
{
   for (int i = 0; i < CPU_USAGE_NB_SOURCES; i++)
   {
      m_tables[i].Reset();
   }
   m_on = false;
   m_havePrevMeasurements = false;
}


// Use a mutex to ensure thread safety!

class Collector
{
public:
   bool m_stopThread;
   THREAD m_thread;

   CpuStats m_total;
   std::vector<CpuStats> m_perCore;
   uint64_t m_cpuInterrupts;
   uint64_t m_cpuContextSwitches;
   void Collect(void);
   Collector();
   float GetCoreUsage(enum CpuUsageSource source, int coreIndex, int nbLastItems);
   float GetTotalUsage(enum CpuUsageSource source, int nbLastItems);
};

Collector::Collector():
   m_stopThread(false),
   m_thread(INVALID_THREAD_HANDLE),
   m_total(),
   m_perCore(0),
   m_cpuInterrupts(0),
   m_cpuContextSwitches(0)
{
}

/**
 * CPU usage data collection
 *
 * Must be called with the mutex held.
 */
void Collector::Collect()
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
            if (ret == 9) {
               uint64_t measurements[CPU_USAGE_NB_SOURCES] = {0, user, nice, system, idle, iowait, irq, softirq, steal, guest};
               m_total.Update(measurements);
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
                  CpuStats &thisCore = m_perCore.at(cpuIndex);
                  assert(thisCore.IsOn() == false);
                  assert(thisCore.m_tables[0].m_size == 0);
                  coreReported.resize(cpuIndex + 1);
               }
               CpuStats &thisCore = m_perCore.at(cpuIndex);
               uint64_t measurements[CPU_USAGE_NB_SOURCES] = {0, user, nice, system, idle, iowait, irq, softirq, steal, guest};
               thisCore.Update(measurements);
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
      if (!coreReported[cpuIndex] && m_perCore[cpuIndex].IsOn())
      {
         nxlog_debug_tag(DEBUG_TAG, 9, _T("Core %u was not reported this time"), m_perCore.size(), cpuIndex + 1);
         m_perCore[cpuIndex].SetOff();
      }
   }
}

bool CpuStats::IsOn()
{
   return m_on;
}

inline uint64_t CpuStats::Delta(uint64_t x, uint64_t y)
{
   return (x > y) ? (x - y) : 0;
}

void CpuStats::Update(uint64_t measurements[CPU_USAGE_NB_SOURCES])
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
         m_tables[i].Update(delta == 0 ? 0 : (float)delta / onePercent);
      }
      /* update overal cpu usage */
      m_tables[CPU_USAGE_OVERAL].Update(totalDelta == 0 ? 0 : 100.0 - (float)deltas[CPU_USAGE_IDLE] / onePercent);

   }
   for (int i = 1 /* skip CPU_USAGE_OVERAL */; i < CPU_USAGE_NB_SOURCES; i++)
   {
      m_prevMeasurements[i] = measurements[i];
   }
   m_havePrevMeasurements = true;
   m_on = true;
}

float Collector::GetTotalUsage(enum CpuUsageSource source, int nbLastItems)
{
      return m_total.m_tables[source].GetAverage(nbLastItems);
}

/**
 * @param coreIndex 0-based core index
 */
float Collector::GetCoreUsage(enum CpuUsageSource source, int coreIndex, int nbLastItems)
{
   if (coreIndex > 100)
   {
      abort();
   }
   if (m_perCore.size() > 100)
   {
      abort();
   }
   if (coreIndex >= m_perCore.size())
   {
      return 0;
   }

   CpuStats &core = m_perCore[coreIndex];

   // If core wasn't reported, or no delta-based samples yet, we have nothing to average.
   if (!core.IsOn() || core.m_tables[source].m_size == 0)
   {
      return 0;
   }
   return core.m_tables[source].GetAverage(nbLastItems);
}
