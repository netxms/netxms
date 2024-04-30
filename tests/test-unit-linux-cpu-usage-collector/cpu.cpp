#include <nms_common.h>
#include <nms_util.h>
#include <testtools.h>

#include "../../src/agent/subagents/linux/cpu.h"

static Collector *collector = nullptr;
// m_cpuUsageMutex must be held to access `collector`, thread and its internals
static Mutex m_cpuUsageMutex(MutexType::FAST);

const int collectionPeriodMs = 10;
/**
 * CPU usage collector thread
 */
static void CpuUsageCollectorThread()
{
   nxlog_debug_tag(DEBUG_TAG, 2, _T("CPU usage collector thread started"));

   m_cpuUsageMutex.lock();
   while(collector->m_stopThread == false)
   {
      collector->Collect();
      m_cpuUsageMutex.unlock();
      ThreadSleepMs(collectionPeriodMs);
      m_cpuUsageMutex.lock();
   }
   m_cpuUsageMutex.unlock();
   nxlog_debug_tag(DEBUG_TAG, 2, _T("CPU usage collector thread stopped"));
}

/**
 * Start CPU usage collector
 */
void StartCpuUsageCollector()
{
   m_cpuUsageMutex.lock();
   if (collector != nullptr)
   {
      nxlog_write(NXLOG_ERROR, _T("CPU Usage Collector extraneous initialization detected!"));
   }
   assert(collector == nullptr);
   collector = new Collector();

   // start collector
   collector->m_thread = ThreadCreateEx(CpuUsageCollectorThread);
   m_cpuUsageMutex.unlock();
}

/**
 * Shutdown CPU usage collector
 */
void ShutdownCpuUsageCollector()
{
   m_cpuUsageMutex.lock();
   collector->m_stopThread = true;
   m_cpuUsageMutex.unlock();
   ThreadJoin(collector->m_thread);
   m_cpuUsageMutex.lock();
   delete collector;
   collector = nullptr;
   m_cpuUsageMutex.unlock();
}


static void ServeAllMetrics()
{
   m_cpuUsageMutex.lock();
   int nbCoresActual = collector->m_perCore.size();
   for (enum CpuUsageSource source = (enum CpuUsageSource)0; source < CPU_USAGE_NB_SOURCES; source = (enum CpuUsageSource)((int)source + 1))
   {
      for (enum CpuUsageInterval interval = INTERVAL_1MIN; interval <= INTERVAL_15MIN; interval = (enum CpuUsageInterval)((int)interval + 1))
      {
         int count;
         switch(interval)
         {
            case INTERVAL_5MIN: count = 5 * 60; break;
            case INTERVAL_15MIN: count = 15 * 60; break;
            default: count = 60; break;
         }

         for (int coreIndex = 0; coreIndex < nbCoresActual; coreIndex++)
         {
            float usage = collector->GetCoreUsage(source, coreIndex, count);
            (void)usage;
         }
         float usage = collector->GetTotalUsage(source, count);
         (void)usage;
      }
   }
   m_cpuUsageMutex.unlock();
}

void TestCpu()
{
   StartTest(_T("CPU stats collector - single threaded work"));
   assert(collector == nullptr);
   collector = new Collector();

   // We have to let it populate the tables with at least one delta value, otherwise it assers
   // For this, two readings are necessary
   collector->Collect();

   for (int i = 0; i < CPU_USAGE_SLOTS * 2; i++)
   {
      collector->Collect();
      ServeAllMetrics();
   }
   delete(collector);
   collector = nullptr;
   EndTest();

   StartTest(_T("CPU stats collector - multi-threaded work"));
   StartCpuUsageCollector();
   ThreadSleepMs(2000);
   INT64 start = GetCurrentTimeMs();
   while (GetCurrentTimeMs() - start < CPU_USAGE_SLOTS * collectionPeriodMs * 2)
   {
      ServeAllMetrics();
   }
   ShutdownCpuUsageCollector();
   delete(collector);
   EndTest();

}
