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
 * Parse data for external table
 */
void ParseExternalTableData(const ExternalTableDefinition& td, const StringList& data, Table *table);

/**
 * Generic external data provider
 */
class ExternalDataProvider
{
protected:
   uint32_t m_pollingInterval;
   time_t m_lastPollTime;
   TCHAR *m_command;
   ProcessExecutor *m_executor;
   MUTEX m_mutex;

   void lock() { MutexLock(m_mutex); }
   void unlock() { MutexUnlock(m_mutex); }

   virtual ProcessExecutor *createExecutor() = 0;
   virtual void processPollResults() = 0;

public:
   ExternalDataProvider(const TCHAR *command, uint32_t pollingInterval);
   virtual ~ExternalDataProvider();

   virtual void listParameters(NXCPMessage *msg, uint32_t *baseId, uint32_t *count) { }
   virtual void listParameters(StringList *list) { }
   virtual void listTables(NXCPMessage *msg, uint32_t *baseId, uint32_t *count) { }
   virtual void listTables(StringList *list) { }

   virtual LONG getValue(const TCHAR *name, TCHAR *buffer) { return SYSINFO_RC_UNSUPPORTED; }
   virtual LONG getTableValue(const TCHAR *name, Table *table) { return SYSINFO_RC_UNSUPPORTED; }

   void init();
   void poll();
   void abort();

   time_t getLastPollTime() const { return m_lastPollTime; }
   uint32_t getPollingInterval() const { return m_pollingInterval; }
};

/**
 * Constructor
 */
ExternalDataProvider::ExternalDataProvider(const TCHAR *command, uint32_t pollingInterval)
{
   m_pollingInterval = pollingInterval;
   m_lastPollTime = 0;
   m_command = MemCopyString(command);
   m_executor = nullptr;
   m_mutex = MutexCreate();
}

/**
 * Destructor
 */
ExternalDataProvider::~ExternalDataProvider()
{
   MemFree(m_command);
   delete m_executor;
   MutexDestroy(m_mutex);
}

/**
 * Initialize provider
 */
void ExternalDataProvider::init()
{
   m_executor = createExecutor();
}

/**
 * Poll provider
 */
void ExternalDataProvider::poll()
{
   if (g_dwFlags & AF_SHUTDOWN)
      return;

   if (m_executor->execute())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("ExternalDataProvider::poll(): started command \"%s\""), m_executor->getCommand());
      if (m_executor->waitForCompletion(g_eppTimeout * 1000))
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("ExternalDataProvider::poll(): command \"%s\" execution completed"), m_command);
         processPollResults();
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("ExternalDataProvider::poll(): command \"%s\" execution timeout (%d seconds)"), m_command, g_eppTimeout);
         m_executor->stop();
      }
   }
   m_lastPollTime = time(nullptr);

   if (!(g_dwFlags & AF_SHUTDOWN))
      ThreadPoolScheduleRelative(g_executorThreadPool, m_pollingInterval * 1000, this, &ExternalDataProvider::poll);
}

/**
 * Abort running process if any
 */
void ExternalDataProvider::abort()
{
   nxlog_debug_tag(DEBUG_TAG, 4, _T("ExternalDataProvider::abort(): aborting execution for command \"%s\""), m_command);
   m_executor->stop();
}

/**
 * External parameter provider
 */
class ParameterProvider : public ExternalDataProvider
{
protected:
   StringMap *m_parameters;

   virtual ProcessExecutor *createExecutor() override;
   virtual void processPollResults() override;

public:
	ParameterProvider(const TCHAR *command, uint32_t pollingInterval);
	virtual ~ParameterProvider();

   virtual void listParameters(NXCPMessage *msg, uint32_t *baseId, uint32_t *count) override;
   virtual void listParameters(StringList *list) override;

	virtual LONG getValue(const TCHAR *name, TCHAR *buffer) override;
};

/**
 * Constructor
 */
ParameterProvider::ParameterProvider(const TCHAR *command, uint32_t pollingInterval) : ExternalDataProvider(command, pollingInterval)
{
   m_parameters = new StringMap();
}

/**
 * Destructor
 */
ParameterProvider::~ParameterProvider()
{
   delete m_parameters;
}

/**
 * Create executor
 */
ProcessExecutor *ParameterProvider::createExecutor()
{
   return new KeyValueOutputProcessExecutor(m_command);
}

/**
 * Process poll result
 */
void ParameterProvider::processPollResults()
{
   lock();
   delete m_parameters;
   m_parameters = new StringMap(static_cast<KeyValueOutputProcessExecutor*>(m_executor)->getData());
   unlock();
   nxlog_debug_tag(DEBUG_TAG, 4, _T("ParamProvider::poll(): command \"%s\" execution completed, %d values read"), m_executor->getCommand(), (int)m_parameters->size());
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
static EnumerationCallbackResult ParameterListCallback(const TCHAR *key, const TCHAR *value, ParameterListCallbackData *context)
{
	context->msg->setField(context->id++, key);
	context->msg->setField(context->id++, _T(""));
	context->msg->setField(context->id++, static_cast<uint16_t>(DCI_DT_STRING));
	context->count++;
   return _CONTINUE;
}

/**
 * List available parameters
 */
void ParameterProvider::listParameters(NXCPMessage *msg, uint32_t *baseId, uint32_t *count)
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
static EnumerationCallbackResult ParameterListCallback2(const TCHAR *key, const TCHAR *value, StringList *context)
{
   context->add(key);
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
 * External table provider
 */
class TableProvider : public ExternalDataProvider
{
private:
   TCHAR *m_name;
   TCHAR *m_description;
   ExternalTableDefinition *m_definition;
   Table *m_value;

   virtual ProcessExecutor *createExecutor() override;
   virtual void processPollResults() override;

public:
   TableProvider(const TCHAR *name, ExternalTableDefinition *definition, uint32_t pollingInterval, const TCHAR *description);
   virtual ~TableProvider();

   virtual void listTables(NXCPMessage *msg, uint32_t *baseId, uint32_t *count) override;
   virtual void listTables(StringList *list) override;

   virtual LONG getTableValue(const TCHAR *name, Table *table) override;
};

/**
 * Table provider constructor
 */
TableProvider::TableProvider(const TCHAR *name, ExternalTableDefinition *definition, uint32_t pollingInterval, const TCHAR *description) :
         ExternalDataProvider(&definition->cmdLine[1], pollingInterval)
{
   m_name = MemCopyString(name);
   m_description = MemCopyString(description);
   m_definition = definition;
   m_value = nullptr;
}

/**
 * Table provider destructor
 */
TableProvider::~TableProvider()
{
   MemFree(m_name);
   MemFree(m_description);
   delete m_definition;
   delete m_value;
}

/**
 * Create executor for table provider
 */
ProcessExecutor *TableProvider::createExecutor()
{
   return new LineOutputProcessExecutor(m_command, m_definition->cmdLine[0] == 'S');
}

/**
 * Process poll results for table provider
 */
void TableProvider::processPollResults()
{
   lock();
   delete m_value;
   m_value = new Table();
   ParseExternalTableData(*m_definition, static_cast<LineOutputProcessExecutor*>(m_executor)->getData(), m_value);
   unlock();
}

/**
 * List supported tables
 */
void TableProvider::listTables(NXCPMessage *msg, uint32_t *baseId, uint32_t *count)
{
   uint32_t fieldId = *baseId;
   msg->setField(fieldId++, m_name);
   if (m_definition->instanceColumnCount > 0)
   {
      StringBuffer instanceColumns;
      for(int i = 0; i < m_definition->instanceColumnCount; i++)
      {
         if (i > 0)
            instanceColumns.append(_T(","));
         instanceColumns.append(m_definition->instanceColumns[i]);
      }
      msg->setField(fieldId++, instanceColumns);
   }
   else
   {
      msg->setField(fieldId++, _T(""));
   }
   msg->setField(fieldId++, m_name);
   *baseId = fieldId;
   (*count)++;
}

/**
 * List supported tables
 */
void TableProvider::listTables(StringList *list)
{
   list->add(m_name);
}

/**
 * Get table value from this provider
 */
LONG TableProvider::getTableValue(const TCHAR *name, Table *table)
{
   if (_tcsicmp(name, m_name))
      return SYSINFO_RC_UNSUPPORTED;

   LONG rc;
   lock();
   if (m_value != nullptr)
   {
      table->merge(m_value);
      rc = SYSINFO_RC_SUCCESS;
   }
   else
   {
      rc = SYSINFO_RC_ERROR;
   }
   unlock();
   return rc;
}

/**
 * Static data
 */
static ObjectArray<ExternalDataProvider> s_providers(0, 8, Ownership::True);

/**
 * Start providers
 */
void StartExternalParameterProviders()
{
   for(int i = 0; (i < s_providers.size()) && !(g_dwFlags & AF_SHUTDOWN); i++)
   {
      ExternalDataProvider *p = s_providers.get(i);
      p->init();
      ThreadPoolExecute(g_executorThreadPool, p, &ExternalDataProvider::poll);
   }
}

/**
 * Stop providers
 */
void StopExternalParameterProviders()
{
   for(int i = 0; i < s_providers.size(); i++)
      s_providers.get(i)->abort();
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

	s_providers.add(new ParameterProvider(buffer, interval));
	return true;
}

/**
 * Add new external table provider
 */
void AddTableProvider(const TCHAR *name, ExternalTableDefinition *definition, uint32_t pollingInterval, const TCHAR *description)
{
   s_providers.add(new TableProvider(name, definition, pollingInterval, description));
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
		ExternalDataProvider *p = s_providers.get(i);
		rc = p->getValue(name, buffer);
		if (rc == SYSINFO_RC_SUCCESS)
			break;
	}
	return rc;
}

/**
 * Get table value from provider
 *
 * @return SYSINFO_RC_SUCCESS, SYSINFO_RC_ERROR, or SYSINFO_RC_UNSUPPORTED
 */
LONG GetTableValueFromExtProvider(const TCHAR *name, Table *table)
{
   LONG rc = SYSINFO_RC_UNSUPPORTED;
   for(int i = 0; i < s_providers.size(); i++)
   {
      ExternalDataProvider *p = s_providers.get(i);
      rc = p->getTableValue(name, table);
      if (rc != SYSINFO_RC_UNSUPPORTED)
         break;
   }
   return rc;
}

/**
 * Add parameters from external providers to NXCP message
 */
void ListParametersFromExtProviders(NXCPMessage *msg, uint32_t *baseId, uint32_t *count)
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

/**
 * Add parameters from external providers to NXCP message
 */
void ListTablesFromExtProviders(NXCPMessage *msg, uint32_t *baseId, uint32_t *count)
{
   for(int i = 0; i < s_providers.size(); i++)
   {
      s_providers.get(i)->listTables(msg, baseId, count);
   }
}

/**
 * Add parameters from external providers to string list
 */
void ListTablesFromExtProviders(StringList *list)
{
   for(int i = 0; i < s_providers.size(); i++)
   {
      s_providers.get(i)->listTables(list);
   }
}

/**
 * Get number of external data providers
 */
int GetExternalDataProviderCount()
{
   return s_providers.size();
}
