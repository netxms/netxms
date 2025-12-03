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

#ifndef _redis_subagent_h_
#define _redis_subagent_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>
#include <hiredis/hiredis.h>

#define MAX_STR 256
#define MAX_PASSWORD 256
#define REDIS_DEFAULT_PORT 6379

/**
 * Database connection information
 */
struct DatabaseInfo
{
   TCHAR id[MAX_STR];
   char server[MAX_STR];
   uint16_t port;
   char user[MAX_STR];
   char password[MAX_PASSWORD];
   int database;
   uint32_t connectionTTL;
};

/**
 * Redis instance
 */
class RedisInstance
{
private:
   DatabaseInfo m_info;
   redisContext *m_redis;
   THREAD m_pollerThread;
   bool m_connected;
   StringMap m_data;
   Mutex m_dataLock;
   Mutex m_connLock;
   Condition m_stopCondition;
   int64_t m_lastPollTime;

   void pollerThread();
   bool connect();
   void disconnect();
   bool poll();
   void parseInfoSection(const char *section, const char *infoData);
   void parseServerInfo(const char *infoData);
   void parseClientsInfo(const char *infoData);
   void parseMemoryInfo(const char *infoData);
   void parseStatsInfo(const char *infoData);
   void parsePersistenceInfo(const char *infoData);
   void parseReplicationInfo(const char *infoData);
   void parseKeyspaceInfo(const char *infoData);
   void setDataFromMB(const TCHAR *key, const char *value);
   void setDataFromMBPair(const char *mbKey, const char *mbValue);

public:
   RedisInstance(DatabaseInfo *info);
   ~RedisInstance();

   void run();
   void stop();

   const TCHAR *getId() { return m_info.id; }
   bool isConnected() { return m_connected; }
   bool getData(const TCHAR *tag, TCHAR *value);
   LONG getKeyspaceData(const TCHAR *param, const TCHAR *arg, TCHAR *value);
};

/**
 * Global variables
 */
extern ObjectArray<RedisInstance> *g_instances;

bool AddRedisFromConfig(const ConfigEntry *config);

#endif /* _redis_subagent_h_ */