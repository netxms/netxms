/*
 ** MQTT subagent
 ** Copyright (C) 2017-2026 Raden Solutions
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
   m_name = nullptr;
   m_lastName[0] = 0;
   m_lastValue[0] = 0;
   m_genericParamName[0] = 0;
   m_instanceListName[0] = 0;
   m_timestamp = 0;
   m_parameters = nullptr;
   m_lists = nullptr;
   m_dataExtractor = nullptr;
   m_instances = nullptr;
   m_instanceSegment = -1;
   m_instanceQuery = nullptr;
   m_instanceTimeout = 0;
   m_maxInstances = 0;
   m_instanceLimitReported = false;
   m_parseAsText = false;
   m_truncationReported = false;
}

/**
 * Topic constructor
 */
Topic::Topic(const char *pattern)
{
   m_pattern = MemCopyStringA(pattern);
   m_event = nullptr;
   m_name = nullptr;
   m_lastName[0] = 0;
   m_lastValue[0] = 0;
   m_genericParamName[0] = 0;
   m_instanceListName[0] = 0;
   m_timestamp = 0;
   m_parameters = nullptr;
   m_lists = nullptr;
   m_dataExtractor = nullptr;
   m_instances = nullptr;
   m_instanceSegment = -1;
   m_instanceQuery = nullptr;
   m_instanceTimeout = 0;
   m_maxInstances = 0;
   m_instanceLimitReported = false;
   m_parseAsText = false;
   m_truncationReported = false;
}

/**
 * Topic constructor for structured parser
 */
Topic::Topic(const TCHAR *pattern, const TCHAR *name, bool parseAsText)
{
   m_pattern = UTF8StringFromTString(pattern);
   m_event = nullptr;
   m_name = MemCopyString(name);
   m_lastName[0] = 0;
   m_lastValue[0] = 0;
   m_timestamp = 0;
   m_parameters = nullptr;
   m_lists = nullptr;
   m_dataExtractor = new StructuredDataExtractor(pattern);
   m_instances = nullptr;
   m_instanceSegment = -1;
   m_instanceQuery = nullptr;
   m_instanceTimeout = 0;
   m_maxInstances = 0;
   m_instanceLimitReported = false;
   m_parseAsText = parseAsText;
   m_truncationReported = false;
   _tcslcpy(m_genericParamName, name, MAX_PARAM_NAME);
   _tcslcat(m_genericParamName, _T("(*)"), MAX_PARAM_NAME);
   _tcslcpy(m_instanceListName, name, MAX_PARAM_NAME);
   _tcslcat(m_instanceListName, _T(".Instances"), MAX_PARAM_NAME);
}

/**
 * Topic destructor
 */
Topic::~Topic()
{
   MemFree(m_pattern);
   MemFree(m_event);
   MemFree(m_name);
   MemFree(m_instanceQuery);
   delete m_dataExtractor;
   delete m_instances;
   delete m_parameters;
   delete m_lists;
}

/**
 * Switch topic to instance discovery mode
 */
void Topic::initInstanceMap(uint32_t timeout, int maxInstances)
{
   delete m_dataExtractor;
   m_dataExtractor = nullptr;
   m_instances = new StringObjectMap<StructuredDataExtractor>(Ownership::True);
   m_instances->setIgnoreCase(false);   // MQTT topics are case sensitive
   m_instanceTimeout = timeout;
   m_maxInstances = maxInstances;
}

/**
 * Enable instance discovery with instance name taken from topic segment
 */
void Topic::enableInstanceDiscoveryFromTopic(int segment, uint32_t timeout, int maxInstances)
{
   initInstanceMap(timeout, maxInstances);
   m_instanceSegment = segment;
}

/**
 * Enable instance discovery with instance name taken from message payload
 */
void Topic::enableInstanceDiscoveryFromPayload(const TCHAR *query, uint32_t timeout, int maxInstances)
{
   initInstanceMap(timeout, maxInstances);
   m_instanceQuery = MemCopyString(query);
}

/**
 * Extract instance name from given segment of topic name
 */
static bool InstanceFromTopic(const char *topic, int segment, TCHAR *buffer, size_t size)
{
   const char *curr = topic;
   for(int i = 0; i < segment; i++)
   {
      curr = strchr(curr, '/');
      if (curr == nullptr)
         return false;
      curr++;
   }

   const char *end = strchr(curr, '/');
   size_t len = (end != nullptr) ? static_cast<size_t>(end - curr) : strlen(curr);
   if ((len == 0) || (len >= size))
      return false;

   char instance[MAX_DB_STRING];
   memcpy(instance, curr, len);
   instance[len] = 0;
   utf8_to_tchar(instance, -1, buffer, size);
   buffer[size - 1] = 0;
   return true;
}

/**
 * Delete instances that were not updated within instance timeout. Should be called with topic mutex locked.
 */
void Topic::purgeExpiredInstances()
{
   if (m_instanceTimeout == 0)
      return;

   StringList expiredInstances;
   m_instances->forEach(
      [this, &expiredInstances] (const TCHAR *instance, StructuredDataExtractor *extractor) -> EnumerationCallbackResult
      {
         if (extractor->isDataExpired(m_instanceTimeout))
            expiredInstances.add(instance);
         return _CONTINUE;
      });

   for(int i = 0; i < expiredInstances.size(); i++)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Instance \"%s\" of MQTT extractor %s expired"), expiredInstances.get(i), m_name);
      m_instances->remove(expiredInstances.get(i));
   }
}

/**
 * Get data extractor for given instance. Returns nullptr if instance is unknown or expired.
 * Should be called with topic mutex locked.
 */
StructuredDataExtractor *Topic::getInstance(const TCHAR *instance)
{
   StructuredDataExtractor *extractor = m_instances->get(instance);
   if (extractor == nullptr)
      return nullptr;

   if ((m_instanceTimeout != 0) && extractor->isDataExpired(m_instanceTimeout))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Instance \"%s\" of MQTT extractor %s expired"), instance, m_name);
      m_instances->remove(instance);
      return nullptr;
   }

   return extractor;
}

/**
 * Check if new instance can be added to this topic. Should be called with topic mutex locked.
 */
bool Topic::canAddInstance()
{
   purgeExpiredInstances();

   if ((m_maxInstances <= 0) || (m_instances->size() < m_maxInstances))
      return true;

   if (!m_instanceLimitReported)
   {
      m_instanceLimitReported = true;
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Instance limit (%d) reached for MQTT extractor %s, new instances will be ignored"), m_maxInstances, m_name);
   }
   return false;
}

/**
 * Update instance content from received message. Should be called with topic mutex locked.
 */
void Topic::updateInstanceContent(const char *topic, const char *msg, size_t msgLen)
{
   if (m_instanceSegment >= 0)
   {
      TCHAR instance[MAX_DB_STRING];
      if (!InstanceFromTopic(topic, m_instanceSegment, instance, MAX_DB_STRING))
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Cannot extract instance name from topic \"%hs\" for MQTT extractor %s"), topic, m_name);
         return;
      }

      StructuredDataExtractor *extractor = m_instances->get(instance);
      if (extractor == nullptr)
      {
         if (!canAddInstance())
            return;
         extractor = new StructuredDataExtractor(m_name);
         m_instances->set(instance, extractor);
         nxlog_debug_tag(DEBUG_TAG, 5, _T("New instance \"%s\" registered for MQTT extractor %s"), instance, m_name);
      }
      extractor->updateContent(msg, msgLen, m_parseAsText, instance);
   }
   else
   {
      // Instance name is part of the payload, so document has to be parsed before instance can be determined
      StructuredDataExtractor *extractor = new StructuredDataExtractor(m_name);
      extractor->updateContent(msg, msgLen, m_parseAsText, m_name);

      TCHAR instance[MAX_DB_STRING];
      if ((extractor->getMetric(m_instanceQuery, instance, MAX_DB_STRING) != SYSINFO_RC_SUCCESS) || (instance[0] == 0))
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Cannot extract instance name from message on topic \"%hs\" for MQTT extractor %s"), topic, m_name);
         delete extractor;
         return;
      }

      if (!m_instances->contains(instance))
      {
         if (!canAddInstance())
         {
            delete extractor;
            return;
         }
         nxlog_debug_tag(DEBUG_TAG, 5, _T("New instance \"%s\" registered for MQTT extractor %s"), instance, m_name);
      }
      m_instances->set(instance, extractor);
   }
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
      size_t msgLen = strlen(msg);
      m_mutex.lock();
      strlcpy(m_lastName, topic, MAX_DB_STRING);
      if (msgLen < MAX_RESULT_LENGTH)
      {
         memcpy(m_lastValue, msg, msgLen + 1);
      }
      else
      {
         // Message is longer than maximum metric value length - truncate it at UTF-8 character boundary
         size_t len = MAX_RESULT_LENGTH - 1;
         while ((len > 0) && ((static_cast<unsigned char>(msg[len]) & 0xC0) == 0x80))
            len--;
         memcpy(m_lastValue, msg, len);
         m_lastValue[len] = 0;
         if (!m_truncationReported)
         {
            m_truncationReported = true;
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Message payload on topic \"%hs\" truncated from %u to %u bytes (maximum metric value length is %d bytes) - use structured data extractors to access complete payload"),
                     topic, static_cast<uint32_t>(msgLen), static_cast<uint32_t>(len), MAX_RESULT_LENGTH - 1);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 6, _T("Message payload on topic \"%hs\" truncated from %u to %u bytes"),
                     topic, static_cast<uint32_t>(msgLen), static_cast<uint32_t>(len));
         }
      }
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
         m_dataExtractor->updateContent(msg, msgLen, m_parseAsText, buffer);
      }
      else if (m_instances != nullptr)
      {
         updateInstanceContent(topic, msg, msgLen);
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
      return false;

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

   StructuredDataExtractor *dataExtractor = m_dataExtractor;
   int genericQueryArg = 1;
   if (m_instances != nullptr)
   {
      TCHAR instance[MAX_DB_STRING];
      if (!AgentGetParameterArg(metricName, 1, instance, MAX_DB_STRING) || (instance[0] == 0))
         return SYSINFO_RC_UNSUPPORTED;
      dataExtractor = getInstance(instance);
      if (dataExtractor == nullptr)
         return SYSINFO_RC_NO_SUCH_INSTANCE;
      genericQueryArg = 2;
   }

   LONG rc = SYSINFO_RC_UNKNOWN;
   const TCHAR *bracket = _tcschr(metricName, _T('('));
   String baseName = (bracket != nullptr) ? String(metricName, bracket - metricName) : String(metricName);
   const TCHAR *query = m_parameters->get(baseName);
   if (query != nullptr)
   {
      if ((bracket != nullptr) && IsParametrizedCommand(query))
      {
         String expandedQuery = SubstituteCommandArguments(query, metricName);
         rc = dataExtractor->getMetric(expandedQuery, buffer, bufferLen);
      }
      else
      {
         rc = dataExtractor->getMetric(query, buffer, bufferLen);
      }
   }
   else if (MatchString(m_genericParamName, metricName, false))
   {
      TCHAR query[1024];
      AgentGetParameterArg(metricName, genericQueryArg, query, 1024);
      rc = dataExtractor->getMetric(query, buffer, bufferLen);
   }

   return rc;
}

/**
 * Get topic data as structured list data
 */
LONG Topic::retrieveListData(const TCHAR *metricName, StringList *buffer)
{
   LockGuard lockGuard(m_mutex);

   StructuredDataExtractor *dataExtractor = m_dataExtractor;
   int genericQueryArg = 1;
   if (m_instances != nullptr)
   {
      TCHAR instance[MAX_DB_STRING];
      if (!AgentGetParameterArg(metricName, 1, instance, MAX_DB_STRING) || (instance[0] == 0))
         return SYSINFO_RC_UNSUPPORTED;
      dataExtractor = getInstance(instance);
      if (dataExtractor == nullptr)
         return SYSINFO_RC_NO_SUCH_INSTANCE;
      genericQueryArg = 2;
   }

   LONG rc = SYSINFO_RC_UNKNOWN;
   const TCHAR *bracket = _tcschr(metricName, _T('('));
   String baseName = (bracket != nullptr) ? String(metricName, bracket - metricName) : String(metricName);
   const TCHAR *query = m_lists->get(baseName);
   if (query != nullptr)
   {
      if ((bracket != nullptr) && IsParametrizedCommand(query))
      {
         String expandedQuery = SubstituteCommandArguments(query, metricName);
         rc = dataExtractor->getList(expandedQuery, buffer);
      }
      else
      {
         rc = dataExtractor->getList(query, buffer);
      }
   }
   else if (MatchString(m_genericParamName, metricName, false))
   {
      TCHAR query[1024];
      AgentGetParameterArg(metricName, genericQueryArg, query, 1024);
      rc = dataExtractor->getList(query, buffer);
   }

   return rc;
}

/**
 * Get list of instances known for this topic
 */
LONG Topic::retrieveInstances(StringList *buffer)
{
   LockGuard lockGuard(m_mutex);

   purgeExpiredInstances();
   m_instances->forEach(
      [buffer] (const TCHAR *instance, const StructuredDataExtractor *extractor) -> EnumerationCallbackResult
      {
         buffer->add(instance);
         return _CONTINUE;
      });

   return SYSINFO_RC_SUCCESS;
}
