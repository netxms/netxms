/*
 ** MQTT subagent
 ** Copyright (C) 2017-2025 Raden Solutions
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
#include "mqtt_subagent.h"

/**
 * Topic constructor
 */
Topic::Topic(const TCHAR *pattern, const TCHAR *event)
{
   m_pattern = UTF8StringFromTString(pattern);
   m_event = MemCopyString(event);
   m_lastName[0] = 0;
   m_lastValue[0] = 0;
   m_timestamp = 0;
   m_parameters = nullptr;
   m_lists = nullptr;
   m_dataExtractor = nullptr;
   m_parseAsText = false;
}

/**
 * Topic constructor
 */
Topic::Topic(const char *pattern)
{
   m_pattern = MemCopyStringA(pattern);
   m_event = nullptr;
   m_lastName[0] = 0;
   m_lastValue[0] = 0;
   m_timestamp = 0;
   m_parameters = nullptr;
   m_lists = nullptr;
   m_dataExtractor = nullptr;
   m_parseAsText = false;
}

/**
 * Topic constructor for structured parser
 */
Topic::Topic(const TCHAR *pattern, const TCHAR *name, bool parseAsText)
{
   m_pattern = UTF8StringFromTString(pattern);
   m_event = nullptr;
   m_lastName[0] = 0;
   m_lastValue[0] = 0;
   m_timestamp = 0;
   m_parameters = nullptr;
   m_lists = nullptr;
   m_dataExtractor = new StructuredDataExtractor(pattern);
   m_parseAsText = parseAsText;
   _tcsncpy(m_genericParamName, name, MAX_PARAM_NAME);
   _tcsncat(m_genericParamName, _T("(*)"), MAX_PARAM_NAME);
}

/**
 * Topic destructor
 */
Topic::~Topic()
{
   MemFree(m_pattern);
   MemFree(m_event);
   delete m_dataExtractor;
   delete m_parameters;
   delete m_lists;
}

/**
 * Process broker message
 */
void Topic::processMessage(const char *topic, const char *msg)
{
   bool match;
   if (mosquitto_topic_matches_sub(m_pattern, topic, &match) != MOSQ_ERR_SUCCESS)
      return;
   if (!match)
      return;

   if (m_event != nullptr)
   {
      AgentPostEvent(0, m_event, 0, StringMap().setMBString(_T("topic"), topic).setMBString(_T("message"), msg));
   }
   else
   {
      m_mutex.lock();
      strlcpy(m_lastName, topic, MAX_DB_STRING);
      strlcpy(m_lastValue, msg, MAX_RESULT_LENGTH);
      m_timestamp = time(nullptr);
      if (m_dataExtractor != nullptr)
      {
         TCHAR buffer[512];
#ifdef UNICODE
         utf8_to_wchar(topic, -1, buffer, 512);
         buffer[511] = 0;
#else
         strlcpy(buffer, topic, 512);
#endif
         m_dataExtractor->updateContent(msg, strlen(msg), m_parseAsText, buffer);
      }
      m_mutex.unlock();
   }
}

/**
 * Retrieve collected data
 */
bool Topic::retrieveData(TCHAR *buffer, size_t bufferLen)
{
   LockGuard lockGuard(m_mutex);

   if ((m_timestamp == 0) || (m_lastName[0] == 0))
   {
      m_mutex.unlock();
      return false;
   }

#ifdef UNICODE
   utf8_to_wchar(m_lastValue, -1, buffer, bufferLen);
   buffer[bufferLen - 1] = 0;
#else
   strlcpy(buffer, m_lastValue, bufferLen);
#endif

   return true;
}

/**
 * Get topic data as structured data
 */
LONG Topic::retrieveData(const TCHAR *metricName, TCHAR *buffer, size_t bufferLen)
{
   LockGuard lockGuard(m_mutex);

   LONG rc = SYSINFO_RC_UNKNOWN;
   const TCHAR *query = m_parameters->get(metricName);
   if (query != nullptr)
   {
      rc = m_dataExtractor->getMetric(query, buffer, MAX_RESULT_LENGTH);
   }
   else if (MatchString(m_genericParamName, metricName, false))
   {
      TCHAR query[1024];
      AgentGetParameterArg(metricName, 1, query, 1024);
      rc = m_dataExtractor->getMetric(query, buffer, MAX_RESULT_LENGTH);
   }

   return rc;
}

/**
 * Get topic data as structured list data
 */
LONG Topic::retrieveListData(const TCHAR *metricName, StringList *buffer)
{
   LockGuard lockGuard(m_mutex);

   LONG rc = SYSINFO_RC_UNKNOWN;
   const TCHAR *query = m_lists->get(metricName);
   if (query != nullptr)
   {
      rc = m_dataExtractor->getList(query, buffer);
   }
   else if (MatchString(m_genericParamName, metricName, false))
   {
      TCHAR query[1024];
      AgentGetParameterArg(metricName, 1, query, 1024);
      rc = m_dataExtractor->getList(query, buffer);
   }

   return rc;
}
