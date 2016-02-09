/*
** NetXMS Tuxedo subagent
** Copyright (C) 2014 Raden Solutions
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
class TuxedoServer
{
public:
   long m_id;
   long m_baseId;
   char m_group[32];
   char m_name[MAX_PATH];
   char m_state[16];
   char m_cmdLine[1024];
   char m_envFile[256];
   char m_rqAddr[32];
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
   long m_count;
   long m_running;

   TuxedoServer(FBFR32 *fb, FLDOCC32 index);
   TuxedoServer(TuxedoServer *src);
   void addInstance(TuxedoServer *s);
};

/**
 * Create server object from FB
 */
TuxedoServer::TuxedoServer(FBFR32 *fb, FLDOCC32 index)
{
   m_id = 0;
   m_baseId = 0;
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
   m_count = 0;
   m_running = 0;

   CFget32(fb, TA_SRVID, index, (char *)&m_id, NULL, FLD_LONG);
   CFget32(fb, TA_BASESRVID, index, (char *)&m_baseId, NULL, FLD_LONG);
   CFgetString(fb, TA_SRVGRP, index, m_group, sizeof(m_group));
   CFgetString(fb, TA_SERVERNAME, index, m_name, sizeof(m_name));
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
 * Create server object from another server object
 */
TuxedoServer::TuxedoServer(TuxedoServer *src)
{
   m_id = src->m_id;
   m_baseId = src->m_baseId;
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
   m_count = 1;
   m_running = !strcmp(src->m_state, "ACTIVE") ? 1 : 0;
}

/**
 * Add new instance information
 */
void TuxedoServer::addInstance(TuxedoServer *s)
{
   m_count++;
   if (!strcmp(s->m_state, "ACTIVE"))
      m_running++;
   m_curThreads += s->m_curThreads;
   m_convCount += s->m_convCount;
   m_dequeueCount += s->m_dequeueCount;
   m_enqueueCount += s->m_enqueueCount;
   m_postCount += s->m_postCount;
   m_reqCount += s->m_reqCount;
   m_subscribeCount += s->m_subscribeCount;
   m_txnCount += s->m_txnCount;
   m_totalRequests += s->m_totalRequests;
   m_totalWorkloads += s->m_totalWorkloads;
   m_activeRequests += s->m_activeRequests;
}

/**
 * Service list
 */
static MUTEX s_lock = MutexCreate();
static time_t s_lastQuery = 0;
static ObjectArray<TuxedoServer> *s_serverInstances = NULL;
static ObjectArray<TuxedoServer> *s_servers = NULL;

/**
 * Query servers
 */
static void QueryServers()
{
   delete_and_null(s_serverInstances);
   delete_and_null(s_servers);

   if (!TuxedoConnect())
      AgentWriteDebugLog(3, _T("Tuxedo: tpinit() call failed (%hs)"), tpstrerrordetail(tperrno, 0));

   FBFR32 *fb = (FBFR32 *)tpalloc((char *)"FML32", NULL, 4096);
   CFchg32(fb, TA_OPERATION, 0, (char *)"GET", 0, FLD_STRING);
   CFchg32(fb, TA_CLASS, 0, (char *)"T_SERVER", 0, FLD_STRING);

   long flags = MIB_LOCAL;
   CFchg32(fb, TA_FLAGS, 0, (char *)&flags, 0, FLD_LONG);

   bool readMore = true;
   long rsplen = 262144;
   FBFR32 *rsp = (FBFR32 *)tpalloc((char *)"FML32", NULL, rsplen);
   while(readMore)
   {
      readMore = false;
      if (tpcall((char *)".TMIB", (char *)fb, 0, (char **)&rsp, &rsplen, 0) != -1)
      {
         if (s_serverInstances == NULL)
            s_serverInstances = new ObjectArray<TuxedoServer>(256, 256, true);

         long count = 0;
         CFget32(rsp, TA_OCCURS, 0, (char *)&count, NULL, FLD_LONG);
         for(int i = 0; i < (int)count; i++)
         {
            TuxedoServer *s = new TuxedoServer(rsp, (FLDOCC32)i);
            s_serverInstances->set((int)s->m_id, s);
         }

         long more = 0;
         CFget32(rsp, TA_MORE, 0, (char *)&more, NULL, FLD_LONG);
         if (more)
         {
         	CFchg32(fb, TA_OPERATION, 0, (char *)"GETNEXT", 0, FLD_STRING);

            char cursor[256];
            CFgetString(rsp, TA_CURSOR, 0, cursor, sizeof(cursor));
         	CFchg32(fb, TA_CURSOR, 0, cursor, 0, FLD_STRING);
         }
      }
      else
      {
         AgentWriteDebugLog(3, _T("Tuxedo: tpcall() call failed (%hs)"), tpstrerrordetail(tperrno, 0));
      }
   }

   tpfree((char *)rsp);
   tpfree((char *)fb);
   TuxedoDisconnect();

   // Create summary list
   s_servers = new ObjectArray<TuxedoServer>(1024, 1024, true);
   TuxedoServer *base = NULL;
   for(int id = 0; id < s_serverInstances->size(); id++)
   {
      TuxedoServer *s = s_serverInstances->get(id);
      if (s == NULL)
         continue;

      if (s->m_baseId == s->m_id)
      {
         if (base != NULL)
            s_servers->set(base->m_baseId, base);
         base = new TuxedoServer(s);
      }
      else if (base != NULL)
      {
         base->addInstance(s);
      }
   }
   if (base != NULL)
      s_servers->set(base->m_baseId, base);
}

/**
 * Handler for Tuxedo.ServerInstances list
 */
LONG H_ServerInstancesList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   LONG rc = SYSINFO_RC_SUCCESS;

   MutexLock(s_lock);
   if (time(NULL) - s_lastQuery > 5)
   {
      QueryServers();
      s_lastQuery = time(NULL);
   }

   if (s_serverInstances != NULL)
   {
      for(int i = 0; i < s_serverInstances->size(); i++)
      {
         TuxedoServer *s = s_serverInstances->get(i);
         if (s != NULL)
         {
            value->add((INT32)s->m_id);
         }
      }
   }
   else
   {
      rc = SYSINFO_RC_ERROR;
   }
   MutexUnlock(s_lock);
   return rc;
}

/**
 * Handler for Tuxedo.Servers list
 */
LONG H_ServersList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   LONG rc = SYSINFO_RC_SUCCESS;

   MutexLock(s_lock);
   if (time(NULL) - s_lastQuery > 5)
   {
      QueryServers();
      s_lastQuery = time(NULL);
   }

   if (s_servers != NULL)
   {
      for(int i = 0; i < s_servers->size(); i++)
      {
         TuxedoServer *s = s_servers->get(i);
         if (s != NULL)
         {
            value->add((INT32)s->m_baseId);
         }
      }
   }
   else
   {
      rc = SYSINFO_RC_ERROR;
   }
   MutexUnlock(s_lock);
   return rc;
}

/**
 * Handler for Tuxedo.ServerInstances table
 */
LONG H_ServerInstancesTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   LONG rc = SYSINFO_RC_SUCCESS;

   MutexLock(s_lock);
   if (time(NULL) - s_lastQuery > 5)
   {
      QueryServers();
      s_lastQuery = time(NULL);
   }

   if (s_serverInstances != NULL)
   {
      value->addColumn(_T("ID"), DCI_DT_INT, _T("ID"), true);
      value->addColumn(_T("BASE_ID"), DCI_DT_INT, _T("Base ID"));
      value->addColumn(_T("GROUP"), DCI_DT_STRING, _T("Group"));
      value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"));
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

      for(int i = 0; i < s_serverInstances->size(); i++)
      {
         TuxedoServer *s = s_serverInstances->get(i);
         if (s == NULL)
            continue;

         value->addRow();
         value->set(0, (INT32)s->m_id);
         value->set(1, (INT32)s->m_baseId);
         value->set(2, s->m_group);
         value->set(3, s->m_name);
         value->set(4, s->m_state);
         value->set(5, s->m_rqAddr);
         value->set(6, s->m_lmid);
         value->set(7, (INT32)s->m_pid);
         value->set(8, (INT32)s->m_generation);
         value->set(9, (INT32)s->m_curThreads);
         value->set(10, (INT32)s->m_minThreads);
         value->set(11, (INT32)s->m_maxThreads);
         value->set(12, (INT32)s->m_activeRequests);
         value->set(13, s->m_currService);
         value->set(14, (INT32)s->m_tranLevel);
         value->set(15, (INT32)s->m_totalRequests);
         value->set(16, (INT32)s->m_totalWorkloads);
         value->set(17, (INT32)s->m_convCount);
         value->set(18, (INT32)s->m_dequeueCount);
         value->set(19, (INT32)s->m_enqueueCount);
         value->set(20, (INT32)s->m_postCount);
         value->set(21, (INT32)s->m_reqCount);
         value->set(22, (INT32)s->m_subscribeCount);
         value->set(23, (INT32)s->m_txnCount);
         value->set(24, (INT32)s->m_timeStart);
         value->set(25, (INT32)s->m_timeRestart);
         value->set(26, s->m_envFile);
         value->set(27, s->m_cmdLine);
      }
   }
   else
   {
      rc = SYSINFO_RC_ERROR;
   }
   MutexUnlock(s_lock);
   return rc;
}

/**
 * Handler for Tuxedo.Servers table
 */
LONG H_ServersTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   LONG rc = SYSINFO_RC_SUCCESS;

   MutexLock(s_lock);
   if (time(NULL) - s_lastQuery > 5)
   {
      QueryServers();
      s_lastQuery = time(NULL);
   }

   if (s_servers != NULL)
   {
      value->addColumn(_T("BASE_ID"), DCI_DT_INT, _T("Base ID"), true);
      value->addColumn(_T("GROUP"), DCI_DT_STRING, _T("Group"));
      value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"));
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

      for(int i = 0; i < s_servers->size(); i++)
      {
         TuxedoServer *s = s_servers->get(i);
         if (s == NULL)
            continue;

         value->addRow();
         value->set(0, (INT32)s->m_baseId);
         value->set(1, s->m_group);
         value->set(2, s->m_name);
         value->set(3, (INT32)s->m_min);
         value->set(4, (INT32)s->m_max);
         value->set(5, (INT32)s->m_running);
         value->set(6, s->m_lmid);
         value->set(7, (INT32)s->m_curThreads);
         value->set(8, (INT32)s->m_minThreads);
         value->set(9, (INT32)s->m_maxThreads);
         value->set(10, (INT32)s->m_activeRequests);
         value->set(11, (INT32)s->m_totalRequests);
         value->set(12, (INT32)s->m_totalWorkloads);
         value->set(13, (INT32)s->m_convCount);
         value->set(14, (INT32)s->m_dequeueCount);
         value->set(15, (INT32)s->m_enqueueCount);
         value->set(16, (INT32)s->m_postCount);
         value->set(17, (INT32)s->m_reqCount);
         value->set(18, (INT32)s->m_subscribeCount);
         value->set(19, (INT32)s->m_txnCount);
         value->set(20, s->m_envFile);
         value->set(21, s->m_cmdLine);
      }
   }
   else
   {
      rc = SYSINFO_RC_ERROR;
   }
   MutexUnlock(s_lock);
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
   long id = _tcstol(buffer, &eptr, 10);
   if ((id < 0) || (*eptr != 0))
      return SYSINFO_RC_UNSUPPORTED;

   LONG rc = SYSINFO_RC_SUCCESS;

   MutexLock(s_lock);
   if (time(NULL) - s_lastQuery > 5)
   {
      QueryServers();
      s_lastQuery = time(NULL);
   }

   if (s_serverInstances != NULL)
   {
      TuxedoServer *s = s_serverInstances->get(id);
      if (s != NULL)
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
   MutexUnlock(s_lock);
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
   long id = _tcstol(buffer, &eptr, 10);
   if ((id < 0) || (*eptr != 0))
      return SYSINFO_RC_UNSUPPORTED;

   LONG rc = SYSINFO_RC_SUCCESS;

   MutexLock(s_lock);
   if (time(NULL) - s_lastQuery > 5)
   {
      QueryServers();
      s_lastQuery = time(NULL);
   }

   if (s_servers != NULL)
   {
      TuxedoServer *s = s_servers->get(id);
      if (s != NULL)
      {
         switch(*arg)
         {
            case 'A':
               ret_int(value, (INT32)s->m_activeRequests);
               break;
            case 'C':
               ret_mbstring(value, s->m_cmdLine);
               break;
            case 'g':
               ret_mbstring(value, s->m_group);
               break;
            case 'i':
               ret_int(value, (INT32)s->m_min);
               break;
            case 'M':
               ret_mbstring(value, s->m_lmid);
               break;
            case 'N':
               ret_mbstring(value, s->m_name);
               break;
            case 'R':
               ret_int(value, (INT32)s->m_totalRequests);
               break;
            case 'r':
               ret_int(value, (INT32)s->m_running);
               break;
            case 'W':
               ret_int(value, (INT32)s->m_totalWorkloads);
               break;
            case 'x':
               ret_int(value, (INT32)s->m_max);
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
   MutexUnlock(s_lock);
   return rc;
}
