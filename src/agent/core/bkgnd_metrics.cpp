/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2025 Raden Solutions
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
** File: bkgnd_metrics.cpp
**
**/

#include "nxagentd.h"

/**
 * Executor for background external metric
 */
class CachedExecutor : public LineOutputProcessExecutor
{
private:
   TCHAR m_value[MAX_RESULT_LENGTH];
   Mutex m_valueLock;

protected:
   virtual void endOfOutput() override
   {
      LineOutputProcessExecutor::endOfOutput();
      const StringList& values = getData();
      m_valueLock.lock();
      _tcslcpy(m_value, !values.isEmpty() ? values.get(0) : _T(""), MAX_RESULT_LENGTH);
      m_valueLock.unlock();
   }

public:
   CachedExecutor(const TCHAR *cmdLine) : LineOutputProcessExecutor(cmdLine), m_valueLock(MutexType::FAST) {}

   String getValue() const
   {
      m_valueLock.lock();
      String value(m_value);
      m_valueLock.unlock();
      return value;
   }
};

/**
 * Background external metric
 */
class BackgroundExternalMetric
{
protected:
   TCHAR *m_cmdTemplate;
   uint32_t m_pollingInterval;
   Mutex m_mutex;
   StringObjectMap<CachedExecutor> m_executors;

public:
   BackgroundExternalMetric(const TCHAR *cmdTemplate, uint32_t pollingInterval) : m_mutex(MutexType::FAST), m_executors(Ownership::True)
   {
      m_cmdTemplate = MemCopyString(cmdTemplate);
      m_pollingInterval = pollingInterval * 1000;
   }
   ~BackgroundExternalMetric()
   {
      MemFree(m_cmdTemplate);
   }

   void poll();

   void getValue(const TCHAR *metric, TCHAR *value);
};

/**
 * Poll background metric
 */
void BackgroundExternalMetric::poll()
{
   if (g_dwFlags & AF_SHUTDOWN)
      return;

   m_executors.forEach(
      [] (const TCHAR *cmd, CachedExecutor *executor) -> EnumerationCallbackResult
      {
         if (!executor->isRunning())
            executor->execute();
         return _CONTINUE;
      });

   ThreadPoolScheduleRelative(g_executorThreadPool, m_pollingInterval, this, &BackgroundExternalMetric::poll);
}

/**
 * Get value from backgroundexecutor
 */
void BackgroundExternalMetric::getValue(const TCHAR *metric, TCHAR *value)
{
   m_mutex.lock();

   // Check if an executor for the given cmdline exists in the map
   CachedExecutor *executor = m_executors.get(metric);
   if (executor == nullptr)
   {
      // Substitute $1 .. $9 with actual arguments
      StringBuffer cmdLine;
      cmdLine.setAllocationStep(1024);

      for (const TCHAR *sptr = m_cmdTemplate; *sptr != 0; sptr++)
      {
         if (*sptr == _T('$'))
         {
            sptr++;
            if (*sptr == 0)
               break;   // Single $ character at the end of line
            if ((*sptr >= _T('1')) && (*sptr <= _T('9')))
            {
               TCHAR buffer[1024];
               if (AgentGetParameterArg(metric, *sptr - '0', buffer, 1024))
               {
                  cmdLine.append(buffer);
               }
            }
            else
            {
               cmdLine.append(*sptr);
            }
         }
         else
         {
            cmdLine.append(*sptr);
         }
      }

      // Create a new executor for the cmdline on the heap and store it in the map
      executor = new CachedExecutor(cmdLine);
      m_executors.set(metric, executor);
   }
   m_mutex.unlock();

   _tcscpy(value, executor->getValue());
}

/**
 * List of configured background metrics
 */
static ObjectArray<BackgroundExternalMetric> s_backgroundMetrics(0, 8, Ownership::True);

/**
 * Handler function for external (user-defined) metrics
 */
static LONG H_BackgroundExternalMetric(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   session->debugPrintf(4, _T("H_BackgroundExternalMetric called for \"%s\""), cmd);
   auto metric = reinterpret_cast<BackgroundExternalMetric*>(const_cast<TCHAR*>(arg));
   metric->getValue(cmd, value);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Add external metric or list
 */
bool AddBackgroundExternalMetric(TCHAR *config)
{
   TCHAR *intervalStr = _tcschr(config, _T(':'));
   if (intervalStr == nullptr)
      return false;

   *intervalStr = 0;
   intervalStr++;

   TCHAR *cmdTemplate = _tcschr(intervalStr, _T(':'));
   if (cmdTemplate == nullptr)
      return false;

   *cmdTemplate = 0;
   cmdTemplate++;

   Trim(config);
   Trim(intervalStr);
   Trim(cmdTemplate);
   if ((*config == 0) || (*intervalStr == 0) || (*cmdTemplate == 0))
      return false;

   uint32_t interval = _tcstoul(intervalStr, nullptr, 0);
   auto metric = new BackgroundExternalMetric(cmdTemplate, interval);
   s_backgroundMetrics.add(metric);
   AddMetric(config, H_BackgroundExternalMetric, reinterpret_cast<TCHAR*>(metric), DCI_DT_STRING, _T(""), nullptr);

   return true;
}

/**
 * Start background metrics
*/
void StartBackgroundMetricCollection()
{
   for(int i = 0; i < s_backgroundMetrics.size(); i++)
   {
      BackgroundExternalMetric *p = s_backgroundMetrics.get(i);
      ThreadPoolExecute(g_executorThreadPool, p, &BackgroundExternalMetric::poll);
   }
}
