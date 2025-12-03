/*
 ** NetXMS - Network Management System
 ** Subagent for Redis monitoring
 ** Copyright (C) 2025 Raden Solutions
 **
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU Lesser General Public License as published
 ** by the Free Software Foundation; either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU Lesser General Public License
 ** along with this program; if not, write to the Free Software
 ** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 **/

#include "redis_subagent.h"
#include <ctype.h>

#define DEBUG_TAG _T("redis")
#define POLL_INTERVAL 60000

/**
 * Read attribute from config string
 */
static String ReadAttribute(const TCHAR *config, const TCHAR *attribute)
{
   TCHAR buffer[256];
   if (!ExtractNamedOptionValue(config, attribute, buffer, 256))
      return String();
   return String(buffer);
}

/**
 * Add Redis instance from config
 */
bool AddRedisFromConfig(const ConfigEntry *config)
{
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Parsing Redis configuration: %s"), config->getName());

   DatabaseInfo info;
   memset(&info, 0, sizeof(info));

   _tcslcpy(info.id, config->getName(), MAX_STR);
   tchar_to_utf8(config->getSubEntryValue(_T("server"), 0, _T("127.0.0.1")), -1, info.server, MAX_STR);
   info.port = config->getSubEntryValueAsInt(_T("port"), 0, REDIS_DEFAULT_PORT);
   info.database = config->getSubEntryValueAsInt(_T("database"), 0, 0);
   info.connectionTTL = config->getSubEntryValueAsInt(_T("connectionTTL"), 0, 3600);
   tchar_to_utf8(config->getSubEntryValue(_T("user"), 0, _T("")), -1, info.user, MAX_STR);
   const TCHAR *password = config->getSubEntryValue(_T("password"), 0, _T(""));
   if ((password != nullptr) && (password[0] != 0))
   {
#ifdef UNICODE
      char passwordBuffer[MAX_PASSWORD];
      wchar_to_utf8(password, -1, passwordBuffer, MAX_PASSWORD);
      DecryptPasswordA("netxms", passwordBuffer, info.password, MAX_PASSWORD);
#else
      DecryptPasswordA("netxms", password, info.password, MAX_PASSWORD);
#endif
   }

   RedisInstance *instance = new RedisInstance(&info);
   g_instances->add(instance);

   nxlog_debug_tag(DEBUG_TAG, 2, _T("Added Redis instance: %s (server=%hs:%d, db=%d)"), info.id, info.server, info.port, info.database);
   return true;
}

/**
 * Constructor
 */
RedisInstance::RedisInstance(DatabaseInfo *dbInfo) : m_dataLock(MutexType::FAST), m_stopCondition(true)
{
   memcpy(&m_info, dbInfo, sizeof(m_info));
   m_redis = nullptr;
   m_pollerThread = INVALID_THREAD_HANDLE;
   m_connected = false;
   m_lastPollTime = 0;
}

/**
 * Destructor
 */
RedisInstance::~RedisInstance()
{
   stop();
   disconnect();
}

/**
 * Connect to Redis
 */
bool RedisInstance::connect()
{
   m_connLock.lock();

   if (m_redis != nullptr)
   {
      redisFree(m_redis);
      m_redis = nullptr;
   }

   nxlog_debug_tag(DEBUG_TAG, 4, _T("Redis %s: connecting to %hs:%d"), m_info.id, m_info.server, m_info.port);

   struct timeval timeout = {1, 500000};
   m_redis = redisConnectWithTimeout(m_info.server, m_info.port, timeout);
   if (m_redis == nullptr || m_redis->err)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Redis %s: connection failed - %hs"), m_info.id, m_redis ? m_redis->errstr : "allocation error");
      if (m_redis)
      {
         redisFree(m_redis);
         m_redis = nullptr;
      }
      m_connected = false;
      m_connLock.unlock();
      return false;
   }

   if (m_info.password[0] != 0)
   {
      redisReply *reply;
      if (m_info.user[0] != 0)
      {
         reply = (redisReply *)redisCommand(m_redis, "AUTH %s %s", m_info.user, m_info.password);
      }
      else
      {
         reply = (redisReply *)redisCommand(m_redis, "AUTH %s", m_info.password);
      }
      if (reply == nullptr || reply->type == REDIS_REPLY_ERROR)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Redis %s: authentication failed"), m_info.id);
         if (reply)
            freeReplyObject(reply);
         redisFree(m_redis);
         m_redis = nullptr;
         m_connected = false;
         m_connLock.unlock();
         return false;
      }
      freeReplyObject(reply);
   }

   if (m_info.database != 0)
   {
      redisReply *reply = (redisReply *)redisCommand(m_redis, "SELECT %d", m_info.database);
      if (reply == nullptr || reply->type == REDIS_REPLY_ERROR)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Redis %s: database selection failed"), m_info.id);
         if (reply)
            freeReplyObject(reply);
         redisFree(m_redis);
         m_redis = nullptr;
         m_connected = false;
         m_connLock.unlock();
         return false;
      }
      freeReplyObject(reply);
   }

   m_connected = true;
   m_connLock.unlock();

   nxlog_debug_tag(DEBUG_TAG, 3, _T("Redis %s: connected successfully"), m_info.id);
   return true;
}

/**
 * Disconnect from Redis
 */
void RedisInstance::disconnect()
{
   m_connLock.lock();
   if (m_redis != nullptr)
   {
      redisFree(m_redis);
      m_redis = nullptr;
   }
   m_connected = false;
   m_connLock.unlock();
}

/**
 * Helper method to convert multibyte string to wide char and store in data map
 */
void RedisInstance::setDataFromMB(const TCHAR *key, const char *value)
{
   TCHAR *tvalue = TStringFromUTF8String(value);
   m_data.set(key, tvalue);
   MemFree(tvalue);
}

/**
 * Helper method to convert both key and value from multibyte to wide char and store
 */
void RedisInstance::setDataFromMBPair(const char *mbKey, const char *mbValue)
{
   TCHAR *tkey = TStringFromUTF8String(mbKey);
   TCHAR *tvalue = TStringFromUTF8String(mbValue);
   m_data.set(tkey, tvalue);
   MemFree(tkey);
   MemFree(tvalue);
}

/**
 * Parse INFO section
 */
void RedisInstance::parseInfoSection(const char *section, const char *infoData)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Redis %s: parseInfoSection called for section '%hs'"), m_info.id, section);

   if (!strcmp(section, "server"))
      parseServerInfo(infoData);
   else if (!strcmp(section, "clients"))
      parseClientsInfo(infoData);
   else if (!strcmp(section, "memory"))
      parseMemoryInfo(infoData);
   else if (!strcmp(section, "persistence"))
      parsePersistenceInfo(infoData);
   else if (!strcmp(section, "stats"))
      parseStatsInfo(infoData);
   else if (!strcmp(section, "replication"))
      parseReplicationInfo(infoData);
   else if (!strcmp(section, "keyspace"))
      parseKeyspaceInfo(infoData);
   else
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Redis %s: unknown section '%hs' ignored"), m_info.id, section);
}

/**
 * Parse server info
 */
void RedisInstance::parseServerInfo(const char *infoData)
{
   char *lines = strdup(infoData);
   char *line = strtok(lines, "\r\n");

   while (line != nullptr)
   {
      char *value = strchr(line, ':');
      if (value != nullptr)
      {
         *value = 0;
         value++;

         if (!strcmp(line, "redis_version"))
         {
            setDataFromMB(_T("redis_version"), value);
            nxlog_debug_tag(DEBUG_TAG, 5, _T("Redis %s: stored redis_version = %hs"), m_info.id, value);
         }
         else if (!strcmp(line, "redis_mode"))
         {
            setDataFromMB(_T("redis_mode"), value);
         }
         else if (!strcmp(line, "uptime_in_seconds"))
         {
            setDataFromMB(_T("uptime_in_seconds"), value);
         }
      }
      line = strtok(nullptr, "\r\n");
   }

   free(lines);
}

/**
 * Parse clients info
 */
void RedisInstance::parseClientsInfo(const char *infoData)
{
   char *lines = MemCopyStringA(infoData);
   char *line = strtok(lines, "\r\n");

   while (line != nullptr)
   {
      char *value = strchr(line, ':');
      if (value != nullptr)
      {
         *value = 0;
         value++;

         setDataFromMBPair(line, value);
      }
      line = strtok(nullptr, "\r\n");
   }

   const TCHAR *connectedClients = m_data.get(_T("connected_clients"));
   const TCHAR *maxClients = m_data.get(_T("maxclients"));
   if (connectedClients != nullptr && maxClients != nullptr)
   {
      uint64_t conn = _tcstoull(connectedClients, nullptr, 10);
      uint64_t max = _tcstoull(maxClients, nullptr, 10);
      if (max > 0)
      {
         double perc = (double)conn * 100.0 / (double)max;
         TCHAR buffer[32];
         _sntprintf(buffer, 32, _T("%.2f"), perc);
         m_data.set(_T("connected_clients_perc"), buffer);
      }
   }

   MemFree(lines);
}

/**
 * Parse memory info
 */
void RedisInstance::parseMemoryInfo(const char *infoData)
{
   char *lines = MemCopyStringA(infoData);
   char *line = strtok(lines, "\r\n");

   while (line != nullptr)
   {
      char *value = strchr(line, ':');
      if (value != nullptr)
      {
         *value = 0;
         value++;

         if (!strncmp(line, "used_memory", 11) ||
             !strcmp(line, "maxmemory") ||
             !strcmp(line, "mem_fragmentation_ratio"))
         {
            setDataFromMBPair(line, value);
         }
      }
      line = strtok(nullptr, "\r\n");
   }

   const TCHAR *usedMemory = m_data.get(_T("used_memory"));
   const TCHAR *maxMemory = m_data.get(_T("maxmemory"));
   if (usedMemory != nullptr && maxMemory != nullptr)
   {
      uint64_t usedMem = _tcstoull(usedMemory, nullptr, 10);
      uint64_t maxMem = _tcstoull(maxMemory, nullptr, 10);
      if (maxMem > 0)
      {
         double perc = (double)usedMem * 100.0 / (double)maxMem;
         TCHAR buffer[32];
         _sntprintf(buffer, 32, _T("%.2f"), perc);
         m_data.set(_T("used_memory_perc"), buffer);
      }
      else
      {
         m_data.set(_T("used_memory_perc"), _T("0.00"));
      }
   }

   MemFree(lines);
}

/**
 * Parse persistence info
 */
void RedisInstance::parsePersistenceInfo(const char *infoData)
{
   char *lines = MemCopyStringA(infoData);
   char *line = strtok(lines, "\r\n");

   while (line != nullptr)
   {
      char *value = strchr(line, ':');
      if (value != nullptr)
      {
         *value = 0;
         value++;

         if (!strcmp(line, "rdb_changes_since_last_save") ||
             !strcmp(line, "rdb_last_save_time") ||
             !strcmp(line, "aof_enabled"))
         {
            setDataFromMBPair(line, value);
         }
      }
      line = strtok(nullptr, "\r\n");
   }

   MemFree(lines);
}

/**
 * Parse stats info
 */
void RedisInstance::parseStatsInfo(const char *infoData)
{
   char *lines = MemCopyStringA(infoData);
   char *line = strtok(lines, "\r\n");

   while (line != nullptr)
   {
      char *value = strchr(line, ':');
      if (value != nullptr)
      {
         *value = 0;
         value++;

         if (!strncmp(line, "total_", 6) ||
             !strncmp(line, "keyspace_", 9) ||
             !strncmp(line, "instantaneous_", 14) ||
             !strncmp(line, "expired_", 8) ||
             !strncmp(line, "evicted_", 8))
         {
            setDataFromMBPair(line, value);
         }
      }
      line = strtok(nullptr, "\r\n");
   }

   const TCHAR *hits = m_data.get(_T("keyspace_hits"));
   const TCHAR *misses = m_data.get(_T("keyspace_misses"));
   if (hits != nullptr && misses != nullptr)
   {
      uint64_t h = _tcstoull(hits, nullptr, 10);
      uint64_t m = _tcstoull(misses, nullptr, 10);
      if (h + m > 0)
      {
         double ratio = (double)h * 100.0 / (double)(h + m);
         TCHAR buffer[32];
         _sntprintf(buffer, 32, _T("%.2f"), ratio);
         m_data.set(_T("keyspace_hit_ratio"), buffer);
      }
      else
      {
         m_data.set(_T("keyspace_hit_ratio"), _T("0.00"));
      }
   }

   MemFree(lines);
}

/**
 * Parse replication info
 */
void RedisInstance::parseReplicationInfo(const char *infoData)
{
   char *lines = MemCopyStringA(infoData);
   char *line = strtok(lines, "\r\n");

   bool isMaster = false;

   while (line != nullptr)
   {
      char *value = strchr(line, ':');
      if (value != nullptr)
      {
         *value = 0;
         value++;

         if (!strcmp(line, "role"))
         {
            setDataFromMBPair(line, value);

            if (!strcmp(value, "master"))
               isMaster = true;
         }
         else if (!strcmp(line, "connected_slaves") ||
                  !strcmp(line, "master_link_status"))
         {
            setDataFromMBPair(line, value);
         }
      }
      line = strtok(nullptr, "\r\n");
   }

   if (isMaster && m_data.get(_T("master_link_status")) == nullptr)
   {
      m_data.set(_T("master_link_status"), _T("N/A"));
   }

   MemFree(lines);
}

/**
 * Parse keyspace info
 */
void RedisInstance::parseKeyspaceInfo(const char *infoData)
{
   nxlog_debug_tag(DEBUG_TAG, 6, _T("Redis %s: parseKeyspaceInfo data: %.200hs"), m_info.id, infoData);

   uint64_t totalKeys = 0;

   if (infoData != nullptr && *infoData != 0)
   {
      char *lines = MemCopyStringA(infoData);
      char *line = strtok(lines, "\r\n");

      while (line != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("Redis %s: keyspace line: %hs"), m_info.id, line);

         if (!strncmp(line, "db", 2))
         {
            char *value = strchr(line, ':');
            if (value != nullptr)
            {
               *value = 0;
               value++;

               char *keys = strstr(value, "keys=");
               if (keys != nullptr)
               {
                  keys += 5;
                  char *end = strchr(keys, ',');
                  if (end)
                     *end = 0;

                  uint64_t count = strtoull(keys, nullptr, 10);
                  totalKeys += count;

                  TCHAR dbKey[64];
                  _sntprintf(dbKey, 64, _T("keys_%hs"), line);
                  TCHAR tvalue[32];
                  m_data.set(dbKey, IntegerToString(count, tvalue));
                  nxlog_debug_tag(DEBUG_TAG, 5, _T("Redis %s: stored %s = %s"), m_info.id, dbKey, tvalue);
               }
            }
         }
         line = strtok(nullptr, "\r\n");
      }

      MemFree(lines);
   }

   TCHAR buffer[32];
   m_data.set(_T("total_keys"), IntegerToString(totalKeys, buffer));
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Redis %s: stored total_keys = %s"), m_info.id, buffer);
}

/**
 * Poll Redis instance
 */
bool RedisInstance::poll()
{
   if (!m_connected)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Redis %s: attempting connection"), m_info.id);
      if (!connect())
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Redis %s: connection attempt failed"), m_info.id);
         return false;
      }
   }

   m_connLock.lock();

   nxlog_debug_tag(DEBUG_TAG, 5, _T("Redis %s: sending INFO command"), m_info.id);
   redisReply *reply = (redisReply *)redisCommand(m_redis, "INFO");
   if (reply == nullptr || reply->type != REDIS_REPLY_STRING)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Redis %s: INFO command failed - reply type %d"), m_info.id, reply ? reply->type : -1);
      if (reply)
         freeReplyObject(reply);
      m_connLock.unlock();
      disconnect();
      return false;
   }

   nxlog_debug_tag(DEBUG_TAG, 5, _T("Redis %s: INFO command successful, response length %d"), m_info.id, (int)strlen(reply->str));
   m_connLock.unlock();

   m_dataLock.lock();

   nxlog_debug_tag(DEBUG_TAG, 6, _T("Redis %s: raw INFO response (first 200 chars): %.200hs"), m_info.id, reply->str);

   char *info = MemCopyStringA(reply->str);
   char *saveptr = nullptr;
   char *line = strtok_r(info, "\r\n", &saveptr);
   char currentSection[64] = "";
   char *sectionData = nullptr;
   size_t sectionDataSize = 0;
   size_t sectionDataAlloc = 0;

   while (line != nullptr)
   {
      if (line[0] == '#')
      {
         if (currentSection[0] != 0)
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("Redis %s: parsing section '%hs'"), m_info.id, currentSection);
            parseInfoSection(currentSection, sectionData != nullptr ? sectionData : "");
            if (sectionData != nullptr)
            {
               free(sectionData);
               sectionData = nullptr;
            }
            sectionDataSize = 0;
            sectionDataAlloc = 0;
         }

         char *sectionName = line + 1;
         while (*sectionName == ' ')
            sectionName++;
         strlcpy(currentSection, sectionName, 64);

         for (char *p = currentSection; *p; p++)
            *p = tolower(*p);
      }
      else if (strchr(line, ':') != nullptr)
      {
         size_t lineLen = strlen(line);
         size_t needed = sectionDataSize + lineLen + 2; // +1 for \n, +1 for \0

         if (needed > sectionDataAlloc)
         {
            sectionDataAlloc = needed + 256;
            sectionData = (char *)realloc(sectionData, sectionDataAlloc);
         }

         if (sectionDataSize > 0)
         {
            strcpy(sectionData + sectionDataSize, line);
            sectionDataSize += lineLen;
            sectionData[sectionDataSize++] = '\n';
         }
         else
         {
            strcpy(sectionData, line);
            sectionDataSize = lineLen;
            sectionData[sectionDataSize++] = '\n';
         }
         sectionData[sectionDataSize] = 0;
      }

      line = strtok_r(nullptr, "\r\n", &saveptr);
   }

   if (currentSection[0] != 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Redis %s: parsing section '%hs'"), m_info.id, currentSection);
      parseInfoSection(currentSection, sectionData != nullptr ? sectionData : "");
      if (sectionData != nullptr)
         free(sectionData);
   }

   MemFree(info);
   freeReplyObject(reply);

   m_lastPollTime = GetCurrentTimeMs();
   m_dataLock.unlock();

   return true;
}

/**
 * Poller thread
 */
void RedisInstance::pollerThread()
{
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Redis %s: poller thread started"), m_info.id);

   if (!poll())
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("Redis %s: initial poll failed"), m_info.id);
   }

   while (!m_stopCondition.wait(POLL_INTERVAL))
   {
      if (!poll())
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Redis %s: poll failed, will retry"), m_info.id);
      }
   }

   nxlog_debug_tag(DEBUG_TAG, 3, _T("Redis %s: poller thread stopped"), m_info.id);
}

/**
 * Start polling
 */
void RedisInstance::run()
{
   m_pollerThread = ThreadCreateEx(this, &RedisInstance::pollerThread);
}

/**
 * Stop polling
 */
void RedisInstance::stop()
{
   m_stopCondition.set();
   ThreadJoin(m_pollerThread);
   m_pollerThread = INVALID_THREAD_HANDLE;
}

/**
 * Get data
 */
bool RedisInstance::getData(const TCHAR *tag, TCHAR *value)
{
   LockGuard lockGuard(m_dataLock);
   const TCHAR *result = m_data.get(tag);
   if (result != nullptr)
   {
      ret_string(value, result);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Redis %s: getData(%s) = %s"), m_info.id, tag, result);
      return true;
   }
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Redis %s: getData(%s) = NOT FOUND"), m_info.id, tag);
   return false;
}

/**
 * Get keyspace data
 */
LONG RedisInstance::getKeyspaceData(const TCHAR *param, const TCHAR *arg, TCHAR *value)
{
   TCHAR dbNum[16];
   if (!AgentGetParameterArg(param, 2, dbNum, 16))
      return SYSINFO_RC_UNSUPPORTED;

   TCHAR key[64];
   _sntprintf(key, 64, _T("keys_db%s"), dbNum);

   LockGuard lockGuard(m_dataLock);
   const TCHAR *result = m_data.get(key);
   if (result != nullptr)
   {
      ret_string(value, result);
      return SYSINFO_RC_SUCCESS;
   }

   ret_uint64(value, 0);
   return SYSINFO_RC_SUCCESS;
}