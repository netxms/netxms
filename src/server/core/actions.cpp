/* 
** NetXMS - Network Management System
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

#include "nms_core.h"


//
// Static data
//

static DWORD m_dwNumActions = 0;
static NMS_ACTION *m_pActionList = NULL;


//
// Destroy action list
//

void DestroyActionList(void)
{
   DWORD i;

   if (m_pActionList != NULL)
   {
      for(i = 0; i < m_dwNumActions; i++)
         if (m_pActionList[i].pszData != NULL)
            free(m_pActionList[i].pszData);
      free(m_pActionList);
      m_pActionList = NULL;
      m_dwNumActions = 0;
   }
}


//
// Load actions list from database
//

BOOL LoadActions(void)
{
   DB_RESULT hResult;
   BOOL bResult = FALSE;
   DWORD i;
   char *pStr;

   hResult = DBSelect(g_hCoreDB, "SELECT action_id,action_type,email_addr,email_subject,action_data FROM actions ORDER BY action_id");
   if (hResult != NULL)
   {
      DestroyActionList();
      m_dwNumActions = (DWORD)DBGetNumRows(hResult);
      m_pActionList = (NMS_ACTION *)malloc(sizeof(NMS_ACTION) * m_dwNumActions);
      for(i = 0; i < m_dwNumActions; i++)
      {
         m_pActionList[i].dwId = DBGetFieldULong(hResult, i, 0);
         m_pActionList[i].iType = DBGetFieldLong(hResult, i, 1);
         pStr = DBGetField(hResult, i, 2);
         strcpy(m_pActionList[i].szEmailAddr, CHECK_NULL(pStr));
         pStr = DBGetField(hResult, i, 3);
         strcpy(m_pActionList[i].szEmailSubject, CHECK_NULL(pStr));
         pStr = DBGetField(hResult, i, 4);
         pStr = CHECK_NULL(pStr);
         m_pActionList[i].pszData = (char *)malloc(strlen(pStr) + 1);
         strcpy(m_pActionList[i].pszData, pStr);
      }
      DBFreeResult(hResult);
      bResult = TRUE;
   }
   else
   {
      WriteLog(MSG_ACTIONS_LOAD_FAILED, EVENTLOG_ERROR_TYPE, NULL);
   }
   return bResult;
}


//
// Compare action's id for bsearch()
//

static int CompareId(const void *key, const void *elem)
{
   return (DWORD)key < ((NMS_ACTION *)elem)->dwId ? -1 : 
            ((DWORD)key > ((NMS_ACTION *)elem)->dwId ? 1 : 0);
}


//
// Execute action on specific event
//

BOOL ExecuteAction(DWORD dwActionId, Event *pEvent)
{
   NMS_ACTION *pAction;
   BOOL bSuccess = FALSE;

   pAction = (NMS_ACTION *)bsearch((void *)dwActionId, m_pActionList, m_dwNumActions, sizeof(NMS_ACTION), CompareId);
   if (pAction != NULL)
   {
      char *pszExpandedData;

      pszExpandedData = pEvent->ExpandText(pAction->pszData);
      switch(pAction->iType)
      {
         case ACTION_EXEC:
            DbgPrintf(AF_DEBUG_ACTIONS, "*actions* Executing command \"%s\"\n", pszExpandedData);
            ExecCommand(pszExpandedData);
            break;
         default:
            break;
      }
      free(pszExpandedData);
   }
   return bSuccess;
}
