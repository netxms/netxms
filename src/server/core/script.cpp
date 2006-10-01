/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006 Victor Kirhenshtein
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
** $module: script.cpp
**
**/

#include "nxcore.h"


//
// Global data
//

NXSL_Library *g_pScriptLibrary = NULL;


//
// Read object's attribute
//

NXSL_Value *NXSL_NetXMSObjectClass::GetAttr(NXSL_Object *pObject, char *pszAttr)
{
   NetObj *pSysObj;
   NXSL_Value *pValue = NULL;
   TCHAR szBuffer[256];

   pSysObj = (NetObj *)pObject->Data();
   if (pSysObj != NULL)
   {
      if (!strcmp(pszAttr, "id"))
      {
         pValue = new NXSL_Value(pSysObj->Id());
      }
      else if (!strcmp(pszAttr, "name"))
      {
         pValue = new NXSL_Value((char *)pSysObj->Name());
      }
      else if (!strcmp(pszAttr, "ipAddr"))
      {
         pValue = new NXSL_Value(IpToStr(pSysObj->IpAddr(), szBuffer));
      }
   }
   return pValue;
}


//
// Set object's attribute
//

BOOL NXSL_NetXMSObjectClass::SetAttr(NXSL_Object *pObject, char *pszAttr, NXSL_Value *pValue)
{
   return FALSE;
}


//
// Load scripts from database
//

void LoadScripts(void)
{
   DB_RESULT hResult;
   NXSL_Program *pScript;
   TCHAR *pszCode, szError[1024], szBuffer[MAX_DB_STRING];
   int i, nRows;

   g_pScriptLibrary = new NXSL_Library;
   hResult = DBSelect(g_hCoreDB, _T("SELECT script_id,script_name,script_code FROM script_library"));
   if (hResult != NULL)
   {
      nRows = DBGetNumRows(hResult);
      for(i = 0; i < nRows; i++)
      {
         pszCode = DBGetField(hResult, i, 2, NULL, 0);
         DecodeSQLString(pszCode);
         pScript = (NXSL_Program *)NXSLCompile(pszCode, szError, 1024);
         free(pszCode);
         if (pScript != NULL)
         {
            g_pScriptLibrary->AddScript(DBGetFieldULong(hResult, i, 0),
                                        DBGetField(hResult, i, 1, szBuffer, MAX_DB_STRING), pScript);
         }
         else
         {
            WriteLog(MSG_SCRIPT_COMPILATION_ERROR, EVENTLOG_WARNING_TYPE, _T("dss"),
                     DBGetFieldULong(hResult, i, 0),
                     DBGetField(hResult, i, 1, szBuffer, MAX_DB_STRING), szError);
         }
      }
      DBFreeResult(hResult);
   }
}


//
// Reload script from database
//

void ReloadScript(DWORD dwScriptId)
{
   TCHAR *pszCode, szQuery[256], szError[1024], szBuffer[MAX_DB_STRING];
   DB_RESULT hResult;
   NXSL_Program *pScript;

   g_pScriptLibrary->Lock();
   g_pScriptLibrary->DeleteScript(dwScriptId);
   
   _stprintf(szQuery, _T("SELECT script_name,script_code FROM script_library WHERE script_id=%d"), dwScriptId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         pszCode = DBGetField(hResult, 0, 1, NULL, 0);
         DecodeSQLString(pszCode);
         pScript = (NXSL_Program *)NXSLCompile(pszCode, szError, 1024);
         free(pszCode);
         if (pScript != NULL)
         {
            g_pScriptLibrary->AddScript(dwScriptId, DBGetField(hResult, 0, 0, szBuffer, MAX_DB_STRING), pScript);
         }
         else
         {
            WriteLog(MSG_SCRIPT_COMPILATION_ERROR, EVENTLOG_WARNING_TYPE, _T("dss"),
                     dwScriptId, DBGetField(hResult, 0, 0, szBuffer, MAX_DB_STRING), szError);
         }
      }
      DBFreeResult(hResult);
   }
   g_pScriptLibrary->Unlock();
}


//
// Check if script ID is valid
//

BOOL IsValidScriptId(DWORD dwId)
{
   TCHAR szQuery[256];
   DB_RESULT hResult;
   BOOL bRet = FALSE;

   _sntprintf(szQuery, 256, _T("SELECT script_id FROM script_library WHERE script_id=%d"), dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         bRet = TRUE;
      DBFreeResult(hResult);
   }
   return bRet;
}
