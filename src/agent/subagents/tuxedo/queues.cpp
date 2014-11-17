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
** File: queues.cpp
**
**/

#include "tuxedo_subagent.h"

/**
 * Tuxedo queue information
 */
class TuxedoQueue
{
public:
   char m_name[32];
   char m_lmid[64];
   char m_serverName[MAX_PATH];
   char m_state[16];
   long m_serverCount;
   long m_requestsTotal;
   long m_requestsCurrent;
   long m_workloadsTotal;
   long m_workloadsCurrent;

   TuxedoQueue(FBFR32 *fb, FLDOCC32 index);
};

/**
 * Create queue object from FB
 */
TuxedoQueue::TuxedoQueue(FBFR32 *fb, FLDOCC32 index)
{
   m_name[0] = 0;
   m_lmid[0] = 0;
   m_serverName[0] = 0;
   m_state[0] = 0;
   m_serverCount = 0;
   m_requestsTotal = 0;
   m_requestsCurrent = 0;
   m_workloadsTotal = 0;
   m_workloadsCurrent = 0;

   CFgetString(fb, TA_RQADDR, index, m_name, sizeof(m_name));
   CFgetString(fb, TA_LMID, index, m_lmid, sizeof(m_lmid));
   CFgetString(fb, TA_SERVERNAME, index, m_serverName, sizeof(m_serverName));
   CFgetString(fb, TA_STATE, index, m_state, sizeof(m_state));
   CFget32(fb, TA_SERVERCNT, index, (char *)&m_serverCount, NULL, FLD_LONG);
   CFget32(fb, TA_TOTNQUEUED, index, (char *)&m_requestsTotal, NULL, FLD_LONG);
   CFget32(fb, TA_NQUEUED, index, (char *)&m_requestsCurrent, NULL, FLD_LONG);
   CFget32(fb, TA_TOTWKQUEUED, index, (char *)&m_workloadsTotal, NULL, FLD_LONG);
   CFget32(fb, TA_WKQUEUED, index, (char *)&m_workloadsCurrent, NULL, FLD_LONG);
}

/**
 * Queue list
 */
static MUTEX s_lock = MutexCreate();
static time_t s_lastQuery = 0;
static ObjectArray<TuxedoQueue> *s_queues = NULL;

/**
 * Query queues
 */
static void QueryQueues()
{
   delete_and_null(s_queues);

   if (!tpinit(NULL))
   {
      AgentWriteDebugLog(3, _T("Tuxedo: tpinit() call failed (%d)"), errno);
      return;
   }

	FBFR32 *fb = (FBFR32 *)tpalloc("FML32", NULL, 4096);
   if (fb == NULL)
   {
      AgentWriteDebugLog(3, _T("Tuxedo: Falloc32() call failed (%d)"), errno);
      return;
   }

	CFchg32(fb, TA_OPERATION, 0, (char *)"GET", 0, FLD_STRING);
	CFchg32(fb, TA_CLASS, 0, (char *)"T_QUEUE", 0, FLD_STRING);

   long flags = 65536; //MIB_LOCAL;
	CFchg32(fb, TA_FLAGS, 0, (char *)&flags, 0, FLD_LONG);

   bool readMore = true;
   long rsplen = 262144;
   FBFR32 *rsp = (FBFR32 *)tpalloc("FML32", NULL, rsplen);
   while(readMore)
   {
      readMore = false;
      if (tpcall(".TMIB", (char *)fb, 0, (char **)&rsp, &rsplen, 0) != -1)
      {
         if (s_queues == NULL)
            s_queues = new ObjectArray<TuxedoQueue>(256, 256, true);

         long count = 0;
         CFget32(rsp, TA_OCCURS, 0, (char *)&count, NULL, FLD_LONG);
         for(int i = 0; i < (int)count; i++)
         {
            TuxedoQueue *q = new TuxedoQueue(rsp, (FLDOCC32)i);
            s_queues->add(q);
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
         AgentWriteDebugLog(3, _T("Tuxedo: tpcall() call failed (%d)"), errno);
      }
   }

   tpfree((char *)rsp);
   tpfree((char *)fb);
}

/**
 * Handler for Tuxedo.Queues list
 */
LONG H_QueuesList(const TCHAR *param, const TCHAR *arg, StringList *value)
{
   LONG rc = SYSINFO_RC_SUCCESS;

   MutexLock(s_lock);
   if (time(NULL) - s_lastQuery > 5)
   {
      QueryQueues();
      s_lastQuery = time(NULL);
   }

   if (s_queues != NULL)
   {
      for(int i = 0; i < s_queues->size(); i++)
      {
#ifdef UNICODE
         value->addPreallocated(WideStringFromMBString(s_queues->get(i)->m_name));
#else
         value->add(s_queues->get(i)->m_name);
#endif
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
 * Handler for Tuxedo.Queues table
 */
LONG H_QueuesTable(const TCHAR *param, const TCHAR *arg, Table *value)
{
   LONG rc = SYSINFO_RC_SUCCESS;

   MutexLock(s_lock);
   if (time(NULL) - s_lastQuery > 5)
   {
      QueryQueues();
      s_lastQuery = time(NULL);
   }

   if (s_queues != NULL)
   {
      value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"), true);
      value->addColumn(_T("MACHINE"), DCI_DT_STRING, _T("Machine"));
      value->addColumn(_T("SERVER"), DCI_DT_STRING, _T("Server"));
      value->addColumn(_T("STATE"), DCI_DT_STRING, _T("State"));
      value->addColumn(_T("SRV_COUNT"), DCI_DT_INT, _T("Server Count"));
      value->addColumn(_T("RQ_TOTAL"), DCI_DT_INT, _T("Total Requests"));
      value->addColumn(_T("RQ_CURRENT"), DCI_DT_INT, _T("Current Requests"));
      value->addColumn(_T("WK_TOTAL"), DCI_DT_INT, _T("Total Workloads"));
      value->addColumn(_T("WK_CURRENT"), DCI_DT_INT, _T("Current Workloads"));

      for(int i = 0; i < s_queues->size(); i++)
      {
         value->addRow();
         TuxedoQueue *q = s_queues->get(i);
#ifdef UNICODE
         value->setPreallocated(0, WideStringFromMBString(q->m_name));
         value->setPreallocated(1, WideStringFromMBString(q->m_lmid));
         value->setPreallocated(2, WideStringFromMBString(q->m_serverName));
         value->setPreallocated(3, WideStringFromMBString(q->m_state));
#else
         value->set(0, q->m_name);
         value->set(1, q->m_lmid);
         value->set(2, q->m_serverName);
         value->set(3, q->m_state);
#endif
         value->set(4, q->m_serverCount);
         value->set(5, q->m_requestsTotal);
         value->set(6, q->m_requestsCurrent);
         value->set(7, q->m_workloadsTotal);
         value->set(8, q->m_workloadsCurrent);
      }
   }
   else
   {
      rc = SYSINFO_RC_ERROR;
   }
   MutexUnlock(s_lock);
   return rc;
}
