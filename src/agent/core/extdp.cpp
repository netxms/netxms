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
** File: epp.cpp
**
**/

#include "nxagentd.h"
#include <nxsde.h>

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
   uint32_t m_timeout;
   time_t m_lastPollTime;
   TCHAR *m_command;
   ProcessExecutor *m_executor;
   Mutex m_mutex;

   void lock() { m_mutex.lock(); }
   void unlock() { m_mutex.unlock(); }

   virtual ProcessExecutor *createExecutor() = 0;
   virtual void processPollResults() = 0;

public:
   ExternalDataProvider(const TCHAR *command, uint32_t pollingInterval, uint32_t timeout);
   virtual ~ExternalDataProvider();

   virtual void listParameters(NXCPMessage *msg, uint32_t *baseId, uint32_t *count) { }
   virtual void listParameters(StringList *list) { }
   virtual void listLists(NXCPMessage *msg, uint32_t *baseId, uint32_t *count) { }
   virtual void listLists(StringList *list) { }
   virtual void listTables(NXCPMessage *msg, uint32_t *baseId, uint32_t *count) { }
   virtual void listTables(StringList *list) { }

   virtual LONG getValue(const TCHAR *name, TCHAR *buffer) { return SYSINFO_RC_UNKNOWN; }
   virtual LONG getList(const TCHAR *name, StringList *value) { return SYSINFO_RC_UNKNOWN; }
   virtual LONG getTableValue(const TCHAR *name, Table *table) { return SYSINFO_RC_UNKNOWN; }

   void init();
   void poll();
   void abort();

   time_t getLastPollTime() const { return m_lastPollTime; }
   uint32_t getPollingInterval() const { return m_pollingInterval; }
};

/**
 * Constructor
 */
ExternalDataProvider::ExternalDataProvider(const TCHAR *command, uint32_t pollingInterval, uint32_t timeout)
{
   m_pollingInterval = pollingInterval;
   m_timeout = (timeout > 0) ? timeout : g_externalMetricProviderTimeout;
   m_lastPollTime = 0;
   m_command = MemCopyString(command);
   m_executor = nullptr;
}

/**
 * Destructor
 */
ExternalDataProvider::~ExternalDataProvider()
{
   MemFree(m_command);
   delete m_executor;
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
      if (m_executor->waitForCompletion(m_timeout))
      {
         if (m_executor->getExitCode() == 0)
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("ExternalDataProvider::poll(): command \"%s\" execution completed"), m_command);
            processPollResults();
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("ExternalDataProvider::poll(): command \"%s\" execution completed with error (exit code %d)"), m_command, m_executor->getExitCode());
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("ExternalDataProvider::poll(): command \"%s\" execution timeout (%u milliseconds)"), m_command, m_timeout);
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
class MetricProvider : public ExternalDataProvider
{
protected:
   StringMap *m_parameters;

   virtual ProcessExecutor *createExecutor() override;
   virtual void processPollResults() override;

public:
	MetricProvider(const TCHAR *command, uint32_t pollingInterval, uint32_t timeout);
	virtual ~MetricProvider();

   virtual void listParameters(NXCPMessage *msg, uint32_t *baseId, uint32_t *count) override;
   virtual void listParameters(StringList *list) override;

	virtual LONG getValue(const TCHAR *name, TCHAR *buffer) override;
};

/**
 * Constructor
 */
MetricProvider::MetricProvider(const TCHAR *command, uint32_t pollingInterval, uint32_t timeout) : ExternalDataProvider(command, pollingInterval, timeout)
{
   m_parameters = new StringMap();
}

/**
 * Destructor
 */
MetricProvider::~MetricProvider()
{
   delete m_parameters;
}

/**
 * Create executor
 */
ProcessExecutor *MetricProvider::createExecutor()
{
   return new KeyValueOutputProcessExecutor(m_command);
}

/**
 * Process poll result
 */
void MetricProvider::processPollResults()
{
   lock();
   delete m_parameters;
   m_parameters = new StringMap(static_cast<KeyValueOutputProcessExecutor*>(m_executor)->getData());
   unlock();
   nxlog_debug_tag(DEBUG_TAG, 4, _T("ParamProvider::poll(): command \"%s\" execution completed, %d values read"), m_executor->getCommand(), m_parameters->size());
}

/**
 * Get parameter's value
 */
LONG MetricProvider::getValue(const TCHAR *name, TCHAR *buffer)
{
	LONG rc = SYSINFO_RC_UNKNOWN;

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
 * List available parameters
 */
void MetricProvider::listParameters(NXCPMessage *msg, uint32_t *baseId, uint32_t *count)
{
	lock();
	m_parameters->forEach(
	   [msg, baseId, count] (const TCHAR *key, void *value)
	   {
         msg->setField((*baseId)++, key);
         msg->setField((*baseId)++, _T(""));
         msg->setField((*baseId)++, static_cast<uint16_t>(DCI_DT_STRING));
         (*count)++;
         return _CONTINUE;
	   });
	unlock();
}

/**
 * List available parameters
 */
void MetricProvider::listParameters(StringList *list)
{
	lock();
	m_parameters->forEach(
	   [list] (const TCHAR *key, const void *value)
	   {
	      list->add(key);
         return _CONTINUE;
	   });
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
   TableProvider(const TCHAR *name, ExternalTableDefinition *definition, uint32_t pollingInterval, uint32_t timeout, const TCHAR *description);
   virtual ~TableProvider();

   virtual void listTables(NXCPMessage *msg, uint32_t *baseId, uint32_t *count) override;
   virtual void listTables(StringList *list) override;

   virtual LONG getTableValue(const TCHAR *name, Table *table) override;
};

/**
 * Table provider constructor
 */
TableProvider::TableProvider(const TCHAR *name, ExternalTableDefinition *definition, uint32_t pollingInterval, uint32_t timeout, const TCHAR *description) :
         ExternalDataProvider(definition->cmdLine, pollingInterval, timeout)
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
   return new LineOutputProcessExecutor(m_command, true);
}

/**
 * Process poll results for table provider
 */
void TableProvider::processPollResults()
{
   lock();
   delete m_value;
   m_value = new Table();
   if (!static_cast<LineOutputProcessExecutor*>(m_executor)->getData().isEmpty())
      ParseExternalTableData(*m_definition, static_cast<LineOutputProcessExecutor*>(m_executor)->getData(), m_value);
   else
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Empty output from command \"%s\""), m_command);
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
      return SYSINFO_RC_UNKNOWN;

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
 * External parameter provider
 */
class StructuredMetricProvider : public ExternalDataProvider
{
protected:
   String m_name;
   TCHAR m_genericParamName[MAX_PARAM_NAME];
   String m_description;
   StringObjectMap<StructuredExtractorParameterDefinition> *m_parameters;
   StringObjectMap<StructuredExtractorParameterDefinition> *m_lists;
   StructuredDataExtractor m_dataExtractor;
   bool m_forcePlainTextParser;

   virtual ProcessExecutor *createExecutor() override;
   virtual void processPollResults() override;

public:
   StructuredMetricProvider(const TCHAR *name, const TCHAR *command,
      StringObjectMap<StructuredExtractorParameterDefinition> *metricDefinitions,
      StringObjectMap<StructuredExtractorParameterDefinition> *listDefinitions,
      bool forcePlainTextParser, uint32_t pollingInterval, uint32_t timeout, const TCHAR *description);
   virtual ~StructuredMetricProvider();

   virtual void listParameters(NXCPMessage *msg, uint32_t *baseId, uint32_t *count) override;
   virtual void listParameters(StringList *list) override;
   virtual void listLists(NXCPMessage *msg, uint32_t *baseId, uint32_t *count) override;
   virtual void listLists(StringList *list) override;

   virtual LONG getValue(const TCHAR *name, TCHAR *buffer) override;
   virtual LONG getList(const TCHAR *name, StringList *value) override;
};

/**
 * Constructor
 */
StructuredMetricProvider::StructuredMetricProvider(const TCHAR *name, const TCHAR *command,
                                             StringObjectMap<StructuredExtractorParameterDefinition> *metricDefinitions,
                                             StringObjectMap<StructuredExtractorParameterDefinition> *listDefinitions,
                                             bool forcePlainTextParser, uint32_t pollingInterval, uint32_t timeout, const TCHAR *description) :
         ExternalDataProvider(command, pollingInterval, timeout), m_name(name), m_description(description), m_dataExtractor(name)
{
   m_forcePlainTextParser = forcePlainTextParser;
   m_parameters = metricDefinitions;
   m_lists = listDefinitions;
   _tcslcpy(m_genericParamName, name, MAX_PARAM_NAME);
   _tcslcat(m_genericParamName, _T("(*)"), MAX_PARAM_NAME);
}

/**
 * Destructor
 */
StructuredMetricProvider::~StructuredMetricProvider()
{
   delete m_parameters;
   delete m_lists;
}

/**
 * Create executor
 */
ProcessExecutor *StructuredMetricProvider::createExecutor()
{
   return new OutputCapturingProcessExecutor(m_command, true);
}

/**
 * Process poll result
 */
void StructuredMetricProvider::processPollResults()
{
   lock();
   OutputCapturingProcessExecutor *ex = static_cast<OutputCapturingProcessExecutor*>(m_executor);
   m_dataExtractor.updateContent(ex->getOutput(), ex->getOutputSize(), m_forcePlainTextParser, m_command);
   ex->clearOutput();
   unlock();
}

/**
 * Get parameter's value
 */
LONG StructuredMetricProvider::getValue(const TCHAR *name, TCHAR *buffer)
{
   LONG rc = SYSINFO_RC_UNKNOWN;

   lock();
   const TCHAR *bracketPos = _tcschr(name, _T('('));
   String cleanName = (bracketPos != nullptr) ? String(name, bracketPos - name) : String(name);
   StructuredExtractorParameterDefinition *definition = m_parameters->get(cleanName);
   if (definition != nullptr)
   {
      if (bracketPos != nullptr && definition->isParametrized)
      {
         StringBuffer cmdLine = SubstituteCommandArguments(definition->query, name);
         rc = m_dataExtractor.getMetric(cmdLine, buffer, MAX_RESULT_LENGTH);
      }
      else
      {
         rc = m_dataExtractor.getMetric(definition->query, buffer, MAX_RESULT_LENGTH);
      }
   }
   else if (MatchString(m_genericParamName, name, false))
   {
      TCHAR query[1024];
      AgentGetParameterArg(name, 1, query, 1024);
      rc = m_dataExtractor.getMetric(query, buffer, MAX_RESULT_LENGTH);
   }

   unlock();
   return rc;
}

/**
 * Get list's value
 */
LONG StructuredMetricProvider::getList(const TCHAR *name, StringList *value)
{
   lock();

   LONG rc = SYSINFO_RC_UNKNOWN;
   StructuredExtractorParameterDefinition *definition = m_lists->get(name);
   if (definition != nullptr)
   {
      rc = m_dataExtractor.getList(definition->query, value);
   }
   else if (MatchString(m_genericParamName, name, false))
   {
      TCHAR query[1024];
      AgentGetParameterArg(name, 1, query, 1024);
      rc = m_dataExtractor.getList(query, value);
   }

   unlock();
   return rc;
}

/**
 * List available parameters
 */
void StructuredMetricProvider::listParameters(NXCPMessage *msg, uint32_t *baseId, uint32_t *count)
{
   lock();
   m_parameters->forEach(
      [msg, baseId, count] (const TCHAR *key, StructuredExtractorParameterDefinition *value)
      {
         if(value->isParametrized)
         {
            StringBuffer paramName(key);
            paramName.append(_T("(*)"));
            msg->setField((*baseId)++, paramName);
         }
         else
         {
            msg->setField((*baseId)++, key);
         }
         msg->setField((*baseId)++, value->description);
         msg->setField((*baseId)++, static_cast<uint16_t>(value->dataType));
         (*count)++;
         return _CONTINUE;

      });
   unlock();

   msg->setField((*baseId)++, m_genericParamName);
   msg->setField((*baseId)++, m_description);
   msg->setField((*baseId)++, static_cast<uint16_t>(DCI_DT_STRING));
   (*count)++;
}

/**
 * List available parameters
 */
void StructuredMetricProvider::listParameters(StringList *list)
{
   lock();
   m_parameters->forEach(
      [list] (const TCHAR *key, StructuredExtractorParameterDefinition *value)
      {
         if(value->isParametrized)
         {
            StringBuffer paramName(key);
            paramName.append(_T("(*)"));
            list->add(paramName);
         }
         else
         {
            list->add(key);
         }
         return _CONTINUE;

      });
   list->add(m_genericParamName);
   unlock();
}

/**
 * List available lists
 */
void StructuredMetricProvider::listLists(NXCPMessage *msg, uint32_t *baseId, uint32_t *count)
{
   lock();
   m_lists->forEach(
      [msg, baseId, count](const TCHAR *key, void *value)
      {
         msg->setField((*baseId)++, key);
         (*count)++;
         return _CONTINUE;

      });
   unlock();

   msg->setField((*baseId)++, m_genericParamName);
   (*count)++;
}

/**
 * List available lists
 */
void StructuredMetricProvider::listLists(StringList *list)
{
   lock();
   list->addAll(m_lists->keys());
   list->add(m_genericParamName);
   unlock();
}

/**
 * Static data
 */
static ObjectArray<ExternalDataProvider> s_providers(0, 8, Ownership::True);

/**
 * Start providers
 */
void StartExternalMetricProviders()
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
void StopExternalMetricProviders()
{
   for(int i = 0; i < s_providers.size(); i++)
      s_providers.get(i)->abort();
}

/**
 * Add new external metric provider from configuration. Expects input in form
 * command:interval,timeout
 * Interval and timeout may be omitted.
 */
bool AddMetricProvider(const TCHAR *line)
{
	TCHAR buffer[1024];
	uint32_t interval = 60;
	uint32_t timeout = 0;

	_tcslcpy(buffer, line, 1024);
	TCHAR *ptr = _tcsrchr(buffer, _T(':'));
	if (ptr != nullptr)
	{
		*ptr = 0;
		ptr++;

      TCHAR *eptr;

	   TCHAR *nptr = _tcschr(ptr, _T(','));
	   if (nptr != nullptr)
	   {
	      *nptr = 0;
	      nptr++;

	      timeout = _tcstoul(nptr, &eptr, 0);
	      if ((*eptr != 0) || (timeout < 1))
	      {
	         nxlog_debug_tag(DEBUG_TAG, 2, _T("Invalid timeout value \"%s\" for external metric provider"), nptr);
	         return false;
	      }
	   }

		interval = _tcstoul(ptr, &eptr, 0);
		if ((*eptr != 0) || (interval < 1))
		{
         nxlog_debug_tag(DEBUG_TAG, 2, _T("Invalid interval value \"%s\" for external metric provider"), ptr);
			return false;
		}
	}

	s_providers.add(new MetricProvider(buffer, interval, timeout));
	return true;
}

/**
 * Add new external table provider
 */
void AddTableProvider(const TCHAR *name, ExternalTableDefinition *definition, uint32_t pollingInterval, uint32_t timeout, const TCHAR *description)
{
   s_providers.add(new TableProvider(name, definition, pollingInterval, timeout, description));
}

/**
 * Add new external table provider
 */
void AddStructuredMetricProvider(const TCHAR *name, const TCHAR *command, StringObjectMap<StructuredExtractorParameterDefinition> *metricDefinitions, StringObjectMap<StructuredExtractorParameterDefinition> *listDefinitions, bool forcePlainTextParser, uint32_t pollingInterval, uint32_t timeout, const TCHAR *description)
{
   s_providers.add(new StructuredMetricProvider(name, command, metricDefinitions, listDefinitions, forcePlainTextParser, pollingInterval, timeout, description));
}

/**
 * Get value from provider
 *
 * @return SYSINFO_RC_SUCCESS or SYSINFO_RC_UNSUPPORTED
 */
LONG GetParameterValueFromExtProvider(const TCHAR *name, TCHAR *buffer)
{
	LONG rc = SYSINFO_RC_UNKNOWN;
	for(int i = 0; i < s_providers.size(); i++)
	{
		ExternalDataProvider *p = s_providers.get(i);
		rc = p->getValue(name, buffer);
		if (rc != SYSINFO_RC_UNKNOWN)
			break;
	}
	return rc;
}

/**
 * Get list from provider
 *
 * @return SYSINFO_RC_SUCCESS or SYSINFO_RC_UNSUPPORTED
 */
LONG GetListValueFromExtProvider(const TCHAR *name, StringList *buffer)
{
   LONG rc = SYSINFO_RC_UNKNOWN;
   for(int i = 0; i < s_providers.size(); i++)
   {
      ExternalDataProvider *p = s_providers.get(i);
      rc = p->getList(name, buffer);
      if (rc != SYSINFO_RC_UNKNOWN)
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
   LONG rc = SYSINFO_RC_UNKNOWN;
   for(int i = 0; i < s_providers.size(); i++)
   {
      ExternalDataProvider *p = s_providers.get(i);
      rc = p->getTableValue(name, table);
      if (rc != SYSINFO_RC_UNKNOWN)
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
 * Add lists from external providers to NXCP message
 */
void ListListsFromExtProviders(NXCPMessage *msg, uint32_t *baseId, uint32_t *count)
{
   for(int i = 0; i < s_providers.size(); i++)
   {
      s_providers.get(i)->listLists(msg, baseId, count);
   }
}

/**
 * Add lists from external providers to string list
 */
void ListListsFromExtProviders(StringList *list)
{
   for(int i = 0; i < s_providers.size(); i++)
   {
      s_providers.get(i)->listLists(list);
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
