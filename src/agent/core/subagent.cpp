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
      BOOL (* SubAgentInit)(NETXMS_SUBAGENT_INFO **);

      SubAgentInit = (BOOL (*)(NETXMS_SUBAGENT_INFO **))DLGetSymbolAddr(hModule, "NxSubAgentInit", szErrorText);
      if (SubAgentInit != NULL)
      {
         NETXMS_SUBAGENT_INFO *pInfo;

         if (SubAgentInit(&pInfo))
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
