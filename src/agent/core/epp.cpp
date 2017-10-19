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

class ParamExec : public CommandExec
{
private:
   StringMap m_parameters;
   bool m_isRunning;
   String m_buffer;

public:
   ParamExec(const TCHAR *command);

   const StringMap *getParameters() const { return &m_parameters; }

   bool isRunning() const { return m_isRunning; }

   bool execute() { m_isRunning = true; m_parameters.clear(); return CommandExec::execute(); }
   void stop() { m_isRunning = false; }

   virtual void onOutput(const char *text);
   virtual void endOfOutput();
};

/**
 * Create new parameter executor object
 */
ParamExec::ParamExec(const TCHAR *command) : CommandExec(command)
{
   m_sendOutput = true;
   m_isRunning = false;
}

/**
 * Parameter executor output handler
 */
void ParamExec::onOutput(const char *text)
{
   m_isRunning = true;

   if (text != NULL)
   {
      TCHAR *buffer;
#ifdef UNICODE
      buffer = WideStringFromMBStringSysLocale(text);
#else
      buffer = _tcsdup(text);
#endif
      TCHAR *newLinePtr = NULL, *lineStartPtr = buffer, *eqPtr = NULL;
      do
      {
         newLinePtr = _tcschr(lineStartPtr, _T('\n'));
         if (newLinePtr != NULL)
         {
            *newLinePtr = 0;
            m_buffer.append(lineStartPtr);
            if (m_buffer.length() > MAX_RESULT_LENGTH*3)
            {
               nxlog_debug(4, _T("ParamExec::onOutput(): result too long - %s"), (const TCHAR *)m_buffer);
               stop();
               m_buffer.clear();
               break;
            }
         }
         else
         {
            m_buffer.append(lineStartPtr);
            if (m_buffer.length() > MAX_RESULT_LENGTH*3)
            {
               nxlog_debug(4, _T("ParamExec::onOutput(): result too long - %s"), (const TCHAR *)m_buffer);
               stop();
               m_buffer.clear();
               break;
            }
            break;
         }

         eqPtr = _tcschr((const TCHAR *)m_buffer, _T('='));
         if (eqPtr != NULL)
         {
            *eqPtr = 0;
            eqPtr++;
            m_parameters.set((const TCHAR *)m_buffer, eqPtr);
         }
         m_buffer.clear();
         lineStartPtr = newLinePtr+1;
      } while (*lineStartPtr != 0);

      free(buffer);
   }
}

/**
 * End of output callback
 */
void ParamExec::endOfOutput()
{
   if (m_buffer.length() > 0)
   {
      TCHAR *ptr = _tcschr((const TCHAR *)m_buffer, _T('='));
      if (ptr != NULL)
      {
         *ptr = 0;
         ptr++;
         m_parameters.set((const TCHAR *)m_buffer, ptr);
      }
      m_buffer.clear();
   }
   m_isRunning = false;
}


/**
 * External parameter provider
 */
class ParamProvider
{
private:
	int m_pollInterval;
	time_t m_lastPollTime;
	ParamExec *m_paramExec;
   StringMap *m_parameters;
   MUTEX m_mutex;

   void lock() { MutexLock(m_mutex); }
   void unlock() { MutexUnlock(m_mutex); }

public:
	ParamProvider(const TCHAR *command, int pollInterval);
	~ParamProvider();

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
ParamProvider::ParamProvider(const TCHAR *command, int pollInterval)
{
	m_pollInterval = pollInterval;
	m_lastPollTime = 0;
	m_paramExec = new ParamExec(command);
	m_parameters = new StringMap();
   m_mutex = MutexCreate();
}

/**
 * Destructor
 */
ParamProvider::~ParamProvider()
{
	delete m_paramExec;
	delete m_parameters;
	MutexDestroy(m_mutex);
}

/**
 * Get parameter's value
 */
LONG ParamProvider::getValue(const TCHAR *name, TCHAR *buffer)
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
void ParamProvider::poll()
{
	if (m_paramExec->execute())
	{
	   nxlog_debug(4, _T("ParamProvider::poll(): started command \"%s\""), m_paramExec->getCommand());
	   bool timeout = false;
	   time_t start = time(NULL);
	   while(true)
	   {
	      if (((time(NULL) - start) < g_eppTimeout) && m_paramExec->isRunning())
	            ThreadSleepMs(10);
	      else
	      {
	         if (m_paramExec->isRunning())
	         {
	            nxlog_debug(4, _T("ParamProvider::poll(): The timeout of %d seconds has been reached for command: %s"), g_eppTimeout, m_paramExec->getCommand());
	            timeout = true;
	            m_paramExec->stop();
	         }
	         break;
	      }
	   }

	   if (!timeout)
	   {
         lock();
         delete m_parameters;
         m_parameters = new StringMap(*m_paramExec->getParameters());
         unlock();
	   }

	   nxlog_debug(4, _T("ParamProvider::poll(): command \"%s\" execution completed, %d values read"), m_paramExec->getCommand(), (int)m_parameters->size());
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
void ParamProvider::listParameters(NXCPMessage *msg, UINT32 *baseId, UINT32 *count)
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
void ParamProvider::listParameters(StringList *list)
{
	lock();
	m_parameters->forEach(ParameterListCallback2, list);
	unlock();
}

/**
 * Static data
 */
static ObjectArray<ParamProvider> s_providers(0, 8, true);

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
			ParamProvider *p = s_providers.get(i);
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

	ParamProvider *p = new ParamProvider(buffer, interval);
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
		ParamProvider *p = s_providers.get(i);
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
