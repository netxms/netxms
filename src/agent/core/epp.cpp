/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2020 Victor Kirhenshtein
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
** File: epp.cpp
**
**/

#include "nxagentd.h"

#define DEBUG_TAG _T("ext.provider")

/**
 * External parameter provider
 */
class ParameterProvider
{
private:
	int m_pollInterval;
	time_t m_lastPollTime;
	KeyValueOutputProcessExecutor *m_executor;
   StringMap *m_parameters;
   MUTEX m_mutex;

   void lock() { MutexLock(m_mutex); }
   void unlock() { MutexUnlock(m_mutex); }

public:
	ParameterProvider(const TCHAR *command, int pollInterval);
	~ParameterProvider();

	time_t getLastPollTime() { return m_lastPollTime; }
	int getPollInterval() { return m_pollInterval; }
	void poll();
   void abort();
	LONG getValue(const TCHAR *name, TCHAR *buffer);
	void listParameters(NXCPMessage *msg, UINT32 *baseId, UINT32 *count);
	void listParameters(StringList *list);
};

/**
 * Constructor
 */
ParameterProvider::ParameterProvider(const TCHAR *command, int pollInterval)
{
	m_pollInterval = pollInterval;
	m_lastPollTime = 0;
	m_executor = new KeyValueOutputProcessExecutor(command);
	m_parameters = new StringMap();
   m_mutex = MutexCreate();
}

/**
 * Destructor
 */
ParameterProvider::~ParameterProvider()
{
	delete m_executor;
	delete m_parameters;
	MutexDestroy(m_mutex);
}

/**
 * Get parameter's value
 */
LONG ParameterProvider::getValue(const TCHAR *name, TCHAR *buffer)
{
	LONG rc = SYSINFO_RC_UNSUPPORTED;

	lock();

   const TCHAR *value = m_parameters->get(name);
   if (value != nullptr)
   {
		_tcslcpy(buffer, value, MAX_RESULT_LENGTH);
		rc = SYSINFO_RC_SUCCESS;
	}

	unlock();
	return rc;
}

/**
 * Poll provider
 */
void ParameterProvider::poll()
{
	if (m_executor->execute())
	{
	   nxlog_debug_tag(DEBUG_TAG, 4, _T("ParamProvider::poll(): started command \"%s\""), m_executor->getCommand());
	   if (m_executor->waitForCompletion(g_eppTimeout * 1000))
	   {
         lock();
         delete m_parameters;
         m_parameters = new StringMap(m_executor->getData());
         unlock();
         nxlog_debug_tag(DEBUG_TAG, 4, _T("ParamProvider::poll(): command \"%s\" execution completed, %d values read"), m_executor->getCommand(), (int)m_parameters->size());
	   }
	   else
	   {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("ParamProvider::poll(): command \"%s\" execution timeout (%d seconds)"), m_executor->getCommand(), g_eppTimeout);
         m_executor->stop();
	   }
	}
	m_lastPollTime = time(nullptr);
}

/**
 * Abort running process if any
 */
void ParameterProvider::abort()
{
   m_executor->stop();
}

/**
 * Parameter list callback data
 */
struct ParameterListCallbackData
{
   NXCPMessage *msg;
   uint32_t id;
   uint32_t count;
};

/**
 * Parameter list callback
 */
static EnumerationCallbackResult ParameterListCallback(const TCHAR *key, const void *value, void *data)
{
	((ParameterListCallbackData *)data)->msg->setField(((ParameterListCallbackData *)data)->id++, key);
	((ParameterListCallbackData *)data)->msg->setField(((ParameterListCallbackData *)data)->id++, _T(""));
	((ParameterListCallbackData *)data)->msg->setField(((ParameterListCallbackData *)data)->id++, (WORD)DCI_DT_STRING);
	((ParameterListCallbackData *)data)->count++;
   return _CONTINUE;
}

/**
 * List available parameters
 */
void ParameterProvider::listParameters(NXCPMessage *msg, UINT32 *baseId, UINT32 *count)
{
   ParameterListCallbackData data;
   data.msg = msg;
   data.id = *baseId;
   data.count = 0;

	lock();
	m_parameters->forEach(ParameterListCallback, &data);
	unlock();

	*baseId = data.id;
   *count += data.count;
}

/**
 * Parameter list callback
 */
static EnumerationCallbackResult ParameterListCallback2(const TCHAR *key, const void *value, void *data)
{
   ((StringList *)data)->add(key);
   return _CONTINUE;
}

/**
 * List available parameters
 */
void ParameterProvider::listParameters(StringList *list)
{
	lock();
	m_parameters->forEach(ParameterListCallback2, list);
	unlock();
}

/**
 * Static data
 */
static ObjectArray<ParameterProvider> s_providers(0, 8, Ownership::True);
static THREAD s_pollerThread = INVALID_THREAD_HANDLE;
static ParameterProvider* volatile s_runningProvider = nullptr;

/**
 * Poller thread
 */
static void PollerThread()
{
   nxlog_debug_tag(DEBUG_TAG, 3, _T("External parameters providers poller thread started"));
   while(!(g_dwFlags & AF_SHUTDOWN))
	{
      if (SleepAndCheckForShutdown(1))
         break;
		time_t now = time(nullptr);
		for(int i = 0; (i < s_providers.size()) && !(g_dwFlags & AF_SHUTDOWN); i++)
		{
			ParameterProvider *p = s_providers.get(i);
         if (now > p->getLastPollTime() + static_cast<time_t>(p->getPollInterval()))
         {
            InterlockedExchangeObjectPointer(&s_runningProvider, p);
            s_runningProvider = p;
            p->poll();
            InterlockedExchangeObjectPointer(&s_runningProvider, static_cast<ParameterProvider*>(nullptr));
         }
		}
	}
   nxlog_debug_tag(DEBUG_TAG, 3, _T("External parameters providers poller thread stopped"));
}

/**
 * Start poller thread
 */
void StartExternalParameterProviders()
{
	if (s_providers.size() > 0)
		s_pollerThread = ThreadCreateEx(PollerThread);
	else
      nxlog_debug_tag(DEBUG_TAG, 2, _T("External parameters providers poller thread will not start"));
}

/**
 * Stop poller thread
 */
void StopExternalParameterProviders()
{
   if (s_pollerThread != INVALID_THREAD_HANDLE)
   {
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Waiting for external parameters providers poller thread to stop"));
      ParameterProvider *runningProvider = s_runningProvider;
      if (runningProvider != nullptr)
         runningProvider->abort();
      ThreadJoin(s_pollerThread);
   }
}

/**
 * Add new provider from config. Expects input in form
 * command:interval
 * Interval may be omited.
 */
bool AddParametersProvider(const TCHAR *line)
{
	TCHAR buffer[1024];
	int interval = 60;

	_tcslcpy(buffer, line, 1024);
	TCHAR *ptr = _tcsrchr(buffer, _T(':'));
	if (ptr != nullptr)
	{
		*ptr = 0;
		ptr++;
		TCHAR *eptr;
		interval = _tcstol(ptr, &eptr, 0);
		if ((*eptr != 0) || (interval < 1))
		{
         nxlog_debug_tag(DEBUG_TAG, 2, _T("Invalid interval value given for parameters provider"));
			return false;
		}
	}

	ParameterProvider *p = new ParameterProvider(buffer, interval);
	s_providers.add(p);

	return true;
}

/**
 * Get value from provider
 *
 * @return SYSINFO_RC_SUCCESS or SYSINFO_RC_UNSUPPORTED
 */
LONG GetParameterValueFromExtProvider(const TCHAR *name, TCHAR *buffer)
{
	LONG rc = SYSINFO_RC_UNSUPPORTED;
	for(int i = 0; i < s_providers.size(); i++)
	{
		ParameterProvider *p = s_providers.get(i);
		rc = p->getValue(name, buffer);
		if (rc == SYSINFO_RC_SUCCESS)
			break;
	}
	return rc;
}

/**
 * Add parameters from external providers to NXCP message
 */
void ListParametersFromExtProviders(NXCPMessage *msg, UINT32 *baseId, UINT32 *count)
{
	for(int i = 0; i < s_providers.size(); i++)
	{
		s_providers.get(i)->listParameters(msg, baseId, count);
	}
}

/**
 * Add parameters from external providers to string list
 */
void ListParametersFromExtProviders(StringList *list)
{
	for(int i = 0; i < s_providers.size(); i++)
	{
		s_providers.get(i)->listParameters(list);
	}
}
