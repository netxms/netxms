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
** File: machines.cpp
**
**/

#include "tuxedo_subagent.h"

/**
 * Tuxedo queue information
 */
class TuxedoMachine
{
public:
   TCHAR m_id[32];
   char m_pmid[32];
   char m_tuxConfig[MAX_PATH];
   char m_tuxDir[MAX_PATH];
   char m_appDir[MAX_PATH];
   char m_envFile[MAX_PATH];
   char m_state[16];
   char m_type[16];
   char m_tlogDevice[MAX_PATH];
   char m_tlogName[32];
   char m_bridge[80];
   char m_role[16];
   char m_swrelease[80];
   long m_accessers;
   long m_clients;
   long m_wsClients;
   long m_conversations;
   long m_load;
   long m_workloadsProcessed;
   long m_workloadsInitiated;

   TuxedoMachine(FBFR32 *fb, FLDOCC32 index);
};

/**
 * Create queue object from FB
 */
TuxedoMachine::TuxedoMachine(FBFR32 *fb, FLDOCC32 index)
{
   m_id[0] = 0;
   m_pmid[0] = 0;
   m_tuxConfig[0] = 0;
   m_tuxDir[0] = 0;
   m_appDir[0] = 0;
   m_envFile[0] = 0;
   m_state[0] = 0;
   m_type[0] = 0;
   m_tlogDevice[0] = 0;
   m_tlogName[0] = 0;
   m_bridge[0] = 0;
   m_role[0] = 0;
   m_swrelease[0] = 0;
   m_accessers = 0;
   m_clients = 0;
   m_wsClients = 0;
   m_conversations = 0;
   m_load = 0;
   m_workloadsProcessed = 0;
   m_workloadsInitiated = 0;

#ifdef UNICODE
   char id[32] = "";
   CFgetString(fb, TA_LMID, index, id, sizeof(id));
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, id, -1, m_id, 32);
#else
   CFgetString(fb, TA_LMID, index, m_id, sizeof(m_id));
#endif
   CFgetString(fb, TA_PMID, index, m_pmid, sizeof(m_pmid));
   CFgetString(fb, TA_TUXCONFIG, index, m_tuxConfig, sizeof(m_tuxConfig));
   CFgetString(fb, TA_TUXDIR, index, m_tuxDir, sizeof(m_tuxDir));
   CFgetString(fb, TA_APPDIR, index, m_appDir, sizeof(m_appDir));
   CFgetString(fb, TA_ENVFILE, index, m_envFile, sizeof(m_envFile));
   CFgetString(fb, TA_STATE, index, m_state, sizeof(m_state));
   CFgetString(fb, TA_TYPE, index, m_type, sizeof(m_type));
   CFgetString(fb, TA_TLOGDEVICE, index, m_tlogDevice, sizeof(m_tlogDevice));
   CFgetString(fb, TA_TLOGNAME, index, m_tlogName, sizeof(m_tlogName));
   CFgetString(fb, TA_BRIDGE, index, m_bridge, sizeof(m_bridge));
   CFgetString(fb, TA_ROLE, index, m_role, sizeof(m_role));
   CFgetString(fb, TA_SWRELEASE, index, m_swrelease, sizeof(m_swrelease));
   CFget32(fb, TA_CURACCESSERS, index, (char *)&m_accessers, NULL, FLD_LONG);
   CFget32(fb, TA_CURCLIENTS, index, (char *)&m_clients, NULL, FLD_LONG);
   CFget32(fb, TA_CURWSCLIENTS, index, (char *)&m_wsClients, NULL, FLD_LONG);
   CFget32(fb, TA_CURCONV, index, (char *)&m_conversations, NULL, FLD_LONG);
   CFget32(fb, TA_CURRLOAD, index, (char *)&m_load, NULL, FLD_LONG);
   CFget32(fb, TA_WKCOMPLETED, index, (char *)&m_workloadsProcessed, NULL, FLD_LONG);
   CFget32(fb, TA_WKINITIATED, index, (char *)&m_workloadsInitiated, NULL, FLD_LONG);
}

/**
 * Queue list
 */
static MUTEX s_lock = MutexCreate();
static time_t s_lastQuery = 0;
static StringObjectMap<TuxedoMachine> *s_machines = NULL;

/**
 * Query machines
 */
static void QueryMachines()
{
   delete_and_null(s_machines);

   if (!TuxedoConnect())
      AgentWriteDebugLog(3, _T("Tuxedo: tpinit() call failed (%d)"), errno);

   FBFR32 *fb = (FBFR32 *)tpalloc((char *)"FML32", NULL, 4096);
   CFchg32(fb, TA_OPERATION, 0, (char *)"GET", 0, FLD_STRING);
   CFchg32(fb, TA_CLASS, 0, (char *)"T_MACHINE", 0, FLD_STRING);

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
         if (s_machines == NULL)
            s_machines = new StringObjectMap<TuxedoMachine>(true);

         long count = 0;
         CFget32(rsp, TA_OCCURS, 0, (char *)&count, NULL, FLD_LONG);
         for(int i = 0; i < (int)count; i++)
         {
            TuxedoMachine *m = new TuxedoMachine(rsp, (FLDOCC32)i);
            s_machines->set(m->m_id, m);
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
   TuxedoDisconnect();
}

/**
 * Handler for Tuxedo.Machines list
 */
LONG H_MachinesList(const TCHAR *param, const TCHAR *arg, StringList *value)
{
   LONG rc = SYSINFO_RC_SUCCESS;

   MutexLock(s_lock);
   if (time(NULL) - s_lastQuery > 5)
   {
      QueryMachines();
      s_lastQuery = time(NULL);
   }

   if (s_machines != NULL)
   {
      StructArray<KeyValuePair> *machines = s_machines->toArray();
      for(int i = 0; i < machines->size(); i++)
      {
         value->add(((TuxedoMachine *)machines->get(i)->value)->m_id);
      }
      delete machines;
   }
   else
   {
      rc = SYSINFO_RC_ERROR;
   }
   MutexUnlock(s_lock);
   return rc;
}

/**
 * Handler for Tuxedo.Machines table
 */
LONG H_MachinesTable(const TCHAR *param, const TCHAR *arg, Table *value)
{
   LONG rc = SYSINFO_RC_SUCCESS;

   MutexLock(s_lock);
   if (time(NULL) - s_lastQuery > 5)
   {
      QueryMachines();
      s_lastQuery = time(NULL);
   }

   if (s_machines != NULL)
   {
      value->addColumn(_T("ID"), DCI_DT_STRING, _T("ID"), true);
      value->addColumn(_T("PMID"), DCI_DT_STRING, _T("Physical ID"));
      value->addColumn(_T("TYPE"), DCI_DT_STRING, _T("Type"));
      value->addColumn(_T("STATE"), DCI_DT_STRING, _T("State"));
      value->addColumn(_T("ROLE"), DCI_DT_STRING, _T("Role"));
      value->addColumn(_T("BRIDGE"), DCI_DT_STRING, _T("Bridge"));
      value->addColumn(_T("SW_REL"), DCI_DT_STRING, _T("Software Release"));
      value->addColumn(_T("ACCESSERS"), DCI_DT_INT, _T("Accessers"));
      value->addColumn(_T("CLIENTS"), DCI_DT_INT, _T("Clients"));
      value->addColumn(_T("WS_CLIENTS"), DCI_DT_INT, _T("Workstation Clients"));
      value->addColumn(_T("CONVERSATIONS"), DCI_DT_INT, _T("Conversations"));
      value->addColumn(_T("LOAD"), DCI_DT_INT, _T("Load"));
      value->addColumn(_T("WK_PROCESSED"), DCI_DT_INT, _T("Workloads Processed"));
      value->addColumn(_T("WK_INITIATED"), DCI_DT_INT, _T("Workloads Initiated"));
      value->addColumn(_T("TUXCONFIG"), DCI_DT_STRING, _T("Config"));
      value->addColumn(_T("TUXDIR"), DCI_DT_STRING, _T("Tuxedo Directory"));
      value->addColumn(_T("APPDIR"), DCI_DT_STRING, _T("Application Directory"));
      value->addColumn(_T("ENV_FILE"), DCI_DT_STRING, _T("Environment File"));
      value->addColumn(_T("TLOG_DEVICE"), DCI_DT_STRING, _T("TLOG Device"));
      value->addColumn(_T("TLOG_NAME"), DCI_DT_STRING, _T("TLOG Name"));

      StructArray<KeyValuePair> *machines = s_machines->toArray();
      for(int i = 0; i < machines->size(); i++)
      {
         value->addRow();
         TuxedoMachine *m = (TuxedoMachine *)machines->get(i)->value;
         value->set(0, m->m_id);
         value->set(1, m->m_pmid);
         value->set(2, m->m_type);
         value->set(3, m->m_state);
         value->set(4, m->m_role);
         value->set(5, m->m_bridge);
         value->set(6, m->m_swrelease);
         value->set(7, (INT32)m->m_accessers);
         value->set(8, (INT32)m->m_clients);
         value->set(9, (INT32)m->m_wsClients);
         value->set(10, (INT32)m->m_conversations);
         value->set(11, (INT32)m->m_load);
         value->set(12, (INT32)m->m_workloadsProcessed);
         value->set(13, (INT32)m->m_workloadsInitiated);
         value->set(14, m->m_tuxConfig);
         value->set(15, m->m_tuxDir);
         value->set(16, m->m_appDir);
         value->set(17, m->m_envFile);
         value->set(18, m->m_tlogDevice);
         value->set(19, m->m_tlogName);
      }
      delete machines;
   }
   else
   {
      rc = SYSINFO_RC_ERROR;
   }
   MutexUnlock(s_lock);
   return rc;
}

/**
 * Handler for Tuxedo.Machine.* parameters
 */
LONG H_MachineInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value)
{
   TCHAR id[32];
   if (!AgentGetParameterArg(param, 1, id, 32))
      return SYSINFO_RC_UNSUPPORTED;

   LONG rc = SYSINFO_RC_SUCCESS;

   MutexLock(s_lock);
   if (time(NULL) - s_lastQuery > 5)
   {
      QueryMachines();
      s_lastQuery = time(NULL);
   }

   if (s_machines != NULL)
   {
      TuxedoMachine *m = s_machines->get(id);
      if (m != NULL)
      {
         switch(*arg)
         {
            case 'A':
               ret_int(value, (INT32)m->m_accessers);
               break;
            case 'B':
               ret_mbstring(value, m->m_bridge);
               break;
            case 'C':
               ret_int(value, (INT32)m->m_clients);
               break;
            case 'c':
               ret_int(value, (INT32)m->m_wsClients);
               break;
            case 'L':
               ret_int(value, (INT32)m->m_load);
               break;
            case 'o':
               ret_int(value, (INT32)m->m_conversations);
               break;
            case 'P':
               ret_mbstring(value, m->m_pmid);
               break;
            case 'R':
               ret_mbstring(value, m->m_role);
               break;
            case 'S':
               ret_mbstring(value, m->m_state);
               break;
            case 's':
               ret_mbstring(value, m->m_swrelease);
               break;
            case 'T':
               ret_mbstring(value, m->m_type);
               break;
            case 'W':
               ret_int(value, (INT32)m->m_workloadsInitiated);
               break;
            case 'w':
               ret_int(value, (INT32)m->m_workloadsProcessed);
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
