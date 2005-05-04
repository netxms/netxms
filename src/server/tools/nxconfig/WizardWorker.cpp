/* 
** NetXMS - Network Management System
** Server Configurator for Windows
** Copyright (C) 2005 Victor Kirhenshtein
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
** $module: WizardWorker.cpp
** Worker thread for configuration wizard
**
**/

#include "stdafx.h"
#include "nxconfig.h"


//
// Global data
//

TCHAR g_szWizardErrorText[MAX_ERROR_TEXT] = _T("Completed successfully");


//
// Static data
//

static HWND m_hStatusWnd = NULL;


//
// Execute query and set error text
//

static BOOL DBQueryEx(DB_HANDLE hConn, TCHAR *pszQuery)
{
   BOOL bResult;

   bResult = DBQuery(hConn, pszQuery);
   if (!bResult)
      _sntprintf(g_szWizardErrorText, MAX_ERROR_TEXT,
                 _T("SQL query failed:\n%s"), pszQuery);
   return bResult;
}


//
// Write string value to configuration table
//

static BOOL WriteConfigStr(DB_HANDLE hConn, TCHAR *pszVar, TCHAR *pszValue,
                           BOOL bIsVisible, BOOL bNeedRestart)
{
   DB_RESULT hResult;
   char szQuery[1024];
   BOOL bVarExist = FALSE;

   if (_tcslen(pszVar) > 127)
      return FALSE;

   // Check for variable existence
   _sntprintf(szQuery, 1024, _T("SELECT var_value FROM config WHERE var_name='%s'"), pszVar);
   hResult = DBSelect(hConn, szQuery);
   if (hResult != 0)
   {
      if (DBGetNumRows(hResult) > 0)
         bVarExist = TRUE;
      DBFreeResult(hResult);
   }

   // Create or update variable value
   if (bVarExist)
      _sntprintf(szQuery, 1024, 
                 _T("UPDATE config SET var_value='%s' WHERE var_name='%s'"),
                 pszValue, pszVar);
   else
      _sntprintf(szQuery, 1024, 
                 _T("INSERT INTO config (var_name,var_value,is_visible,need_server_restart) VALUES ('%s','%s',%d,%d)"),
                 pszVar, pszValue, bIsVisible, bNeedRestart);
   return DBQueryEx(hConn, szQuery);
}


//
// Write integer value to configuration table
//

static BOOL WriteConfigInt(DB_HANDLE hConn, TCHAR *pszVar, int iValue,
                           BOOL bIsVisible, BOOL bNeedRestart)
{
   TCHAR szBuffer[64];

   _stprintf(szBuffer, _T("%ld"), iValue);
   return WriteConfigStr(hConn, pszVar, szBuffer, bIsVisible, bNeedRestart);
}


//
// Write unsigned long value to configuration table
//

static BOOL WriteConfigULong(DB_HANDLE hConn, TCHAR *pszVar, DWORD dwValue,
                             BOOL bIsVisible, BOOL bNeedRestart)
{
   TCHAR szBuffer[64];

   _stprintf(szBuffer, _T("%lu"), dwValue);
   return WriteConfigStr(hConn, pszVar, szBuffer, bIsVisible, bNeedRestart);
}


//
// Create configuration file
//

static BOOL CreateConfigFile(WIZARD_CFG_INFO *pc)
{
   BOOL bResult = FALSE;
   FILE *pf;
   time_t t;

   PostMessage(m_hStatusWnd, WM_START_STAGE, 0, (LPARAM)_T("Creating configuration file"));

   pf = _tfopen(pc->m_szConfigFile, _T("w"));
   if (pf != NULL)
   {
      t= time(NULL);
      fprintf(pf, _T("#\n# NetXMS Server configuration file\n")
                  _T("# Created by NetXMS Server configuration wizard at %s#\n\n"),
              _tctime(&t));
      fprintf(pf, _T("LogFile = %s\n"), pc->m_bLogToSyslog ? _T("{syslog}") : pc->m_szLogFile);
      fprintf(pf, _T("DBDriver = %s\n"), pc->m_szDBDriver);
      if (pc->m_szDBDrvParams[0] != 0)
         fprintf(pf, _T("DBDrvParams = %s\n"), pc->m_szDBDrvParams);
      if (pc->m_szDBServer[0] != 0)
         fprintf(pf, _T("DBServer = %s\n"), pc->m_szDBServer);
      if (pc->m_szDBName[0] != 0)
         fprintf(pf, _T("DBName = %s\n"), pc->m_szDBName);
      if (pc->m_szDBLogin[0] != 0)
         fprintf(pf, _T("DBLogin = %s\n"), pc->m_szDBLogin);
      if (pc->m_szDBPassword[0] != 0)
         fprintf(pf, _T("DBPassword = %s\n"), pc->m_szDBPassword);
      fprintf(pf, _T("LogFailedSQLQueries = %s\n"), pc->m_bLogFailedSQLQueries ? _T("yes") : _T("no"));
      fclose(pf);
      bResult = TRUE;
   }
   else
   {
      _sntprintf(g_szWizardErrorText, MAX_ERROR_TEXT, _T("Error creating file %s"), pc->m_szConfigFile);
   }

   PostMessage(m_hStatusWnd, WM_STAGE_COMPLETED, bResult, 0);
   return bResult;
}


//
// Create database in MySQL
//

static BOOL CreateDBMySQL(WIZARD_CFG_INFO *pc, DB_HANDLE hConn)
{
   TCHAR szQuery[256];
   BOOL bResult;

   _stprintf(szQuery, _T("CREATE DATABASE %s"), pc->m_szDBName);
   bResult = DBQueryEx(hConn, szQuery);

   if (bResult)
   {
      _stprintf(szQuery, _T("GRANT ALL ON %s.* TO %s IDENTIFIED BY '%s'"),
                pc->m_szDBName, pc->m_szDBLogin, pc->m_szDBPassword);
      bResult = DBQueryEx(hConn, szQuery);

      _stprintf(szQuery, _T("GRANT ALL ON %s.* TO %s@localhost IDENTIFIED BY '%s'"),
                pc->m_szDBName, pc->m_szDBLogin, pc->m_szDBPassword);
      bResult = DBQueryEx(hConn, szQuery);
   }

   if (bResult)
      bResult = DBQueryEx(hConn, _T("FLUSH PRIVILEGES"));
   return bResult;
}


//
// Create database in Microsoft SQL
//

static BOOL CreateDBMSSQL(WIZARD_CFG_INFO *pc, DB_HANDLE hConn)
{
   TCHAR szQuery[256];
   BOOL bResult;

   bResult = DBQueryEx(hConn, _T("USE master"));
   if (bResult)
   {
      _stprintf(szQuery, _T("CREATE DATABASE %s"), pc->m_szDBName);
      bResult = DBQueryEx(hConn, szQuery);
   }

   if (bResult)
   {
      _stprintf(szQuery, _T("USE %s"), pc->m_szDBName);
      bResult = DBQueryEx(hConn, szQuery);
   }

   if (bResult)
   {
      _stprintf(szQuery, _T("sp_addlogin @loginame = '%s', @passwd = '%s', @defdb = '%s'"),
                pc->m_szDBLogin, pc->m_szDBPassword, pc->m_szDBName);
      bResult = DBQueryEx(hConn, szQuery);
   }

   if (bResult)
   {
      _stprintf(szQuery, _T("sp_grantdbaccess @loginame = '%s'"), pc->m_szDBLogin);
      bResult = DBQueryEx(hConn, szQuery);
   }

   if (bResult)
   {
      _stprintf(szQuery, _T("GRANT ALL TO %s"), pc->m_szDBLogin);
      bResult = DBQueryEx(hConn, szQuery);
   }

   return bResult;
}


//
// Create database
//

static BOOL CreateDatabase(WIZARD_CFG_INFO *pc)
{
   DB_HANDLE hConn;
   BOOL bResult = FALSE;

   PostMessage(m_hStatusWnd, WM_START_STAGE, 0, (LPARAM)_T("Connecting to database server as DBA"));
   hConn = DBConnectEx(pc->m_szDBServer, NULL, pc->m_szDBALogin, pc->m_szDBAPassword);
   if (hConn != NULL)
   {
      PostMessage(m_hStatusWnd, WM_STAGE_COMPLETED, TRUE, 0);
      PostMessage(m_hStatusWnd, WM_START_STAGE, 0, (LPARAM)_T("Creating database"));

      switch(pc->m_iDBEngine)
      {
         case DB_ENGINE_MYSQL:
            bResult = CreateDBMySQL(pc, hConn);
            break;
         case DB_ENGINE_MSSQL:
            bResult = CreateDBMSSQL(pc, hConn);
            break;
         default:
            bResult = FALSE;
            _sntprintf(g_szWizardErrorText, MAX_ERROR_TEXT,
                       _T("Unsupported database engine code %d"), pc->m_iDBEngine);
            break;
      }

      DBDisconnect(hConn);
   }

   PostMessage(m_hStatusWnd, WM_STAGE_COMPLETED, bResult, 0);
   return bResult;
}


//
// Worker thread
//

static DWORD __stdcall WorkerThread(void *pArg)
{
   WIZARD_CFG_INFO *pc = (WIZARD_CFG_INFO *)pArg;
   DB_HANDLE hConn = NULL;
   BOOL bResult;

   bResult = CreateConfigFile(pc);

   // Load database driver
   if (bResult)
   {
      PostMessage(m_hStatusWnd, WM_START_STAGE, 0, (LPARAM)_T("Loading database driver"));
      _tcsncpy(g_szDbDriver, pc->m_szDBDriver, MAX_PATH);
      _tcsncpy(g_szDbServer, pc->m_szDBServer, MAX_PATH);
      bResult = DBInit(FALSE, FALSE, FALSE);
      if (!bResult)
         _tcscpy(g_szWizardErrorText, _T("Error loading database driver"));
      PostMessage(m_hStatusWnd, WM_STAGE_COMPLETED, bResult, 0);
   }

   // Create database if requested
   if (pc->m_bCreateDB && bResult)
      bResult = CreateDatabase(pc);

   // Connect to database as user
   if (bResult)
   {
      PostMessage(m_hStatusWnd, WM_START_STAGE, 0, (LPARAM)_T("Connecting to database"));
      hConn = DBConnectEx(pc->m_szDBServer, pc->m_szDBName, pc->m_szDBLogin, pc->m_szDBPassword);
      bResult = (hConn != NULL);
      if (!bResult)
         _tcscpy(g_szWizardErrorText, _T("Unable to connect to database"));
      PostMessage(m_hStatusWnd, WM_STAGE_COMPLETED, bResult, 0);
   }

   // Initialize database if requested
   if ((pc->m_bCreateDB || pc->m_bInitDB) && bResult)
   {
      TCHAR szInitFile[MAX_PATH];
      static TCHAR *szEngineName[] = { _T("mysql"), _T("pgsql"), _T("mssql"), _T("oracle") };

      PostMessage(m_hStatusWnd, WM_START_STAGE, 0, (LPARAM)_T("Initializing database"));

      _sntprintf(szInitFile, MAX_PATH, _T("%s\\lib\\sql\\dbinit_%s.sql"),
                 pc->m_szInstallDir, szEngineName[pc->m_iDBEngine]);
      bResult = ExecSQLBatch(hConn, szInitFile);

      PostMessage(m_hStatusWnd, WM_STAGE_COMPLETED, bResult, 0);
   }

   // Set server parameters
   if (bResult)
   {
      PostMessage(m_hStatusWnd, WM_START_STAGE, 0, (LPARAM)_T("Configuring server"));

#define __CHK(x) if (bResult) bResult = x;

      __CHK(WriteConfigInt(hConn, _T("RunNetworkDiscovery"), pc->m_bRunAutoDiscovery, TRUE, TRUE));
      __CHK(WriteConfigULong(hConn, _T("DiscoveryPollingInterval"), pc->m_dwDiscoveryPI, TRUE, TRUE));
      __CHK(WriteConfigULong(hConn, _T("NumberOfStatusPollers"), pc->m_dwNumStatusPollers, TRUE, TRUE));
      __CHK(WriteConfigULong(hConn, _T("StatusPollingInterval"), pc->m_dwStatusPI, TRUE, TRUE));
      __CHK(WriteConfigULong(hConn, _T("NumberOfConfigurationPollers"), pc->m_dwNumConfigPollers, TRUE, TRUE));
      __CHK(WriteConfigULong(hConn, _T("ConfigurationPollingInterval"), pc->m_dwConfigurationPI, TRUE, TRUE));

      __CHK(WriteConfigStr(hConn, _T("SMTPServer"), pc->m_szSMTPServer, TRUE, FALSE));
      __CHK(WriteConfigStr(hConn, _T("SMTPFromAddr"), pc->m_szSMTPMailFrom, TRUE, FALSE));

      __CHK(WriteConfigInt(hConn, _T("EnableAdminInterface"), pc->m_bEnableAdminInterface, TRUE, TRUE));

#undef __CHK

      PostMessage(m_hStatusWnd, WM_STAGE_COMPLETED, bResult, 0);
   }

   // Cleanup
   if (hConn != NULL)
      DBDisconnect(hConn);

   // Notify UI that job is finished
   PostMessage(m_hStatusWnd, WM_JOB_FINISHED, bResult, 0);
   return 0;
}


//
// Worker thread starter
//

void StartWizardWorkerThread(HWND hWnd, WIZARD_CFG_INFO *pc)
{
   HANDLE hThread;
   DWORD dwThreadId;

   m_hStatusWnd = hWnd;
   hThread = CreateThread(NULL, 0, WorkerThread, pc, 0, &dwThreadId);
   if (hThread != NULL)
      CloseHandle(hThread);
}
