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
** $module: epp.cpp
**
**/

#include "nms_core.h"


//
// Event policy rule constructor
//

EPRule::EPRule(DWORD dwId, char *szComment, DWORD dwFlags)
{
   m_dwId = dwId;
   m_dwFlags = dwFlags;
   m_dwNumSources = 0;
   m_pSourceList = NULL;
   m_dwNumEvents = 0;
   m_pdwEventList = NULL;
   m_dwNumActions = 0;
   m_pdwActionList = NULL;
   m_pszComment = strdup(szComment);
}


//
// Event policy rule destructor
//

EPRule::~EPRule()
{
   safe_free(m_pSourceList);
   safe_free(m_pdwEventList);
   safe_free(m_pdwActionList);
   safe_free(m_pszComment);
}


//
// Load rule from database
//

BOOL EPRule::LoadFromDB(void)
{
   DB_RESULT hResult;
   char szQuery[256];
   BOOL bSuccess = TRUE;
   DWORD i;
   
   // Load rule's sources
   sprintf(szQuery, "SELECT source_type,object_id FROM PolicySourceList WHERE rule_id=%ld", m_dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      m_dwNumSources = DBGetNumRows(hResult);
      m_pSourceList = (EVENT_SOURCE *)malloc(sizeof(EVENT_SOURCE) * m_dwNumSources);
      for(i = 0; i < m_dwNumSources; i++)
      {
         m_pSourceList[i].iType = DBGetFieldLong(hResult, i, 0);
         m_pSourceList[i].dwObjectId = DBGetFieldULong(hResult, i, 1);
      }
      DBFreeResult(hResult);
   }
   else
   {
      bSuccess = FALSE;
   }

   // Load rule's events
   sprintf(szQuery, "SELECT event_id FROM PolicyEventList WHERE rule_id=%ld", m_dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      m_dwNumEvents = DBGetNumRows(hResult);
      m_pdwEventList = (DWORD *)malloc(sizeof(DWORD) * m_dwNumEvents);
      for(i = 0; i < m_dwNumEvents; i++)
         m_pdwEventList[i] = DBGetFieldULong(hResult, i, 0);
      DBFreeResult(hResult);
   }
   else
   {
      bSuccess = FALSE;
   }

   // Load rule's sources
   sprintf(szQuery, "SELECT action_id FROM PolicyActionList WHERE rule_id=%ld", m_dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      m_dwNumActions = DBGetNumRows(hResult);
      m_pdwActionList = (DWORD *)malloc(sizeof(DWORD) * m_dwNumActions);
      for(i = 0; i < m_dwNumActions; i++)
         m_pdwActionList[i] = DBGetFieldULong(hResult, i, 0);
      DBFreeResult(hResult);
   }
   else
   {
      bSuccess = FALSE;
   }

   return bSuccess;
}


//
// Event processing policy constructor
//

EventPolicy::EventPolicy()
{
   m_dwNumRules = 0;
   m_ppRuleList = NULL;
}


//
// Event processing policy destructor
//

EventPolicy::~EventPolicy()
{
   DWORD i;

   for(i = 0; i < m_dwNumRules; i++)
      safe_free(m_ppRuleList[i]);
   safe_free(m_ppRuleList);
}


//
// Load event processing policy from database
//

BOOL EventPolicy::LoadFromDB(void)
{
   DB_RESULT hResult;
   BOOL bSuccess = FALSE;

   hResult = DBSelect(g_hCoreDB, "SELECT id,flags,comments FROM EventPolicy ORDER BY id");
   if (hResult != NULL)
   {
      DWORD i;

      bSuccess = TRUE;
      m_dwNumRules = DBGetNumRows(hResult);
      m_ppRuleList = (EPRule **)malloc(sizeof(EPRule *) * m_dwNumRules);
      for(i = 0; i < m_dwNumRules; i++)
      {
         m_ppRuleList[i] = new EPRule(DBGetFieldULong(hResult, i, 0), 
                                      DBGetField(hResult, i, 2),
                                      DBGetFieldULong(hResult, i, 1));
         bSuccess = bSuccess && m_ppRuleList[i]->LoadFromDB();
      }
      DBFreeResult(hResult);
   }

   return bSuccess;
}
