/*
** NetXMS Tuxedo subagent
** Copyright (C) 2014-2020 Raden Solutions
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
** File: servers.cpp
**
**/

#include "tuxedo_subagent.h"

/**
 * Tuxedo server information
 */
class TuxedoServerInstance
{
public:
   long m_id;
   long m_baseId;
   long m_groupId;
   char m_group[32];
   char m_name[128];
   char m_state[16];
   char m_cmdLine[1024];
   char m_envFile[256];
   char m_rqAddr[132];
   char m_lmid[64];
   long m_min;
   long m_max;
   long m_generation;
   long m_pid;
   long m_minThreads;
   long m_maxThreads;
   long m_curThreads;
   long m_timeStart;
   long m_timeRestart;
   long m_convCount;
   long m_dequeueCount;
   long m_enqueueCount;
   long m_postCount;
   long m_reqCount;
   long m_subscribeCount;
   long m_txnCount;
   long m_totalRequests;
   long m_totalWorkloads;
   long m_activeRequests;
   char m_currService[128];
   long m_tranLevel;

   TuxedoServerInstance(FBFR32 *fb, FLDOCC32 index);
   TuxedoServerInstance(TuxedoServerInstance *src);

   UINT32 getUniqueBaseId() const { return (m_groupId << 16) | m_baseId; }
};

/**
 * Create server object from FB
 */
TuxedoServerInstance::TuxedoServerInstance(FBFR32 *fb, FLDOCC32 index)
{
   m_id = 0;
   m_baseId = 0;
   m_groupId = 0;
   m_group[0] = 0;
   m_name[0] = 0;
   m_state[0] = 0;
   m_cmdLine[0] = 0;
   m_envFile[0] = 0;
   m_rqAddr[0] = 0;
   m_lmid[0] = 0;
   m_min = 0;
   m_max = 0;
   m_generation = 0;
   m_pid = 0;
   m_minThreads = 0;
   m_maxThreads = 0;
   m_curThreads = 0;
   m_timeStart = 0;
   m_timeRestart = 0;
   m_convCount = 0;
   m_dequeueCount = 0;
   m_enqueueCount = 0;
   m_postCount = 0;
   m_reqCount = 0;
   m_subscribeCount = 0;
   m_txnCount = 0;
   m_totalRequests = 0;
   m_totalWorkloads = 0;
   m_activeRequests = 0;
   m_currService[0] = 0;
   m_tranLevel = 0;

   CFget32(fb, TA_SRVID, index, (char *)&m_id, NULL, FLD_LONG);
   CFget32(fb, TA_BASESRVID, index, (char *)&m_baseId, NULL, FLD_LONG);
   CFget32(fb, TA_GRPNO, index, (char *)&m_groupId, NULL, FLD_LONG);
   CFgetString(fb, TA_SRVGRP, index, m_group, sizeof(m_group));
   CFgetExecutableName(fb, TA_SERVERNAME, index, m_name, sizeof(m_name));
   CFgetString(fb, TA_STATE, index, m_state, sizeof(m_state));
   CFgetString(fb, TA_CLOPT, index, m_cmdLine, sizeof(m_cmdLine));
   CFgetString(fb, TA_ENVFILE, index, m_envFile, sizeof(m_envFile));
   CFgetString(fb, TA_RQADDR, index, m_rqAddr, sizeof(m_rqAddr));
   CFgetString(fb, TA_LMID, index, m_lmid, sizeof(m_lmid));
   CFget32(fb, TA_MIN, index, (char *)&m_min, NULL, FLD_LONG);
   CFget32(fb, TA_MAX, index, (char *)&m_max, NULL, FLD_LONG);
   CFget32(fb, TA_GENERATION, index, (char *)&m_generation, NULL, FLD_LONG);
   CFget32(fb, TA_PID, index, (char *)&m_pid, NULL, FLD_LONG);
   CFget32(fb, TA_MINDISPATCHTHREADS, index, (char *)&m_minThreads, NULL, FLD_LONG);
   CFget32(fb, TA_MAXDISPATCHTHREADS, index, (char *)&m_maxThreads, NULL, FLD_LONG);
   CFget32(fb, TA_CURDISPATCHTHREADS, index, (char *)&m_curThreads, NULL, FLD_LONG);
   CFget32(fb, TA_TIMESTART, index, (char *)&m_timeStart, NULL, FLD_LONG);
   CFget32(fb, TA_TIMERESTART, index, (char *)&m_timeRestart, NULL, FLD_LONG);
   CFget32(fb, TA_NUMCONV, index, (char *)&m_convCount, NULL, FLD_LONG);
   CFget32(fb, TA_NUMDEQUEUE, index, (char *)&m_dequeueCount, NULL, FLD_LONG);
   CFget32(fb, TA_NUMENQUEUE, index, (char *)&m_enqueueCount, NULL, FLD_LONG);
   CFget32(fb, TA_NUMPOST, index, (char *)&m_postCount, NULL, FLD_LONG);
   CFget32(fb, TA_NUMREQ, index, (char *)&m_reqCount, NULL, FLD_LONG);
   CFget32(fb, TA_NUMSUBSCRIBE, index, (char *)&m_subscribeCount, NULL, FLD_LONG);
   CFget32(fb, TA_NUMTRAN, index, (char *)&m_txnCount, NULL, FLD_LONG);
   CFget32(fb, TA_TOTREQC, index, (char *)&m_totalRequests, NULL, FLD_LONG);
   CFget32(fb, TA_TOTWORKL, index, (char *)&m_totalWorkloads, NULL, FLD_LONG);
   CFget32(fb, TA_CURREQ, index, (char *)&m_activeRequests, NULL, FLD_LONG);
   CFgetString(fb, TA_CURRSERVICE, index, m_currService, sizeof(m_currService));
   CFget32(fb, TA_TRANLEV, index, (char *)&m_tranLevel, NULL, FLD_LONG);
}

/**
 * Create server instance object from another server instance object
 */
TuxedoServerInstance::TuxedoServerInstance(TuxedoServerInstance *src)
{
   m_id = src->m_id;
   m_baseId = src->m_baseId;
   m_groupId = src->m_groupId;
   strcpy(m_group, src->m_group);
   strcpy(m_name, src->m_name);
   strcpy(m_state, src->m_state);
   strcpy(m_cmdLine, src->m_cmdLine);
   strcpy(m_envFile, src->m_envFile);
   strcpy(m_rqAddr, src->m_rqAddr);
   strcpy(m_lmid, src->m_lmid);
   m_min = src->m_min;
   m_max = src->m_max;
   m_generation = src->m_generation;
   m_pid = src->m_pid;
   m_minThreads = src->m_minThreads;
   m_maxThreads = src->m_maxThreads;
   m_curThreads = src->m_curThreads;
   m_timeStart = src->m_timeStart;
   m_timeRestart = src->m_timeRestart;
   m_convCount = src->m_convCount;
   m_dequeueCount = src->m_dequeueCount;
   m_enqueueCount = src->m_enqueueCount;
   m_postCount = src->m_postCount;
   m_reqCount = src->m_reqCount;
   m_subscribeCount = src->m_subscribeCount;
   m_txnCount = src->m_txnCount;
   m_totalRequests = src->m_totalRequests;
   m_totalWorkloads = src->m_totalWorkloads;
   m_activeRequests = src->m_activeRequests;
   strcpy(m_currService, src->m_currService);
   m_tranLevel = src->m_tranLevel;
}

/**
 * Tuxedo server
 */
class TuxedoServer
{
public:
   uint32_t m_uniqueId;
   ObjectArray<TuxedoServerInstance> m_instances;
   TuxedoServerInstance m_summary;
   long m_running;

   TuxedoServer(TuxedoServerInstance *m_base);

   void addInstance(TuxedoServerInstance *s);
};

/**
 * Create new Tuxedo server object
 */
TuxedoServer::TuxedoServer(TuxedoServerInstance *base) : m_instances(64, 64, Ownership::True), m_summary(base)
{
   m_uniqueId = (base->m_groupId << 16) | base->m_baseId;
   m_running = !strncmp(base->m_state, "ACT", 3) ? 1 : 0;
   m_instances.add(base);
}

/**
 * Add new instance information
 */
void TuxedoServer::addInstance(TuxedoServerInstance *s)
{
   m_instances.add(s);
   if (!strcmp(s->m_state, "ACTIVE"))
      m_running++;
   m_summary.m_curThreads += s->m_curThreads;
   m_summary.m_convCount += s->m_convCount;
   m_summary.m_dequeueCount += s->m_dequeueCount;
   m_summary.m_enqueueCount += s->m_enqueueCount;
   m_summary.m_postCount += s->m_postCount;
   m_summary.m_reqCount += s->m_reqCount;
   m_summary.m_subscribeCount += s->m_subscribeCount;
   m_summary.m_txnCount += s->m_txnCount;
   m_summary.m_totalRequests += s->m_totalRequests;
   m_summary.m_totalWorkloads += s->m_totalWorkloads;
   m_summary.m_activeRequests += s->m_activeRequests;
}

/**
 * Service list
 */
static Mutex s_lock(MutexType::FAST);
static HashMap<uint32_t, TuxedoServer> *s_servers = nullptr;
static HashMap<uint32_t, TuxedoServerInstance> *s_serverInstances = nullptr;

/**
 * Reset server cache
 */
void TuxedoResetServers()
{
   s_lock.lock();
   delete_and_null(s_servers);
   delete_and_null(s_serverInstances);
   s_lock.unlock();
}

/**
 * Query servers
 */
void TuxedoQueryServers()
{
   auto servers = new HashMap<uint32_t, TuxedoServer>(Ownership::True);
   auto serverInstances = new HashMap<uint32_t, TuxedoServerInstance>(Ownership::False);

   FBFR32 *fb = (FBFR32 *)tpalloc((char *)"FML32", nullptr, 4096);
   CFchg32(fb, TA_OPERATION, 0, (char *)"GET", 0, FLD_STRING);
   CFchg32(fb, TA_CLASS, 0, (char *)"T_SERVER", 0, FLD_STRING);

   long flags = ((g_tuxedoQueryLocalData & LOCAL_DATA_SERVERS) ? MIB_LOCAL : 0);
   CFchg32(fb, TA_FLAGS, 0, (char *)&flags, 0, FLD_LONG);

   char lmid[64];
   if (g_tuxedoLocalMachineFilter && TuxedoGetLocalMachineID(lmid))
   {
      CFchg32(fb, TA_LMID, 0, lmid, 0, FLD_STRING);
   }

   bool readMore = true;
   long rsplen = 262144;
   FBFR32 *rsp = (FBFR32 *)tpalloc((char *)"FML32", nullptr, rsplen);
   while(readMore)
   {
      readMore = false;
      if (tpcall((char *)".TMIB", (char *)fb, 0, (char **)&rsp, &rsplen, 0) != -1)
      {
         long count = 0;
         CFget32(rsp, TA_OCCURS, 0, (char *)&count, nullptr, FLD_LONG);
         for(int i = 0; i < (int)count; i++)
         {
            TuxedoServerInstance *instance = new TuxedoServerInstance(rsp, (FLDOCC32)i);
            TuxedoServer *server = servers->get(instance->getUniqueBaseId());
            if (server != nullptr)
            {
               server->addInstance(instance);
            }
            else
            {
               server = new TuxedoServer(instance);
               servers->set(server->m_uniqueId, server);
            }
            serverInstances->set(static_cast<uint32_t>((instance->m_groupId << 16) | instance->m_id), instance);
         }

         long more = 0;
         CFget32(rsp, TA_MORE, 0, (char *)&more, nullptr, FLD_LONG);
         if (more)
         {
         	CFchg32(fb, TA_OPERATION, 0, (char *)"GETNEXT", 0, FLD_STRING);

            char cursor[256];
            CFgetString(rsp, TA_CURSOR, 0, cursor, sizeof(cursor));
         	CFchg32(fb, TA_CURSOR, 0, cursor, 0, FLD_STRING);

            readMore = true;
         }
      }
      else
      {
         nxlog_debug_tag(TUXEDO_DEBUG_TAG, 3, _T("tpcall() call failed (%hs)"), tpstrerrordetail(tperrno, 0));
         delete_and_null(servers);
         delete_and_null(serverInstances);
      }
   }

   tpfree((char *)rsp);
   tpfree((char *)fb);

   s_lock.lock();
   delete s_servers;
   s_servers = servers;
   delete s_serverInstances;
   s_serverInstances = serverInstances;
   s_lock.unlock();
}

/**
 * Fill server instance list
 */
static EnumerationCallbackResult FillServerInstanceList(const uint32_t& key, TuxedoServer *server, StringList *list)
{
   for(int i = 0; i < server->m_instances.size(); i++)
   {
      TuxedoServerInstance *instance = server->m_instances.get(i);
      TCHAR serverId[64];
      _sntprintf(serverId, 64, _T("%ld,%ld"), instance->m_groupId, instance->m_id);
      list->add(serverId);
   }
   return _CONTINUE;
}

/**
 * Handler for Tuxedo.ServerInstances list
 */
LONG H_ServerInstancesList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   LONG rc = SYSINFO_RC_SUCCESS;

   s_lock.lock();
   if (s_servers != nullptr)
   {
      s_servers->forEach(FillServerInstanceList, value);
   }
   else
   {
      rc = SYSINFO_RC_ERROR;
   }
   s_lock.unlock();
   return rc;
}

/**
 * Fill server list
 */
static EnumerationCallbackResult FillServerList(const uint32_t& key, TuxedoServer *server, StringList *list)
{
   TCHAR serverId[64];
   _sntprintf(serverId, 64, _T("%ld,%ld"), server->m_summary.m_groupId, server->m_summary.m_baseId);
   list->add(serverId);
   return _CONTINUE;
}

/**
 * Handler for Tuxedo.Servers list
 */
LONG H_ServersList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   LONG rc = SYSINFO_RC_SUCCESS;

   s_lock.lock();
   if (s_servers != nullptr)
   {
      s_servers->forEach(FillServerList, value);
   }
   else
   {
      rc = SYSINFO_RC_ERROR;
   }
   s_lock.unlock();
   return rc;
}

/**
 * Fill server instance table
 */
static EnumerationCallbackResult FillServerInstanceTable(const uint32_t& key, TuxedoServer *server, Table *table)
{
   for(int i = 0; i < server->m_instances.size(); i++)
   {
      table->addRow();
      TuxedoServerInstance *s = server->m_instances.get(i);
      table->set(0, (INT32)s->m_groupId);
      table->set(1, (INT32)s->m_id);
      table->set(2, (INT32)s->m_baseId);
      table->set(3, s->m_name);
      table->set(4, s->m_group);
      table->set(5, s->m_state);
      table->set(6, s->m_rqAddr);
      table->set(7, s->m_lmid);
      table->set(8, (INT32)s->m_pid);
      table->set(9, (INT32)s->m_generation);
      table->set(10, (INT32)s->m_curThreads);
      table->set(11, (INT32)s->m_minThreads);
      table->set(12, (INT32)s->m_maxThreads);
      table->set(13, (INT32)s->m_activeRequests);
      table->set(14, s->m_currService);
      table->set(15, (INT32)s->m_tranLevel);
      table->set(16, (INT32)s->m_totalRequests);
      table->set(17, (INT32)s->m_totalWorkloads);
      table->set(18, (INT32)s->m_convCount);
      table->set(19, (INT32)s->m_dequeueCount);
      table->set(20, (INT32)s->m_enqueueCount);
      table->set(21, (INT32)s->m_postCount);
      table->set(22, (INT32)s->m_reqCount);
      table->set(23, (INT32)s->m_subscribeCount);
      table->set(24, (INT32)s->m_txnCount);
      table->set(25, (INT32)s->m_timeStart);
      table->set(26, (INT32)s->m_timeRestart);
      table->set(27, s->m_envFile);
      table->set(28, s->m_cmdLine);
   }
   return _CONTINUE;
}

/**
 * Handler for Tuxedo.ServerInstances table
 */
LONG H_ServerInstancesTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   LONG rc = SYSINFO_RC_SUCCESS;

   s_lock.lock();
   if (s_servers != nullptr)
   {
      value->addColumn(_T("GROUP_ID"), DCI_DT_INT, _T("Group ID"), true);
      value->addColumn(_T("ID"), DCI_DT_INT, _T("ID"), true);
      value->addColumn(_T("BASE_ID"), DCI_DT_INT, _T("Base ID"));
      value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"));
      value->addColumn(_T("GROUP_NAME"), DCI_DT_STRING, _T("Group Name"));
      value->addColumn(_T("STATE"), DCI_DT_STRING, _T("State"));
      value->addColumn(_T("RQ_ADDR"), DCI_DT_STRING, _T("Request Address"));
      value->addColumn(_T("LMID"), DCI_DT_STRING, _T("Machine"));
      value->addColumn(_T("PID"), DCI_DT_INT, _T("Process ID"));
      value->addColumn(_T("GENERATION"), DCI_DT_INT, _T("Generation"));
      value->addColumn(_T("THREADS"), DCI_DT_INT, _T("Threads"));
      value->addColumn(_T("THREADS_MIN"), DCI_DT_INT, _T("Min Threads"));
      value->addColumn(_T("THREADS_MAX"), DCI_DT_INT, _T("Max Threads"));
      value->addColumn(_T("ACTIVE_REQUESTS"), DCI_DT_INT, _T("Active Requests"));
      value->addColumn(_T("CURR_SERVICE"), DCI_DT_STRING, _T("Current Service"));
      value->addColumn(_T("TRAN_LEVEL"), DCI_DT_INT, _T("Transaction Level"));
      value->addColumn(_T("PROCESSED_REQUESTS"), DCI_DT_INT, _T("Processed Requests"));
      value->addColumn(_T("PROCESSED_WORKLOADS"), DCI_DT_INT, _T("Processed Workloads"));
      value->addColumn(_T("CONVERSATIONS"), DCI_DT_INT, _T("Conversations"));
      value->addColumn(_T("DEQUEUE_COUNT"), DCI_DT_INT, _T("Dequeue Ops"));
      value->addColumn(_T("ENQUEUE_COUNT"), DCI_DT_INT, _T("Enqueue Ops"));
      value->addColumn(_T("POSTS"), DCI_DT_INT, _T("Posts"));
      value->addColumn(_T("INITIATED_REQUESTS"), DCI_DT_INT, _T("Initiated Requests"));
      value->addColumn(_T("SUBSCRIPTIONS"), DCI_DT_INT, _T("Subscriptions"));
      value->addColumn(_T("TRANSACTIONS"), DCI_DT_INT, _T("Transactions"));
      value->addColumn(_T("START_TIME"), DCI_DT_INT, _T("Start Time"));
      value->addColumn(_T("RESTART_TIME"), DCI_DT_INT, _T("Restart Time"));
      value->addColumn(_T("ENV_FILE"), DCI_DT_STRING, _T("Environment File"));
      value->addColumn(_T("CMDLINE"), DCI_DT_STRING, _T("Command Line"));

      s_servers->forEach(FillServerInstanceTable, value);
   }
   else
   {
      rc = SYSINFO_RC_ERROR;
   }
   s_lock.unlock();
   return rc;
}

/**
 * Fill server table
 */
static EnumerationCallbackResult FillServerTable(const uint32_t& key, TuxedoServer *server, Table *table)
{
   table->addRow();
   table->set(0, (INT32)server->m_summary.m_groupId);
   table->set(1, (INT32)server->m_summary.m_baseId);
   table->set(2, server->m_summary.m_name);
   table->set(3, server->m_summary.m_group);
   table->set(4, (INT32)server->m_summary.m_min);
   table->set(5, (INT32)server->m_summary.m_max);
   table->set(6, (INT32)server->m_running);
   table->set(7, server->m_summary.m_lmid);
   table->set(8, (INT32)server->m_summary.m_curThreads);
   table->set(9, (INT32)server->m_summary.m_minThreads);
   table->set(10, (INT32)server->m_summary.m_maxThreads);
   table->set(11, (INT32)server->m_summary.m_activeRequests);
   table->set(12, (INT32)server->m_summary.m_totalRequests);
   table->set(13, (INT32)server->m_summary.m_totalWorkloads);
   table->set(14, (INT32)server->m_summary.m_convCount);
   table->set(15, (INT32)server->m_summary.m_dequeueCount);
   table->set(16, (INT32)server->m_summary.m_enqueueCount);
   table->set(17, (INT32)server->m_summary.m_postCount);
   table->set(18, (INT32)server->m_summary.m_reqCount);
   table->set(19, (INT32)server->m_summary.m_subscribeCount);
   table->set(20, (INT32)server->m_summary.m_txnCount);
   table->set(21, server->m_summary.m_envFile);
   table->set(22, server->m_summary.m_cmdLine);
   return _CONTINUE;
}

/**
 * Handler for Tuxedo.Servers table
 */
LONG H_ServersTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   LONG rc = SYSINFO_RC_SUCCESS;

   s_lock.lock();
   if (s_servers != nullptr)
   {
      value->addColumn(_T("GROUP_ID"), DCI_DT_INT, _T("Group ID"), true);
      value->addColumn(_T("BASE_ID"), DCI_DT_INT, _T("Base ID"), true);
      value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"));
      value->addColumn(_T("GROUP_NAME"), DCI_DT_STRING, _T("Group Name"));
      value->addColumn(_T("MIN"), DCI_DT_INT, _T("Min"));
      value->addColumn(_T("MAX"), DCI_DT_INT, _T("Max"));
      value->addColumn(_T("RUNNING"), DCI_DT_INT, _T("Running"));
      value->addColumn(_T("LMID"), DCI_DT_STRING, _T("Machine"));
      value->addColumn(_T("THREADS"), DCI_DT_INT, _T("Threads"));
      value->addColumn(_T("THREADS_MIN"), DCI_DT_INT, _T("Min Threads"));
      value->addColumn(_T("THREADS_MAX"), DCI_DT_INT, _T("Max Threads"));
      value->addColumn(_T("ACTIVE_REQUESTS"), DCI_DT_INT, _T("Active Requests"));
      value->addColumn(_T("PROCESSED_REQUESTS"), DCI_DT_INT, _T("Processed Requests"));
      value->addColumn(_T("PROCESSED_WORKLOADS"), DCI_DT_INT, _T("Processed Workloads"));
      value->addColumn(_T("CONVERSATIONS"), DCI_DT_INT, _T("Conversations"));
      value->addColumn(_T("DEQUEUE_COUNT"), DCI_DT_INT, _T("Dequeue Ops"));
      value->addColumn(_T("ENQUEUE_COUNT"), DCI_DT_INT, _T("Enqueue Ops"));
      value->addColumn(_T("POSTS"), DCI_DT_INT, _T("Posts"));
      value->addColumn(_T("INITIATED_REQUESTS"), DCI_DT_INT, _T("Initiated Requests"));
      value->addColumn(_T("SUBSCRIPTIONS"), DCI_DT_INT, _T("Subscriptions"));
      value->addColumn(_T("TRANSACTIONS"), DCI_DT_INT, _T("Transactions"));
      value->addColumn(_T("ENV_FILE"), DCI_DT_STRING, _T("Environment File"));
      value->addColumn(_T("CMDLINE"), DCI_DT_STRING, _T("Command Line"));

      s_servers->forEach(FillServerTable, value);
   }
   else
   {
      rc = SYSINFO_RC_ERROR;
   }
   s_lock.unlock();
   return rc;
}

/**
 * Handler for Tuxedo.ServerInstance.* parameters
 */
LONG H_ServerInstanceInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR buffer[32];
   if (!AgentGetParameterArg(param, 1, buffer, 32))
      return SYSINFO_RC_UNSUPPORTED;

   TCHAR *eptr;
   long groupId = _tcstol(buffer, &eptr, 10);
   if ((groupId < 0) || (*eptr != 0))
      return SYSINFO_RC_UNSUPPORTED;

   if (!AgentGetParameterArg(param, 2, buffer, 32))
      return SYSINFO_RC_UNSUPPORTED;

   long serverId = _tcstol(buffer, &eptr, 10);
   if ((serverId < 0) || (*eptr != 0))
      return SYSINFO_RC_UNSUPPORTED;

   LONG rc = SYSINFO_RC_SUCCESS;

   s_lock.lock();
   if ((s_servers != nullptr) && (s_serverInstances != nullptr))
   {
      const TuxedoServerInstance *s = s_serverInstances->get(static_cast<uint32_t>((groupId << 16) | serverId));
      if (s != nullptr)
      {
         switch(*arg)
         {
            case 'A':
               ret_int(value, (INT32)s->m_activeRequests);
               break;
            case 'B':
               ret_int(value, (INT32)s->m_baseId);
               break;
            case 'C':
               ret_mbstring(value, s->m_cmdLine);
               break;
            case 'c':
               ret_mbstring(value, s->m_currService);
               break;
            case 'G':
               ret_int(value, (INT32)s->m_generation);
               break;
            case 'g':
               ret_mbstring(value, s->m_group);
               break;
            case 'M':
               ret_mbstring(value, s->m_lmid);
               break;
            case 'N':
               ret_mbstring(value, s->m_name);
               break;
            case 'P':
               ret_int(value, (INT32)s->m_pid);
               break;
            case 'R':
               ret_int(value, (INT32)s->m_totalRequests);
               break;
            case 'S':
               ret_mbstring(value, s->m_state);
               break;
            case 'W':
               ret_int(value, (INT32)s->m_totalWorkloads);
               break;
            default:
               rc = SYSINFO_RC_UNSUPPORTED;
               break;
         }
      }
      else
      {
         rc = SYSINFO_RC_UNSUPPORTED;
      }
   }
   else
   {
      rc = SYSINFO_RC_ERROR;
   }
   s_lock.unlock();
   return rc;
}

/**
 * Handler for Tuxedo.Server.* parameters
 */
LONG H_ServerInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR buffer[32];
   if (!AgentGetParameterArg(param, 1, buffer, 32))
      return SYSINFO_RC_UNSUPPORTED;

   TCHAR *eptr;
   long groupId = _tcstol(buffer, &eptr, 10);
   if ((groupId < 0) || (*eptr != 0))
      return SYSINFO_RC_UNSUPPORTED;

   if (!AgentGetParameterArg(param, 2, buffer, 32))
      return SYSINFO_RC_UNSUPPORTED;

   long baseId = _tcstol(buffer, &eptr, 10);
   if ((baseId < 0) || (*eptr != 0))
      return SYSINFO_RC_UNSUPPORTED;

   LONG rc = SYSINFO_RC_SUCCESS;

   s_lock.lock();
   if (s_servers != nullptr)
   {
      const TuxedoServer *s = s_servers->get(static_cast<uint32_t>((groupId << 16) | baseId));
      if (s != nullptr)
      {
         switch(*arg)
         {
            case 'A':
               ret_int(value, (INT32)s->m_summary.m_activeRequests);
               break;
            case 'C':
               ret_mbstring(value, s->m_summary.m_cmdLine);
               break;
            case 'g':
               ret_mbstring(value, s->m_summary.m_group);
               break;
            case 'i':
               ret_int(value, (INT32)s->m_summary.m_min);
               break;
            case 'M':
               ret_mbstring(value, s->m_summary.m_lmid);
               break;
            case 'N':
               ret_mbstring(value, s->m_summary.m_name);
               break;
            case 'R':
               ret_int(value, (INT32)s->m_summary.m_totalRequests);
               break;
            case 'r':
               ret_int(value, (INT32)s->m_running);
               break;
            case 'W':
               ret_int(value, (INT32)s->m_summary.m_totalWorkloads);
               break;
            case 'x':
               ret_int(value, (INT32)s->m_summary.m_max);
               break;
            default:
               rc = SYSINFO_RC_UNSUPPORTED;
               break;
         }
      }
      else
      {
         rc = SYSINFO_RC_NO_SUCH_INSTANCE;
      }
   }
   else
   {
      rc = SYSINFO_RC_ERROR;
   }
   s_lock.unlock();
   return rc;
}
