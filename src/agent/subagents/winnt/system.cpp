/* 
** Windows NT/2000/XP/2003 NetXMS subagent
** Copyright (C) 2003, 2004, 2005 Victor Kirhenshtein
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
** $module: system.cpp
** WinNT+ specific system information parameters
**
**/

#include "winnt_subagent.h"
#include <wtsapi32.h>


//
// Handler for System.ServiceState parameter
//

LONG H_ServiceState(char *cmd, char *arg, char *value)
{
   SC_HANDLE hManager, hService;
   TCHAR szServiceName[MAX_PATH];
   LONG iResult = SYSINFO_RC_SUCCESS;

   if (!NxGetParameterArg(cmd, 1, szServiceName, MAX_PATH))
      return SYSINFO_RC_UNSUPPORTED;

   hManager = OpenSCManager(NULL, NULL, GENERIC_READ);
   if (hManager == NULL)
   {
      return SYSINFO_RC_ERROR;
   }

   hService = OpenService(hManager, szServiceName, SERVICE_QUERY_STATUS);
   if (hService == NULL)
   {
      iResult = SYSINFO_RC_UNSUPPORTED;
   }
   else
   {
      SERVICE_STATUS status;

      if (QueryServiceStatus(hService, &status))
      {
         int i;
         static DWORD dwStates[7]={ SERVICE_RUNNING, SERVICE_PAUSED, SERVICE_START_PENDING,
                                    SERVICE_PAUSE_PENDING, SERVICE_CONTINUE_PENDING,
                                    SERVICE_STOP_PENDING, SERVICE_STOPPED };

         for(i = 0; i < 7; i++)
            if (status.dwCurrentState == dwStates[i])
               break;
         ret_uint(value, i);
      }
      else
      {
         ret_uint(value, 255);    // Unable to retrieve information
      }
      CloseServiceHandle(hService);
   }

   CloseServiceHandle(hManager);
   return iResult;
}


//
// Handler for System.ThreadCount
//

LONG H_ThreadCount(char *cmd, char *arg, char *value)
{
   PERFORMANCE_INFORMATION pi;

   if (imp_GetPerformanceInfo != NULL)
   {
      pi.cb = sizeof(PERFORMANCE_INFORMATION);
      if (!imp_GetPerformanceInfo(&pi, sizeof(PERFORMANCE_INFORMATION)))
         return SYSINFO_RC_ERROR;
      ret_uint(value, pi.ThreadCount);
      return SYSINFO_RC_SUCCESS;
   }
   else
   {
      return SYSINFO_RC_UNSUPPORTED;
   }
}


//
// Handler for System.ConnectedUsers parameter
//

LONG H_ConnectedUsers(char *pszCmd, char *pArg, char *pValue)
{
   LONG nRet;
   WTS_SESSION_INFO *pSessionList;
   DWORD i, dwNumSessions, dwCount;

   if (WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1,
                            &pSessionList, &dwNumSessions))
   {
      for(i = 0, dwCount = 0; i < dwNumSessions; i++)
         if ((pSessionList[i].State == WTSActive) ||
             (pSessionList[i].State == WTSConnected))
            dwCount++;
      WTSFreeMemory(pSessionList);
      ret_uint(pValue, dwCount);
      nRet = SYSINFO_RC_SUCCESS;
   }
   else
   {
      nRet = SYSINFO_RC_ERROR;
   }
   return nRet;
}
