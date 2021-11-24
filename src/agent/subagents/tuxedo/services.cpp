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
** File: services.cpp
**
**/

#include "tuxedo_subagent.h"

/**
 * Tuxedo service information
 */
class TuxedoService
{
public:
   TCHAR m_name[32];
   char m_state[16];
   char m_routingName[16];
   long m_load;
   long m_priority;

   TuxedoService(FBFR32 *fb, FLDOCC32 index);
};

/**
 * Create service object from FB
 */
TuxedoService::TuxedoService(FBFR32 *fb, FLDOCC32 index)
{
   m_name[0] = 0;
   m_state[0] = 0;
   m_routingName[0] = 0;
   m_load = 0;
   m_priority = 0;

#ifdef UNICODE
   char name[32] = "";
   CFgetString(fb, TA_SERVICENAME, index, name, sizeof(name));
   mb_to_wchar(name, -1, m_name, 32);
#else
   CFgetString(fb, TA_SERVICENAME, index, m_name, sizeof(m_name));
#endif
   CFgetString(fb, TA_STATE, index, m_state, sizeof(m_state));
   CFgetString(fb, TA_ROUTINGNAME, index, m_routingName, sizeof(m_routingName));
   CFget32(fb, TA_LOAD, index, (char *)&m_load, NULL, FLD_LONG);
   CFget32(fb, TA_PRIO, index, (char *)&m_priority, NULL, FLD_LONG);
}

/**
 * Service list
 */
static Mutex s_lock;
static StringObjectMap<TuxedoService> *s_services = NULL;

/**
 * Reset services cache
 */
void TuxedoResetServices()
{
   s_lock.lock();
   delete_and_null(s_services);
   s_lock.unlock();
}

/**
 * Query services
 */
void TuxedoQueryServices()
{
   StringObjectMap<TuxedoService> *services = new StringObjectMap<TuxedoService>(Ownership::True);

   FBFR32 *fb = (FBFR32 *)tpalloc((char *)"FML32", NULL, 4096);
   CFchg32(fb, TA_OPERATION, 0, (char *)"GET", 0, FLD_STRING);
   CFchg32(fb, TA_CLASS, 0, (char *)"T_SERVICE", 0, FLD_STRING);

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
            TuxedoService *s = new TuxedoService(rsp, (FLDOCC32)i);
            services->set(s->m_name, s);
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
         delete_and_null(services);
      }
   }

   tpfree((char *)rsp);
   tpfree((char *)fb);

   s_lock.lock();
   delete s_services;
   s_services = services;
   s_lock.unlock();
}

/**
 * Handler for Tuxedo.Services list
 */
LONG H_ServicesList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   LONG rc = SYSINFO_RC_SUCCESS;

   s_lock.lock();
   if (s_services != NULL)
   {
      StructArray<KeyValuePair<TuxedoService>> *services = s_services->toArray();
      for(int i = 0; i < services->size(); i++)
      {
         value->add(services->get(i)->value->m_name);
      }
      delete services;
   }
   else
   {
      rc = SYSINFO_RC_ERROR;
   }
   s_lock.unlock();
   return rc;
}

/**
 * Handler for Tuxedo.Services table
 */
LONG H_ServicesTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   LONG rc = SYSINFO_RC_SUCCESS;

   s_lock.lock();
   if (s_services != NULL)
   {
      value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"), true);
      value->addColumn(_T("STATE"), DCI_DT_STRING, _T("State"));
      value->addColumn(_T("RT_NAME"), DCI_DT_STRING, _T("Routing Name"));
      value->addColumn(_T("LOAD"), DCI_DT_INT, _T("Load"));
      value->addColumn(_T("PRIO"), DCI_DT_INT, _T("Priority"));

      StructArray<KeyValuePair<TuxedoService>> *services = s_services->toArray();
      for(int i = 0; i < services->size(); i++)
      {
         value->addRow();
         const TuxedoService *s = services->get(i)->value;
         value->set(0, s->m_name);
         value->set(1, s->m_state);
         value->set(2, s->m_routingName);
         value->set(3, (INT32)s->m_load);
         value->set(4, (INT32)s->m_priority);
      }
      delete services;
   }
   else
   {
      rc = SYSINFO_RC_ERROR;
   }
   s_lock.unlock();
   return rc;
}

/**
 * Handler for Tuxedo.Service.* parameters
 */
LONG H_ServiceInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR serviceName[128];
   if (!AgentGetParameterArg(param, 1, serviceName, 128))
      return SYSINFO_RC_UNSUPPORTED;

   LONG rc = SYSINFO_RC_SUCCESS;

   s_lock.lock();
   if (s_services != NULL)
   {
      TuxedoService *s = s_services->get(serviceName);
      if (s != NULL)
      {
         switch(*arg)
         {
            case 'L':
               ret_int(value, (INT32)s->m_load);
               break;
            case 'P':
               ret_int(value, (INT32)s->m_priority);
               break;
            case 'R':
               ret_mbstring(value, s->m_routingName);
               break;
            case 'S':
               ret_mbstring(value, s->m_state);
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
