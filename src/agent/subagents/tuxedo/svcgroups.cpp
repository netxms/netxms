/*
** NetXMS Tuxedo subagent
** Copyright (C) 2014-2021 Raden Solutions
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
** File: svcgroups.cpp
**
**/

#include "tuxedo_subagent.h"

/**
 * Tuxedo service information
 */
class TuxedoServiceGropup
{
public:
   TCHAR m_svcName[32];
   TCHAR m_srvGroup[32];
   long m_groupNumber;
   char m_rqAddr[32];
   char m_lmid[MAX_LMID_LEN];
   char m_state[16];
   char m_routingName[16];
   long m_load;
   long m_priority;
   long m_completed;
   long m_queued;
#ifdef NDRX_VERSION
   long m_successfulRequests;
   long m_failedRequests;
   long m_lastExecTime;
   long m_maxExecTime;
   long m_minExecTime;
#endif

   TuxedoServiceGropup(FBFR32 *fb, FLDOCC32 index);
};

/**
 * Create service object from FB
 */
TuxedoServiceGropup::TuxedoServiceGropup(FBFR32 *fb, FLDOCC32 index)
{
   m_svcName[0] = 0;
   m_srvGroup[0] = 0;
   m_groupNumber = 0;
   m_rqAddr[0] = 0;
   m_lmid[0] = 0;
   m_state[0] = 0;
   m_routingName[0] = 0;
   m_load = 0;
   m_priority = 0;
   m_completed = 0;
   m_queued = 0;
#ifdef NDRX_VERSION
   m_successfulRequests = 0;
   m_failedRequests = 0;
   m_lastExecTime = 0;
   m_maxExecTime = 0;
   m_minExecTime = 0;
#endif

#ifdef UNICODE
   char buffer[32] = "";
   CFgetString(fb, TA_SERVICENAME, index, buffer, sizeof(buffer));
   mb_to_wchar(buffer, -1, m_svcName, 32);
   CFgetString(fb, TA_SRVGRP, index, buffer, sizeof(buffer));
   mb_to_wchar(buffer, -1, m_srvGroup, 32);
#else
   CFgetString(fb, TA_SERVICENAME, index, m_svcName, sizeof(m_svcName));
   CFgetString(fb, TA_SRVGRP, index, m_srvGroup, sizeof(m_srvGroup));
#endif
   CFget32(fb, TA_GRPNO, index, (char *)&m_groupNumber, NULL, FLD_LONG);
   CFgetString(fb, TA_RQADDR, index, m_rqAddr, sizeof(m_rqAddr));
   CFgetString(fb, TA_LMID, index, m_lmid, sizeof(m_lmid));
   CFgetString(fb, TA_STATE, index, m_state, sizeof(m_state));
   CFgetString(fb, TA_ROUTINGNAME, index, m_routingName, sizeof(m_routingName));
   CFget32(fb, TA_LOAD, index, (char *)&m_load, NULL, FLD_LONG);
   CFget32(fb, TA_PRIO, index, (char *)&m_priority, NULL, FLD_LONG);
   CFget32(fb, TA_NCOMPLETED, index, (char *)&m_completed, NULL, FLD_LONG);
   CFget32(fb, TA_NQUEUED, index, (char *)&m_queued, NULL, FLD_LONG);
#ifdef NDRX_VERSION
   CFget32(fb, TA_TOTSUCCNUM, index, (char *)&m_successfulRequests, NULL, FLD_LONG);
   CFget32(fb, TA_TOTSFAILNUM, index, (char *)&m_failedRequests, NULL, FLD_LONG);
   CFget32(fb, TA_LASTEXECTIMEUSEC, index, (char *)&m_lastExecTime, NULL, FLD_LONG);
   CFget32(fb, TA_MAXEXECTIMEUSEC, index, (char *)&m_maxExecTime, NULL, FLD_LONG);
   CFget32(fb, TA_MINEXECTIMEUSEC, index, (char *)&m_minExecTime, NULL, FLD_LONG);
#endif
}

/**
 * Service list
 */
static Mutex s_lock;
static StringObjectMap<TuxedoServiceGropup> *s_serviceGroups = NULL;

/**
 * Reset services cache
 */
void TuxedoResetServiceGroups()
{
   s_lock.lock();
   delete_and_null(s_serviceGroups);
   s_lock.unlock();
}

/**
 * Query services
 */
void TuxedoQueryServiceGroups()
{
   StringObjectMap<TuxedoServiceGropup> *serviceGroups = new StringObjectMap<TuxedoServiceGropup>(Ownership::True);

   FBFR32 *fb = (FBFR32 *)tpalloc((char *)"FML32", NULL, 4096);
   CFchg32(fb, TA_OPERATION, 0, (char *)"GET", 0, FLD_STRING);
   CFchg32(fb, TA_CLASS, 0, (char *)"T_SVCGRP", 0, FLD_STRING);

   char lmid[MAX_LMID_LEN];
   if (g_tuxedoLocalMachineFilter && TuxedoGetLocalMachineID(lmid))
   {
      CFchg32(fb, TA_LMID, 0, lmid, 0, FLD_STRING);
   }

   long flags = ((g_tuxedoQueryLocalData & LOCAL_DATA_SERVERS) ? MIB_LOCAL : 0);
   CFchg32(fb, TA_FLAGS, 0, (char *)&flags, 0, FLD_LONG);

   bool readMore = true;
   long rsplen = 262144;
   FBFR32 *rsp = (FBFR32 *)tpalloc((char *)"FML32", NULL, rsplen);
   while(readMore)
   {
      readMore = false;
      if (tpcall((char *)".TMIB", (char *)fb, 0, (char **)&rsp, &rsplen, 0) != -1)
      {
         long count = 0;
         CFget32(rsp, TA_OCCURS, 0, (char *)&count, NULL, FLD_LONG);
         for(int i = 0; i < (int)count; i++)
         {
            TuxedoServiceGropup *s = new TuxedoServiceGropup(rsp, (FLDOCC32)i);
            TCHAR key[128];
            _sntprintf(key, 128, _T("%s/%s/%hs"), s->m_svcName, s->m_srvGroup, s->m_lmid);
            serviceGroups->set(key, s);
         }

         long more = 0;
         CFget32(rsp, TA_MORE, 0, (char *)&more, NULL, FLD_LONG);
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
         delete_and_null(serviceGroups);
      }
   }

   tpfree((char *)rsp);
   tpfree((char *)fb);

   s_lock.lock();
   delete s_serviceGroups;
   s_serviceGroups = serviceGroups;
   s_lock.unlock();
}

/**
 * Handler for Tuxedo.ServiceGroups list
 */
LONG H_ServiceGroupsList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   LONG rc = SYSINFO_RC_SUCCESS;

   s_lock.lock();
   if (s_serviceGroups != NULL)
   {
      StructArray<KeyValuePair<TuxedoServiceGropup>> *serviceGroups = s_serviceGroups->toArray();
      for(int i = 0; i < serviceGroups->size(); i++)
      {
         auto group = serviceGroups->get(i)->value;
         TCHAR buffer[128];
         _sntprintf(buffer, 128, _T("%s,%s,%hs"), group->m_svcName, group->m_srvGroup, group->m_lmid);
         value->add(buffer);
      }
      delete serviceGroups;
   }
   else
   {
      rc = SYSINFO_RC_ERROR;
   }
   s_lock.unlock();
   return rc;
}

/**
 * Handler for Tuxedo.ServiceGroups table
 */
LONG H_ServiceGroupsTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   LONG rc = SYSINFO_RC_SUCCESS;

   s_lock.lock();
   if (s_serviceGroups != NULL)
   {
      value->addColumn(_T("SVCNAME"), DCI_DT_STRING, _T("Service name"), true);
      value->addColumn(_T("SRVGROUP"), DCI_DT_STRING, _T("Server group"), true);
      value->addColumn(_T("LMID"), DCI_DT_STRING, _T("LMID"), true);
      value->addColumn(_T("GROUPNO"), DCI_DT_INT, _T("Group number"));
      value->addColumn(_T("RQADDR"), DCI_DT_STRING, _T("Request address"));
      value->addColumn(_T("STATE"), DCI_DT_STRING, _T("State"));
      value->addColumn(_T("RT_NAME"), DCI_DT_STRING, _T("Routing name"));
      value->addColumn(_T("LOAD"), DCI_DT_INT, _T("Load"));
      value->addColumn(_T("PRIO"), DCI_DT_INT, _T("Priority"));
      value->addColumn(_T("COMPLETED"), DCI_DT_INT, _T("Completed requests"));
      value->addColumn(_T("QUEUED"), DCI_DT_INT, _T("Queued requests"));
#ifdef NDRX_VERSION
      value->addColumn(_T("SUCCESSFUL"), DCI_DT_INT, _T("Successful requests"));
      value->addColumn(_T("FAILED"), DCI_DT_INT, _T("Failed Requests"));
      value->addColumn(_T("EXECTIME_LAST"), DCI_DT_INT, _T("Last execution time"));
      value->addColumn(_T("EXECTIME_MAX"), DCI_DT_INT, _T("Failed Requests"));
      value->addColumn(_T("EXECTIME_MIN"), DCI_DT_INT, _T("Failed Requests"));
#endif

      StructArray<KeyValuePair<TuxedoServiceGropup>> *serviceGroups = s_serviceGroups->toArray();
      for(int i = 0; i < serviceGroups->size(); i++)
      {
         value->addRow();
         auto group = serviceGroups->get(i)->value;
         value->set(0, group->m_svcName);
         value->set(1, group->m_srvGroup);
         value->set(2, group->m_lmid);
         value->set(3, (INT32)group->m_groupNumber);
         value->set(4, group->m_rqAddr);
         value->set(5, group->m_state);
         value->set(6, group->m_routingName);
         value->set(7, (INT32)group->m_load);
         value->set(8, (INT32)group->m_priority);
         value->set(9, (INT32)group->m_completed);
         value->set(10, (INT32)group->m_queued);
#ifdef NDRX_VERSION
         value->set(11, (INT32)group->m_successfulRequests);
         value->set(12, (INT32)group->m_failedRequests);
         value->set(13, (INT32)group->m_lastExecTime);
         value->set(14, (INT32)group->m_maxExecTime);
         value->set(15, (INT32)group->m_minExecTime);
#endif
      }
      delete serviceGroups;
   }
   else
   {
      rc = SYSINFO_RC_ERROR;
   }
   s_lock.unlock();
   return rc;
}

/**
 * Handler for Tuxedo.ServiceGroup.* parameters
 */
LONG H_ServiceGroupInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR serviceName[32], serverGroup[32];
   char lmid[MAX_LMID_LEN];
   if (!AgentGetParameterArg(param, 1, serviceName, 32) ||
       !AgentGetParameterArg(param, 2, serverGroup, 32) ||
       !AgentGetParameterArgA(param, 3, lmid, MAX_LMID_LEN))
      return SYSINFO_RC_UNSUPPORTED;

   if (lmid[0] == 0)
      TuxedoGetLocalMachineID(lmid);

   TCHAR key[128];
   _sntprintf(key, 128, _T("%s/%s/%hs"), serviceName, serverGroup, lmid);

   LONG rc = SYSINFO_RC_SUCCESS;

   s_lock.lock();
   if (s_serviceGroups != NULL)
   {
      TuxedoServiceGropup *s = s_serviceGroups->get(key);
      if (s != NULL)
      {
         switch(*arg)
         {
            case 'A':
               ret_mbstring(value, s->m_rqAddr);
               break;
            case 'C':
               ret_int(value, (INT32)s->m_completed);
               break;
            case 'L':
               ret_int(value, (INT32)s->m_load);
               break;
            case 'M':
               ret_mbstring(value, s->m_lmid);
               break;
            case 'P':
               ret_int(value, (INT32)s->m_priority);
               break;
            case 'Q':
               ret_int(value, (INT32)s->m_queued);
               break;
            case 'R':
               ret_mbstring(value, s->m_routingName);
               break;
            case 'S':
               ret_mbstring(value, s->m_state);
               break;
#ifdef NDRX_VERSION
            case 'e':
               ret_int(value, (INT32)s->m_lastExecTime);
               break;
            case 'f':
               ret_int(value, (INT32)s->m_failedRequests);
               break;
            case 'm':
               ret_int(value, (INT32)s->m_minExecTime);
               break;
            case 's':
               ret_int(value, (INT32)s->m_successfulRequests);
               break;
            case 'x':
               ret_int(value, (INT32)s->m_maxExecTime);
               break;
#endif
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
