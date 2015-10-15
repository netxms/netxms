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
** File: clients.cpp
**
**/

#include "tuxedo_subagent.h"

/**
 * Tuxedo client information
 */
class TuxedoClient
{
public:
   TCHAR m_id[80];
   char m_name[32];
   char m_lmid[64];
   char m_state[16];
   char m_serverGroup[32];
   char m_userName[32];
   long m_idleTime;
   long m_pid;
   char m_wsc[2];
   char m_wsClientId[80];
   long m_convCount;
   long m_dequeueCount;
   long m_enqueueCount;
   long m_postCount;
   long m_requestCount;
   long m_subscribeCount;
   long m_tranCount;
   long m_activeConv;
   long m_activeRequests;
   char m_netAddr[256];
   char m_encBits[8];

   TuxedoClient(FBFR32 *fb, FLDOCC32 index);
};

/**
 * Create queue object from FB
 */
TuxedoClient::TuxedoClient(FBFR32 *fb, FLDOCC32 index)
{
   m_id[0] = 0;
   m_name[0] = 0;
   m_lmid[0] = 0;
   m_state[0] = 0;
   m_serverGroup[0] = 0;
   m_userName[0] = 0;
   m_idleTime = 0;
   m_pid = 0;
   m_wsc[0] = 0;
   m_wsClientId[0] = 0;
   m_convCount = 0;
   m_dequeueCount = 0;
   m_enqueueCount = 0;
   m_postCount = 0;
   m_requestCount = 0;
   m_subscribeCount = 0;
   m_tranCount = 0;
   m_activeConv = 0;
   m_activeRequests = 0;
   m_netAddr[0] = 0;
   m_encBits[0] = 0;

#ifdef UNICODE
   char id[80] = "";
   CFgetString(fb, TA_CLIENTID, index, id, sizeof(id));
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, id, -1, m_id, 80);
#else
   CFgetString(fb, TA_CLIENTID, index, m_id, sizeof(m_id));
#endif
   CFgetString(fb, TA_CLTNAME, index, m_name, sizeof(m_name));
   CFgetString(fb, TA_LMID, index, m_lmid, sizeof(m_lmid));
   CFgetString(fb, TA_STATE, index, m_state, sizeof(m_state));
   CFgetString(fb, TA_SRVGRP, index, m_serverGroup, sizeof(m_serverGroup));
   CFgetString(fb, TA_USRNAME, index, m_userName, sizeof(m_userName));
   CFget32(fb, TA_IDLETIME, index, (char *)&m_idleTime, NULL, FLD_LONG);
   CFget32(fb, TA_PID, index, (char *)&m_pid, NULL, FLD_LONG);
   CFgetString(fb, TA_WSC, index, m_wsc, sizeof(m_wsc));
   CFgetString(fb, TA_WSHCLIENTID, index, m_wsClientId, sizeof(m_wsClientId));
   CFget32(fb, TA_NUMCONV, index, (char *)&m_convCount, NULL, FLD_LONG);
   CFget32(fb, TA_NUMDEQUEUE, index, (char *)&m_dequeueCount, NULL, FLD_LONG);
   CFget32(fb, TA_NUMENQUEUE, index, (char *)&m_enqueueCount, NULL, FLD_LONG);
   CFget32(fb, TA_NUMPOST, index, (char *)&m_postCount, NULL, FLD_LONG);
   CFget32(fb, TA_NUMREQ, index, (char *)&m_requestCount, NULL, FLD_LONG);
   CFget32(fb, TA_NUMSUBSCRIBE, index, (char *)&m_subscribeCount, NULL, FLD_LONG);
   CFget32(fb, TA_NUMTRAN, index, (char *)&m_tranCount, NULL, FLD_LONG);
   CFget32(fb, TA_CURCONV, index, (char *)&m_activeConv, NULL, FLD_LONG);
   CFget32(fb, TA_CURREQ, index, (char *)&m_activeRequests, NULL, FLD_LONG);
   CFgetString(fb, TA_NADDR, index, m_netAddr, sizeof(m_netAddr));
   CFgetString(fb, TA_CURENCRYPTBITS, index, m_encBits, sizeof(m_encBits));
}

/**
 * Queue list
 */
static MUTEX s_lock = MutexCreate();
static time_t s_lastQuery = 0;
static StringObjectMap<TuxedoClient> *s_clients = NULL;

/**
 * Query clients
 */
static void QueryClients()
{
   delete_and_null(s_clients);

   if (!TuxedoConnect())
      AgentWriteDebugLog(3, _T("Tuxedo: tpinit() call failed (%hs)"), tpstrerrordetail(tperrno, 0));

   FBFR32 *fb = (FBFR32 *)tpalloc((char *)"FML32", NULL, 4096);
   CFchg32(fb, TA_OPERATION, 0, (char *)"GET", 0, FLD_STRING);
   CFchg32(fb, TA_CLASS, 0, (char *)"T_CLIENT", 0, FLD_STRING);

   bool readMore = true;
   long rsplen = 262144;
   FBFR32 *rsp = (FBFR32 *)tpalloc((char *)"FML32", NULL, rsplen);
   while(readMore)
   {
      readMore = false;
      if (tpcall((char *)".TMIB", (char *)fb, 0, (char **)&rsp, &rsplen, 0) != -1)
      {
         if (s_clients == NULL)
            s_clients = new StringObjectMap<TuxedoClient>(true);

         long count = 0;
         CFget32(rsp, TA_OCCURS, 0, (char *)&count, NULL, FLD_LONG);
         for(int i = 0; i < (int)count; i++)
         {
            TuxedoClient *c = new TuxedoClient(rsp, (FLDOCC32)i);
            s_clients->set(c->m_id, c);
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
}

/**
 * Handler for Tuxedo.Clients list
 */
LONG H_ClientsList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   LONG rc = SYSINFO_RC_SUCCESS;

   MutexLock(s_lock);
   if (time(NULL) - s_lastQuery > 5)
   {
      QueryClients();
      s_lastQuery = time(NULL);
   }

   if (s_clients != NULL)
   {
      StructArray<KeyValuePair> *clients = s_clients->toArray();
      for(int i = 0; i < clients->size(); i++)
      {
         value->add(((TuxedoClient *)clients->get(i)->value)->m_id);
      }
      delete clients;
   }
   else
   {
      rc = SYSINFO_RC_ERROR;
   }
   MutexUnlock(s_lock);
   return rc;
}

/**
 * Handler for Tuxedo.Clients table
 */
LONG H_ClientsTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   LONG rc = SYSINFO_RC_SUCCESS;

   MutexLock(s_lock);
   if (time(NULL) - s_lastQuery > 5)
   {
      QueryClients();
      s_lastQuery = time(NULL);
   }

   if (s_clients != NULL)
   {
      value->addColumn(_T("ID"), DCI_DT_STRING, _T("ID"), true);
      value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"));
      value->addColumn(_T("MACHINE"), DCI_DT_STRING, _T("Machine"));
      value->addColumn(_T("STATE"), DCI_DT_STRING, _T("State"));
      value->addColumn(_T("GROUP"), DCI_DT_STRING, _T("Server Group"));
      value->addColumn(_T("USERNAME"), DCI_DT_STRING, _T("User Name"));
      value->addColumn(_T("IDLE_TIME"), DCI_DT_INT, _T("Idle Time"));
      value->addColumn(_T("PID"), DCI_DT_INT, _T("PID"));
      value->addColumn(_T("ACTIVE_REQUESTS"), DCI_DT_INT, _T("Active Requests"));
      value->addColumn(_T("ACTIVE_CONV"), DCI_DT_INT, _T("Active Conversations"));
      value->addColumn(_T("CONV_COUNT"), DCI_DT_INT, _T("Conversations"));
      value->addColumn(_T("DEQUEUE_COUNT"), DCI_DT_INT, _T("Dequeue Ops"));
      value->addColumn(_T("ENQUEUE_COUNT"), DCI_DT_INT, _T("Enqueue Ops"));
      value->addColumn(_T("POST_COUNT"), DCI_DT_INT, _T("Posts"));
      value->addColumn(_T("REQUEST_COUNT"), DCI_DT_INT, _T("Requests"));
      value->addColumn(_T("SUBSCR_COUNT"), DCI_DT_INT, _T("Subscriptions"));
      value->addColumn(_T("TRAN_COUNT"), DCI_DT_INT, _T("Transactions"));
      value->addColumn(_T("WSCLIENT"), DCI_DT_STRING, _T("Workstation Client"));
      value->addColumn(_T("WSCID"), DCI_DT_STRING, _T("Workstation ID"));
      value->addColumn(_T("NETADDR"), DCI_DT_STRING, _T("Network Address"));
      value->addColumn(_T("ENCBITS"), DCI_DT_INT, _T("Encryption Bits"));

      StructArray<KeyValuePair> *clients = s_clients->toArray();
      for(int i = 0; i < clients->size(); i++)
      {
         value->addRow();
         TuxedoClient *c = (TuxedoClient *)clients->get(i)->value;
         value->set(0, c->m_id);
         value->set(1, c->m_name);
         value->set(2, c->m_lmid);
         value->set(3, c->m_state);
         value->set(4, c->m_serverGroup);
         value->set(5, c->m_userName);
         value->set(6, (INT32)c->m_idleTime);
         value->set(7, (INT32)c->m_pid);
         value->set(8, (INT32)c->m_activeRequests);
         value->set(9, (INT32)c->m_activeConv);
         value->set(10, (INT32)c->m_convCount);
         value->set(11, (INT32)c->m_dequeueCount);
         value->set(12, (INT32)c->m_enqueueCount);
         value->set(13, (INT32)c->m_postCount);
         value->set(14, (INT32)c->m_requestCount);
         value->set(15, (INT32)c->m_subscribeCount);
         value->set(16, (INT32)c->m_tranCount);
         value->set(17, c->m_wsc);
         value->set(18, c->m_wsClientId);
         value->set(19, c->m_netAddr);
         value->set(20, c->m_encBits);
      }
      delete clients;
   }
   else
   {
      rc = SYSINFO_RC_ERROR;
   }
   MutexUnlock(s_lock);
   return rc;
}

/**
 * Handler for Tuxedo.Client.* parameters
 */
LONG H_ClientInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR id[80];
   if (!AgentGetParameterArg(param, 1, id, 80))
      return SYSINFO_RC_UNSUPPORTED;

   LONG rc = SYSINFO_RC_SUCCESS;

   MutexLock(s_lock);
   if (time(NULL) - s_lastQuery > 5)
   {
      QueryClients();
      s_lastQuery = time(NULL);
   }

   if (s_clients != NULL)
   {
      TuxedoClient *c = s_clients->get(id);
      if (c != NULL)
      {
         switch(*arg)
         {
            case 'A':
               ret_int(value, (INT32)c->m_activeRequests);
               break;
            case 'a':
               ret_int(value, (INT32)c->m_activeConv);
               break;
            case 'M':
               ret_mbstring(value, c->m_lmid);
               break;
            case 'N':
               ret_mbstring(value, c->m_name);
               break;
            case 'S':
               ret_mbstring(value, c->m_state);
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
