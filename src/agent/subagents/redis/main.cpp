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
#include <netxms-version.h>

#define DEBUG_TAG _T("redis")

/**
 * Database instances
 */
ObjectArray<RedisInstance> *g_instances = nullptr;

/**
 * Find instance by ID
 */
static RedisInstance *FindInstance(const TCHAR *id)
{
   for (int i = 0; i < g_instances->size(); i++)
   {
      RedisInstance *instance = g_instances->get(i);
      if (!_tcsicmp(instance->getId(), id))
         return instance;
   }
   return nullptr;
}

/**
 * Handler for parameters without instance
 */
static LONG H_GlobalParameter(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR id[MAX_STR];
   if (!AgentGetParameterArg(param, 1, id, MAX_STR))
      return SYSINFO_RC_UNSUPPORTED;

   RedisInstance *instance = FindInstance(id);
   if (instance == nullptr)
      return SYSINFO_RC_UNSUPPORTED;

   return instance->getData(arg, value) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

/**
 * Handler for Redis.IsReachable parameter
 */
static LONG H_ConnectionStatus(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR id[MAX_STR];
   if (!AgentGetParameterArg(param, 1, id, MAX_STR))
      return SYSINFO_RC_UNSUPPORTED;

   RedisInstance *instance = FindInstance(id);
   if (instance == nullptr)
      return SYSINFO_RC_UNSUPPORTED;

   ret_string(value, instance->isConnected() ? _T("YES") : _T("NO"));
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for keyspace parameters
 */
static LONG H_KeyspaceParameter(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR id[MAX_STR];
   if (!AgentGetParameterArg(param, 1, id, MAX_STR))
      return SYSINFO_RC_UNSUPPORTED;

   RedisInstance *instance = FindInstance(id);
   if (instance == nullptr)
      return SYSINFO_RC_UNSUPPORTED;

   return instance->getKeyspaceData(param, arg, value);
}

/**
 * Handler for Redis.Databases list
 */
static LONG H_DatabaseList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   for (int i = 0; i < g_instances->size(); i++)
      value->add(g_instances->get(i)->getId());
   return SYSINFO_RC_SUCCESS;
}

/**
 * Load configuration
 */
static bool LoadConfiguration(Config *config)
{
   g_instances = new ObjectArray<RedisInstance>(8, 8, Ownership::True);

   ConfigEntry *databaseRoot = config->getEntry(_T("/Redis/Database"));
   if (databaseRoot != nullptr)
   {
      unique_ptr<ObjectArray<ConfigEntry>> databases = databaseRoot->getSubEntries(_T("*"));

      for (int i = 0; i < databases->size(); i++)
      {
         if (!AddRedisFromConfig(databases->get(i)))
         {
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG,
                            _T("Unable to add Redis connection from configuration. Configuration: %s"),
                            databaseRoot->getValue(i));
         }
      }
   }

   if (g_instances->size() == 0)
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("No Redis instances configured"));
      delete g_instances;
      g_instances = nullptr;
      return false;
   }

   for (int i = 0; i < g_instances->size(); i++)
      g_instances->get(i)->run();

   return true;
}

/**
 * Subagent initialization
 */
static bool SubAgentInit(Config *config)
{
   return LoadConfiguration(config);
}

/**
 * Shutdown handler
 */
static void SubAgentShutdown()
{
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Stopping Redis pollers"));
   if (g_instances != nullptr)
   {
      for (int i = 0; i < g_instances->size(); i++)
         g_instances->get(i)->stop();
      delete g_instances;
   }
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Redis subagent stopped"));
}

/**
 * Supported parameters
 */
static NETXMS_SUBAGENT_PARAM s_parameters[] =
    {
        {_T("Redis.Clients.Blocked(*)"), H_GlobalParameter, _T("blocked_clients"), DCI_DT_UINT64, _T("Redis: blocked clients")},
        {_T("Redis.Clients.Connected(*)"), H_GlobalParameter, _T("connected_clients"), DCI_DT_UINT64, _T("Redis: connected clients")},
        {_T("Redis.Clients.MaxClients(*)"), H_GlobalParameter, _T("maxclients"), DCI_DT_UINT64, _T("Redis: maximum number of clients")},
        {_T("Redis.Clients.ConnectedPerc(*)"), H_GlobalParameter, _T("connected_clients_perc"), DCI_DT_FLOAT, _T("Redis: connected clients percentage")},
        {_T("Redis.IsReachable(*)"), H_ConnectionStatus, nullptr, DCI_DT_STRING, _T("Redis: is instance reachable")},
        {_T("Redis.Keys.DB(*)"), H_KeyspaceParameter, _T("db_keys"), DCI_DT_UINT64, _T("Redis: keys in database")},
        {_T("Redis.Keys.Evicted(*)"), H_GlobalParameter, _T("evicted_keys"), DCI_DT_UINT64, _T("Redis: evicted keys")},
        {_T("Redis.Keys.Expired(*)"), H_GlobalParameter, _T("expired_keys"), DCI_DT_UINT64, _T("Redis: expired keys")},
        {_T("Redis.Keys.Total(*)"), H_GlobalParameter, _T("total_keys"), DCI_DT_UINT64, _T("Redis: total keys")},
        {_T("Redis.Memory.Fragmentation(*)"), H_GlobalParameter, _T("mem_fragmentation_ratio"), DCI_DT_FLOAT, _T("Redis: memory fragmentation ratio")},
        {_T("Redis.Memory.MaxMemory(*)"), H_GlobalParameter, _T("maxmemory"), DCI_DT_UINT64, _T("Redis: max memory configuration")},
        {_T("Redis.Memory.Peak(*)"), H_GlobalParameter, _T("used_memory_peak"), DCI_DT_UINT64, _T("Redis: peak memory usage")},
        {_T("Redis.Memory.Used(*)"), H_GlobalParameter, _T("used_memory"), DCI_DT_UINT64, _T("Redis: used memory")},
        {_T("Redis.Memory.UsedPerc(*)"), H_GlobalParameter, _T("used_memory_perc"), DCI_DT_FLOAT, _T("Redis: used memory percentage")},
        {_T("Redis.Memory.UsedRSS(*)"), H_GlobalParameter, _T("used_memory_rss"), DCI_DT_UINT64, _T("Redis: RSS memory")},
        {_T("Redis.Persistence.AOFEnabled(*)"), H_GlobalParameter, _T("aof_enabled"), DCI_DT_INT, _T("Redis: AOF enabled")},
        {_T("Redis.Persistence.Changes(*)"), H_GlobalParameter, _T("rdb_changes_since_last_save"), DCI_DT_UINT64, _T("Redis: changes since last save")},
        {_T("Redis.Persistence.LastSave(*)"), H_GlobalParameter, _T("rdb_last_save_time"), DCI_DT_UINT64, _T("Redis: last RDB save timestamp")},
        {_T("Redis.Replication.ConnectedSlaves(*)"), H_GlobalParameter, _T("connected_slaves"), DCI_DT_UINT, _T("Redis: connected slaves")},
        {_T("Redis.Replication.MasterLinkStatus(*)"), H_GlobalParameter, _T("master_link_status"), DCI_DT_STRING, _T("Redis: master link status")},
        {_T("Redis.Replication.Role(*)"), H_GlobalParameter, _T("role"), DCI_DT_STRING, _T("Redis: replication role")},
        {_T("Redis.Server.Mode(*)"), H_GlobalParameter, _T("redis_mode"), DCI_DT_STRING, _T("Redis: server mode")},
        {_T("Redis.Server.Uptime(*)"), H_GlobalParameter, _T("uptime_in_seconds"), DCI_DT_UINT64, _T("Redis: server uptime in seconds")},
        {_T("Redis.Server.Version(*)"), H_GlobalParameter, _T("redis_version"), DCI_DT_STRING, _T("Redis: server version")},
        {_T("Redis.Stats.HitRatio(*)"), H_GlobalParameter, _T("keyspace_hit_ratio"), DCI_DT_FLOAT, _T("Redis: keyspace hit ratio")},
        {_T("Redis.Stats.KeyspaceHits(*)"), H_GlobalParameter, _T("keyspace_hits"), DCI_DT_UINT64, _T("Redis: keyspace hits")},
        {_T("Redis.Stats.KeyspaceMisses(*)"), H_GlobalParameter, _T("keyspace_misses"), DCI_DT_UINT64, _T("Redis: keyspace misses")},
        {_T("Redis.Stats.NetworkInput(*)"), H_GlobalParameter, _T("total_net_input_bytes"), DCI_DT_UINT64, _T("Redis: total network input bytes")},
        {_T("Redis.Stats.NetworkOutput(*)"), H_GlobalParameter, _T("total_net_output_bytes"), DCI_DT_UINT64, _T("Redis: total network output bytes")},
        {_T("Redis.Stats.OpsPerSec(*)"), H_GlobalParameter, _T("instantaneous_ops_per_sec"), DCI_DT_UINT64, _T("Redis: operations per second")},
        {_T("Redis.Stats.TotalCommands(*)"), H_GlobalParameter, _T("total_commands_processed"), DCI_DT_UINT64, _T("Redis: total processed commands")}};

/**
 * Supported lists
 */
static NETXMS_SUBAGENT_LIST s_lists[] =
    {
        {_T("Redis.Databases"), H_DatabaseList, nullptr}};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO subagentInfo =
    {
        NETXMS_SUBAGENT_INFO_MAGIC,
        _T("REDIS"), NETXMS_VERSION_STRING,
        SubAgentInit, SubAgentShutdown, nullptr, nullptr, nullptr,
        sizeof(s_parameters) / sizeof(NETXMS_SUBAGENT_PARAM), s_parameters,
        sizeof(s_lists) / sizeof(NETXMS_SUBAGENT_LIST), s_lists,
        0, nullptr, // tables
        0, nullptr, // actions
        0, nullptr  // push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(REDIS)
{
   *ppInfo = &subagentInfo;
   return true;
}