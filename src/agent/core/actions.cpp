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
** $module: actions.cpp
**
**/

#include "nxagentd.h"


//
// Static data
//

static ACTION *m_pActionList = NULL;
static DWORD m_dwNumActions = 0;


//
// Add action
//

BOOL AddAction(char *pszName, int iType, char *pszCmdLine)
{
   DWORD i;

   // Check if action with given name already registered
   for(i = 0; i < m_dwNumActions; i++)
      if (!stricmp(m_pActionList[i].szName, pszName))
         return FALSE;

   // Create new entry in action list
   m_dwNumActions++;
   m_pActionList = (ACTION *)realloc(m_pActionList, sizeof(ACTION) * m_dwNumActions);
   strncpy(m_pActionList[i].szName, pszName, MAX_PARAM_NAME);
   m_pActionList[i].iType = iType;
   m_pActionList[i].pszCmdLine = strdup(pszCmdLine);
   return TRUE;
}


//
// Add action from config record
// Accepts string of format <action_name>:<command_line>
//

BOOL AddActionFromConfig(char *pszLine)
{
   char *pCmdLine;

   pCmdLine = strchr(pszLine, ':');
   if (pCmdLine == NULL)
      return FALSE;
   *pCmdLine = 0;
   pCmdLine++;
   StrStrip(pszLine);
   StrStrip(pCmdLine);
   return AddAction(pszLine, AGENT_ACTION_EXEC, pCmdLine);
}


//
// Execute action
//

DWORD ExecAction(char *pszAction, NETXMS_VALUES_LIST *pArgs)
{
   DWORD i, dwErrorCode = ERR_UNKNOWN_PARAMETER;

   for(i = 0; i < m_dwNumActions; i++)
      if (!stricmp(m_pActionList[i].szName, pszAction))
      {
         DebugPrintf("Executing action %s of type %d", pszAction, m_pActionList[i].iType);
         switch(m_pActionList[i].iType)
         {
            case AGENT_ACTION_EXEC:
               dwErrorCode = ExecuteCommand(m_pActionList[i].pszCmdLine, pArgs);
               break;
            default:
               dwErrorCode = ERR_NOT_IMPLEMENTED;
               break;
         }
         break;
      }
   return dwErrorCode;
}


//
// Enumerate available actions
//

LONG H_ActionList(char *cmd, char *arg, NETXMS_VALUES_LIST *value)
{
   DWORD i;
   char szBuffer[1024];

   for(i = 0; i < m_dwNumActions; i++)
   {
      snprintf(szBuffer, 1024, "%s %d \"%s\"", m_pActionList[i].szName, m_pActionList[i].iType,
               m_pActionList[i].pszCmdLine);
      NxAddResultString(value, szBuffer);
   }
   return SYSINFO_RC_SUCCESS;
}
