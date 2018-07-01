/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2011 Victor Kirhenshtein
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
   if (value != NULL)
   {
		nx_strncpy(buffer, value, MAX_RESULT_LENGTH);
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
	   nxlog_debug(4, _T("ParamProvider::poll(): started command \"%s\""), m_executor->getCommand());
	   if (m_executor->waitForCompletion(g_eppTimeout * 1000))
	   {
         lock();
         delete m_parameters;
         m_parameters = new StringMap(*m_executor->getData());
         unlock();
         nxlog_debug(4, _T("ParamProvider::poll(): command \"%s\" execution completed, %d values read"), m_executor->getCommand(), (int)m_parameters->size());
	   }
	   else
	   {
         nxlog_debug(4, _T("ParamProvider::poll(): command \"%s\" execution timeout (%d seconds)"), m_executor->getCommand(), g_eppTimeout);
         m_executor->stop();
	   }
	}
	m_lastPollTime = time(NULL);
}

/**
 * Parameter list callback data
 */
struct ParameterListCallbackData
{
   NXCPMessage *msg;
   UINT32 id;
   UINT32 count;
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
static ObjectArray<ParameterProvider> s_providers(0, 8, true);

/**
 * Poller thread
 */
static THREAD_RESULT THREAD_CALL PollerThread(void *arg)
{
	while(!(g_dwFlags & AF_SHUTDOWN))
	{
		ThreadSleep(1);
		time_t now = time(NULL);
		for(int i = 0; i < s_providers.size(); i++)
		{
			ParameterProvider *p = s_providers.get(i);
			if (now > p->getLastPollTime() + (time_t)p->getPollInterval())
				p->poll();
		}
	}
	return THREAD_OK;
}

/**
 * Start poller thread
 */
void StartParamProvidersPoller()
{
	if (s_providers.size() > 0)
		ThreadCreate(PollerThread, 0, NULL);
	else
		DebugPrintf(2, _T("External parameters providers poller thread will not start"));
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

	nx_strncpy(buffer, line, 1024);
	TCHAR *ptr = _tcsrchr(buffer, _T(':'));
	if (ptr != NULL)
	{
		*ptr = 0;
		ptr++;
		TCHAR *eptr;
		interval = _tcstol(ptr, &eptr, 0);
		if ((*eptr != 0) || (interval < 1))
		{
			DebugPrintf(2, _T("Invalid interval value given for parameters provider"));
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
