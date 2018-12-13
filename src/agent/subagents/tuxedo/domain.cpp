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
** File: domain.cpp
**
**/

#include "tuxedo_subagent.h"

/**
 * Domain info
 */
static Mutex s_lock;
static bool s_validData = false;
static char s_domainId[32] = "";
static char s_master[256] = "";
static char s_model[16] = "";
static char s_state[16] = "";
static long s_queues = 0;
static long s_routes = 0;
static long s_servers = 0;
static long s_services = 0;

/**
 * Reset domain cache
 */
void TuxedoResetDomain()
{
   s_lock.lock();
   s_validData = false;
   s_lock.unlock();
}

/**
 * Query domain information
 */
void TuxedoQueryDomain()
{
	FBFR32 *fb = (FBFR32 *)tpalloc((char *)"FML32", NULL, 4096);
	CFchg32(fb, TA_OPERATION, 0, (char *)"GET", 0, FLD_STRING);
	CFchg32(fb, TA_CLASS, 0, (char *)"T_DOMAIN", 0, FLD_STRING);

   long rsplen = 8192;
   FBFR32 *rsp = (FBFR32 *)tpalloc((char *)"FML32", NULL, rsplen);
   if (tpcall((char *)".TMIB", (char *)fb, 0, (char **)&rsp, &rsplen, 0) != -1)
   {
      s_lock.lock();
      CFgetString(rsp, TA_DOMAINID, 0, s_domainId, sizeof(s_domainId));
      CFgetString(rsp, TA_MASTER, 0, s_master, sizeof(s_master));
      CFgetString(rsp, TA_MODEL, 0, s_model, sizeof(s_model));
      CFgetString(rsp, TA_STATE, 0, s_state, sizeof(s_state));
      CFget32(rsp, TA_CURQUEUES, 0, (char *)&s_queues, NULL, FLD_LONG);
      CFget32(rsp, TA_CURDRT, 0, (char *)&s_routes, NULL, FLD_LONG);
      CFget32(rsp, TA_CURSERVERS, 0, (char *)&s_servers, NULL, FLD_LONG);
      CFget32(rsp, TA_CURSERVICES, 0, (char *)&s_services, NULL, FLD_LONG);
      s_validData = true;
      s_lock.unlock();
   }
   else
   {
      nxlog_debug_tag(TUXEDO_DEBUG_TAG, 3, _T("tpcall() call failed (%hs)"), tpstrerrordetail(tperrno, 0));

      s_lock.lock();
      s_validData = false;
      s_lock.unlock();
   }

   tpfree((char *)rsp);
   tpfree((char *)fb);
}

/**
 * Handler for Tuxedo.Domain.* parameters
 */
LONG H_DomainInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   LONG rc = SYSINFO_RC_SUCCESS;

   s_lock.lock();
   if (s_validData)
   {
      switch(*arg)
      {
         case 'I':
            ret_mbstring(value, s_domainId);
            break;
         case 'M':
            ret_mbstring(value, s_master);
            break;
         case 'm':
            ret_mbstring(value, s_model);
            break;
         case 'Q':
            ret_int(value, (INT32)s_queues);
            break;
         case 'R':
            ret_int(value, (INT32)s_routes);
            break;
         case 'S':
            ret_int(value, (INT32)s_servers);
            break;
         case 's':
            ret_int(value, (INT32)s_services);
            break;
         case 'T':
            ret_mbstring(value, s_state);
            break;
         default:
            rc = SYSINFO_RC_UNSUPPORTED;
            break;
      }
   }
   else
   {
      rc = SYSINFO_RC_ERROR;
   }
   s_lock.unlock();

   return rc;
}
