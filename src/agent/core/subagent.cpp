/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: subagent.cpp
**
**/

#include "nxagentd.h"

#ifdef _NETWARE
#include <fsio.h>
#endif


//
// Static data
//

static DWORD m_dwNumSubAgents = 0;
static SUBAGENT *m_pSubAgentList = NULL;


//
// Load subagent
//

BOOL LoadSubAgent(char *szModuleName)
{
   HMODULE hModule;
   BOOL bSuccess = FALSE;
   char szErrorText[256];

   hModule = DLOpen(szModuleName, szErrorText);
   if (hModule != NULL)
   {
      BOOL (* SubAgentInit)(NETXMS_SUBAGENT_INFO **, TCHAR *pszConfigFile);

      // Under NetWare, we have slightly different subagent
      // initialization procedure. Because normally two NLMs
      // cannot export symbols with identical names, we cannot 
      // simply export NxSubAgentInit in each subagent. Instead,
      // agent expect to find symbol called NxSubAgentInit_<module_file_name>
      // in every subagent. Note that module file name should
      // be in capital letters.
#ifdef _NETWARE
      char *pExt, szFileName[MAX_PATH], szEntryPoint[MAX_PATH];
      int iElem, iFlags;

      deconstruct(szModuleName, NULL, NULL, NULL, szFileName, NULL, &iElem, &iFlags);
      pExt = strrchr(szFileName, '.');
      if (pExt != NULL)
         *pExt = 0;
      strupr(szFileName);
      sprintf(szEntryPoint, "NxSubAgentInit_%s", szFileName);
      SubAgentInit = (BOOL (*)(NETXMS_SUBAGENT_INFO **, TCHAR *))DLGetSymbolAddr(hModule, szEntryPoint, szErrorText);
#else
      SubAgentInit = (BOOL (*)(NETXMS_SUBAGENT_INFO **, TCHAR *))DLGetSymbolAddr(hModule, "NxSubAgentInit", szErrorText);
#endif

      if (SubAgentInit != NULL)
      {
         NETXMS_SUBAGENT_INFO *pInfo;

         if (SubAgentInit(&pInfo, g_szConfigFile))
         {
            DWORD i;

            // Add subagent to subagent's list
            m_pSubAgentList = (SUBAGENT *)realloc(m_pSubAgentList, 
                                                  sizeof(SUBAGENT) * (m_dwNumSubAgents + 1));
            m_pSubAgentList[m_dwNumSubAgents].hModule = hModule;
            strncpy(m_pSubAgentList[m_dwNumSubAgents].szName, szModuleName, MAX_PATH - 1);
            m_pSubAgentList[m_dwNumSubAgents].pInfo = pInfo;
            m_dwNumSubAgents++;

            // Add parameters provided by this subagent to common list
            for(i = 0; i < pInfo->dwNumParameters; i++)
               AddParameter(pInfo->pParamList[i].szName, 
                            pInfo->pParamList[i].fpHandler,
                            pInfo->pParamList[i].pArg);

            // Add enums provided by this subagent to common list
            for(i = 0; i < pInfo->dwNumEnums; i++)
               AddEnum(pInfo->pEnumList[i].szName, 
                       pInfo->pEnumList[i].fpHandler,
                       pInfo->pEnumList[i].pArg);

            WriteLog(MSG_SUBAGENT_LOADED, EVENTLOG_INFORMATION_TYPE,
                     "s", szModuleName);
            bSuccess = TRUE;
         }
         else
         {
            WriteLog(MSG_SUBAGENT_INIT_FAILED, EVENTLOG_ERROR_TYPE,
                     "s", szModuleName);
            DLClose(hModule);
         }
      }
      else
      {
         WriteLog(MSG_NO_SUBAGENT_ENTRY_POINT, EVENTLOG_ERROR_TYPE,
                  "s", szModuleName);
         DLClose(hModule);
      }
   }
   else
   {
      WriteLog(MSG_SUBAGENT_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "ss", szModuleName, szErrorText);
   }

   return bSuccess;
}


//
// Unload all subagents.
// This function should be called on shutdown, so we don't care
// about deregistering parameters and so on.
//

void UnloadAllSubAgents(void)
{
   DWORD i;

   for(i = 0; i < m_dwNumSubAgents; i++)
   {
      if (m_pSubAgentList[i].pInfo->pUnloadHandler != NULL)
         m_pSubAgentList[i].pInfo->pUnloadHandler();
      DLClose(m_pSubAgentList[i].hModule);
   }
}


//
// Enumerate loaded subagents
//

LONG H_SubAgentList(char *cmd, char *arg, NETXMS_VALUES_LIST *value)
{
   DWORD i;
   char szBuffer[MAX_PATH + 32];

   for(i = 0; i < m_dwNumSubAgents; i++)
   {
      _sntprintf(szBuffer, MAX_PATH + 32, "%s 0x%08X %s", m_pSubAgentList[i].pInfo->szName,
               m_pSubAgentList[i].hModule, m_pSubAgentList[i].szName);
      NxAddResultString(value, szBuffer);
   }
   return SYSINFO_RC_SUCCESS;
}
