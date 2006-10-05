/* 
** Project X - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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
** $module: config.cpp
**
**/

#include "nxcore.h"


//
// Static data
//

static TCHAR m_szCodePage[256];


//
// Load and parse configuration file
// Returns TRUE on success and FALSE on failure
//

static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
   { "CodePage", CT_STRING, 0, 0, 256, 0, m_szCodePage },
   { "DBDriver", CT_STRING, 0, 0, MAX_PATH, 0, g_szDbDriver },
   { "DBDrvParams", CT_STRING, 0, 0, MAX_PATH, 0, g_szDbDrvParams },
   { "DBLogin", CT_STRING, 0, 0, MAX_DB_LOGIN, 0, g_szDbLogin },
   { "DBName", CT_STRING, 0, 0, MAX_DB_NAME, 0, g_szDbName },
   { "DBPassword", CT_STRING, 0, 0, MAX_DB_PASSWORD, 0, g_szDbPassword },
   { "DBServer", CT_STRING, 0, 0, MAX_PATH, 0, g_szDbServer },
   { "LogFailedSQLQueries", CT_BOOLEAN, 0, 0, AF_LOG_SQL_ERRORS, 0, &g_dwFlags },
   { "LogFile", CT_STRING, 0, 0, MAX_PATH, 0, g_szLogFile },
   { "", CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};

BOOL NXCORE_EXPORTABLE LoadConfig(void)
{
   BOOL bSuccess = FALSE;

   if (IsStandalone())
      printf("Using configuration file \"%s\"\n", g_szConfigFile);

   if (NxLoadConfig(g_szConfigFile, "", m_cfgTemplate, IsStandalone()) == NXCFG_ERR_OK)
   {
      if ((!stricmp(g_szLogFile,"{EventLog}")) ||
          (!stricmp(g_szLogFile,"{syslog}")))
      {
         g_dwFlags |= AF_USE_EVENT_LOG;
      }
      else
      {
         g_dwFlags &= ~AF_USE_EVENT_LOG;
      }
#ifndef _WIN32
		SetDefaultCodepage(m_szCodePage);
#endif
      bSuccess = TRUE;
   }
   return bSuccess;
}


//
// Read string value from configuration table
//

BOOL NXCORE_EXPORTABLE ConfigReadStr(TCHAR *szVar, TCHAR *szBuffer, int iBufSize, const TCHAR *szDefault)
{
   DB_RESULT hResult;
   TCHAR szQuery[256];
   BOOL bSuccess = FALSE;

   nx_strncpy(szBuffer, szDefault, iBufSize);
   if (_tcslen(szVar) > 127)
      return FALSE;

   _sntprintf(szQuery, 256, _T("SELECT var_value FROM config WHERE var_name='%s'"), szVar);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult == 0)
      return FALSE;

   if (DBGetNumRows(hResult) > 0)
   {
      DBGetField(hResult, 0, 0, szBuffer, iBufSize);
      DecodeSQLString(szBuffer);
      bSuccess = TRUE;
   }

   DBFreeResult(hResult);
   return bSuccess;
}


//
// Read integer value from configuration table
//

int NXCORE_EXPORTABLE ConfigReadInt(TCHAR *szVar, int iDefault)
{
   TCHAR szBuffer[64];

   if (ConfigReadStr(szVar, szBuffer, 64, _T("")))
      return _tcstol(szBuffer, NULL, 0);
   else
      return iDefault;
}


//
// Read unsigned long value from configuration table
//

DWORD NXCORE_EXPORTABLE ConfigReadULong(TCHAR *szVar, DWORD dwDefault)
{
   TCHAR szBuffer[64];

   if (ConfigReadStr(szVar, szBuffer, 64, _T("")))
      return _tcstoul(szBuffer, NULL, 0);
   else
      return dwDefault;
}


//
// Read byte array (in hex form) from configuration table into integer array
//

BOOL NXCORE_EXPORTABLE ConfigReadByteArray(TCHAR *pszVar, int *pnArray, int nSize, int nDefault)
{
   TCHAR szBuffer[256];
   char pbBytes[128];
   BOOL bResult;
   int i, nLen;

   if (ConfigReadStr(pszVar, szBuffer, 256, _T("")))
   {
      StrToBin(szBuffer, (BYTE *)pbBytes, 128);
      nLen = (int)strlen(szBuffer) / 2;
      for(i = 0; (i < nSize) && (i < nLen); i++)
         pnArray[i] = pbBytes[i];
      for(; i < nSize; i++)
         pnArray[i] = nDefault;
      bResult = TRUE;
   }
   else
   {
      for(i = 0; i < nSize; i++)
         pnArray[i] = nDefault;
      bResult = FALSE;
   }
   return bResult;
}


//
// Write string value to configuration table
//

BOOL NXCORE_EXPORTABLE ConfigWriteStr(TCHAR *szVar, TCHAR *szValue, BOOL bCreate)
{
   DB_RESULT hResult;
   TCHAR *pszEscValue, szQuery[1024];
   BOOL bVarExist = FALSE;

   if (strlen(szVar) > 127)
      return FALSE;

   // Check for variable existence
   _sntprintf(szQuery, 1024, _T("SELECT var_value FROM config WHERE var_name='%s'"), szVar);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         bVarExist = TRUE;
      DBFreeResult(hResult);
   }

   // Don't create non-existing variable if creation flag not set
   if (!bCreate && !bVarExist)
      return FALSE;

   // Create or update variable value
   pszEscValue = EncodeSQLString(szValue);
   if (bVarExist)
      _sntprintf(szQuery, 1024, _T("UPDATE config SET var_value='%s' WHERE var_name='%s'"),
              pszEscValue, szVar);
   else
      _sntprintf(szQuery, 1024, _T("INSERT INTO config (var_name,var_value,is_visible,")
                                _T("need_server_restart) VALUES ('%s','%s',1,1)"),
                 szVar, pszEscValue);
   free(pszEscValue);
   return DBQuery(g_hCoreDB, szQuery);
}


//
// Write integer value to configuration table
//

BOOL NXCORE_EXPORTABLE ConfigWriteInt(TCHAR *szVar, int iValue, BOOL bCreate)
{
   TCHAR szBuffer[64];

   _stprintf(szBuffer, _T("%d"), iValue);
   return ConfigWriteStr(szVar, szBuffer, bCreate);
}


//
// Write unsigned long value to configuration table
//

BOOL NXCORE_EXPORTABLE ConfigWriteULong(TCHAR *szVar, DWORD dwValue, BOOL bCreate)
{
   TCHAR szBuffer[64];

   _stprintf(szBuffer, _T("%u"), dwValue);
   return ConfigWriteStr(szVar, szBuffer, bCreate);
}


//
// Write integer array to configuration table
//

BOOL NXCORE_EXPORTABLE ConfigWriteByteArray(TCHAR *pszVar, int *pnArray, int nSize, BOOL bCreate)
{
   TCHAR szBuffer[256];
   int i, j;

   for(i = 0, j = 0; (i < nSize) && (i < 127); i++, j += 2)
      _stprintf(&szBuffer[j], _T("%02X"), (char)((pnArray[i] > 127) ? 127 : ((pnArray[i] < -127) ? -127 : pnArray[i])));
   return ConfigWriteStr(pszVar, szBuffer, bCreate);
}
