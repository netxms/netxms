/* 
** NetXMS - Network Management System
** Server Configurator for Windows
** Copyright (C) 2005-2010 Victor Kirhenshtein
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
** File: WizardWorker.cpp
** Worker thread for configuration wizard
**
**/

#include "stdafx.h"
#include "nxconfig.h"
#include <nxsrvapi.h>


//
// Global data
//

TCHAR g_szWizardErrorText[MAX_ERROR_TEXT] = _T("Completed successfully");


//
// Static data
//

static HWND m_hStatusWnd = NULL;


//
// Install event source
//

static BOOL InstallEventSource(TCHAR *pszPath)
{
   HKEY hKey;
   TCHAR szBuffer[256];
   DWORD dwTypes = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;

   if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE,
         _T("System\\CurrentControlSet\\Services\\EventLog\\System\\") CORE_EVENT_SOURCE,
         0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL))
   {
      _sntprintf(g_szWizardErrorText, MAX_ERROR_TEXT, 
                 _T("Unable to create registry key: %s"),
                 GetSystemErrorText(GetLastError(), szBuffer, 256));
      return FALSE;
   }

   RegSetValueEx(hKey, _T("TypesSupported"), 0, REG_DWORD,(BYTE *)&dwTypes, sizeof(DWORD));
   RegSetValueEx(hKey, _T("EventMessageFile"), 0, REG_EXPAND_SZ,
                 (BYTE *)pszPath, ((DWORD)_tcslen(pszPath) + 1) * sizeof(TCHAR));

   RegCloseKey(hKey);
   return TRUE;
}


//
// Install Windows service
//

static BOOL InstallService(WIZARD_CFG_INFO *pc)
{
   SC_HANDLE hMgr, hService;
   TCHAR szCmdLine[MAX_PATH * 2], *pszLogin, *pszPassword, szBuffer[256];
   BOOL bResult = FALSE;

   hMgr = OpenSCManager(NULL, NULL, GENERIC_WRITE);
   if (hMgr == NULL)
   {
      _sntprintf(g_szWizardErrorText, MAX_ERROR_TEXT, 
                 _T("Cannot connect to Service Manager: %s"),
                 GetSystemErrorText(GetLastError(), szBuffer, 256));
      return FALSE;
   }

   if (pc->m_szServiceLogin[0] == 0)
   {
      pszLogin = NULL;
      pszPassword = NULL;
   }
   else
   {
      pszLogin = pc->m_szServiceLogin;
      pszPassword = pc->m_szServicePassword;
   }
   _sntprintf(szCmdLine, MAX_PATH * 2, _T("\"%s\\bin\\netxmsd.exe\" --config \"%s\""), 
              pc->m_szInstallDir, pc->m_szConfigFile);
   hService = CreateService(hMgr, CORE_SERVICE_NAME, _T("NetXMS Core"),
                            GENERIC_READ, SERVICE_WIN32_OWN_PROCESS,
                            SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
                            szCmdLine, NULL, NULL, pc->m_pszDependencyList,
                            pszLogin, pszPassword);
   if (hService == NULL)
   {
      DWORD dwCode = GetLastError();

      if (dwCode == ERROR_SERVICE_EXISTS)
      {
         _sntprintf(g_szWizardErrorText, MAX_ERROR_TEXT,
                    _T("Service named '") CORE_SERVICE_NAME _T("' already exist"));
      }
      else
      {
         _sntprintf(g_szWizardErrorText, MAX_ERROR_TEXT,
                    _T("Cannot create service: %s"),
                    GetSystemErrorText(dwCode, szBuffer, 256));
      }
   }
   else
   {
      CloseServiceHandle(hService);
      bResult = TRUE;
      _sntprintf(szCmdLine, MAX_PATH, _T("%s\\bin\\libnxsrv.dll"), pc->m_szInstallDir);
   }

   CloseServiceHandle(hMgr);

   return bResult ? InstallEventSource(szCmdLine) : FALSE;
}


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


/**
 * Write string value to configuration table
 */
BOOL WriteConfigStr(DB_HANDLE hConn, const TCHAR *varName, const TCHAR *value, BOOL isVisible, BOOL needRestart)
{
   if (_tcslen(varName) > 63)
      return FALSE;

   // Check for variable existence
	DB_STATEMENT hStmt = DBPrepare(hConn, _T("SELECT var_value FROM config WHERE var_name=?"));
	if (hStmt == NULL)
		return FALSE;
	DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, varName, DB_BIND_STATIC);
	DB_RESULT hResult = DBSelectPrepared(hStmt);
   BOOL bVarExist = FALSE;
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         bVarExist = TRUE;
      DBFreeResult(hResult);
   }
	DBFreeStatement(hStmt);

   // Create or update variable value
   if (bVarExist)
	{
		hStmt = DBPrepare(hConn, _T("UPDATE config SET var_value=? WHERE var_name=?"));
		if (hStmt == NULL)
			return FALSE;
		DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, value, DB_BIND_STATIC);
		DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, varName, DB_BIND_STATIC);
	}
   else
	{
		hStmt = DBPrepare(hConn, _T("INSERT INTO config (var_name,var_value,is_visible,need_server_restart) VALUES (?,?,?,?)"));
		if (hStmt == NULL)
			return FALSE;
		DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, varName, DB_BIND_STATIC);
		DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, value, DB_BIND_STATIC);
		DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (LONG)(isVisible ? 1 : 0));
		DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (LONG)(needRestart ? 1 : 0));
	}
   BOOL success = DBExecute(hStmt);
	DBFreeStatement(hStmt);
	return success;
}

/**
 * Write integer value to configuration table
 */
static BOOL WriteConfigInt(DB_HANDLE hConn, TCHAR *pszVar, int iValue, BOOL bIsVisible, BOOL bNeedRestart)
{
   TCHAR szBuffer[64];

   _sntprintf(szBuffer, 64, _T("%d"), iValue);
   return WriteConfigStr(hConn, pszVar, szBuffer, bIsVisible, bNeedRestart);
}

/**
 * Write unsigned long value to configuration table
 */
static BOOL WriteConfigULong(DB_HANDLE hConn, TCHAR *pszVar, DWORD dwValue,
                             BOOL bIsVisible, BOOL bNeedRestart)
{
   TCHAR szBuffer[64];

   _sntprintf(szBuffer, 64, _T("%u"), dwValue);
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
      _ftprintf(pf, _T("#\n# NetXMS Server configuration file\n")
                  _T("# Created by NetXMS Server configuration wizard at %s#\n\n"),
              _tctime(&t));
      _ftprintf(pf, _T("LogFile = %s\n"), pc->m_bLogToSyslog ? _T("{syslog}") : pc->m_szLogFile);
      _ftprintf(pf, _T("DBDriver = %s\n"), pc->m_szDBDriver);
      if (pc->m_szDBDrvParams[0] != 0)
         _ftprintf(pf, _T("DBDrvParams = %s\n"), pc->m_szDBDrvParams);
      if (!_tcsicmp(pc->m_szDBDriver, _T("sqlite.ddr")))
      {
         _ftprintf(pf, _T("DBName = %s\\database\\%s\n"), pc->m_szInstallDir, pc->m_szDBName);
      }
      else
      {
         if (pc->m_szDBServer[0] != 0)
            _ftprintf(pf, _T("DBServer = %s\n"), pc->m_szDBServer);
         if ((pc->m_szDBName[0] != 0) && (_tcsicmp(pc->m_szDBDriver, _T("odbc.ddr"))))
            _ftprintf(pf, _T("DBName = %s\n"), pc->m_szDBName);
         if (pc->m_szDBLogin[0] != 0)
            _ftprintf(pf, _T("DBLogin = %s\n"), pc->m_szDBLogin);
         if (pc->m_szDBPassword[0] != 0)
            _ftprintf(pf, _T("DBPassword = %s\n"), pc->m_szDBPassword);
      }
      _ftprintf(pf, _T("LogFailedSQLQueries = %s\n"), pc->m_bLogFailedSQLQueries ? _T("yes") : _T("no"));
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

   _sntprintf(szQuery, 256, _T("CREATE DATABASE %s"), pc->m_szDBName);
   bResult = DBQueryEx(hConn, szQuery);

   if (bResult)
   {
      _sntprintf(szQuery, 256, _T("GRANT ALL ON %s.* TO %s IDENTIFIED BY '%s'"),
                pc->m_szDBName, pc->m_szDBLogin, pc->m_szDBPassword);
      bResult = DBQueryEx(hConn, szQuery);

      _sntprintf(szQuery, 256, _T("GRANT ALL ON %s.* TO %s@localhost IDENTIFIED BY '%s'"),
                pc->m_szDBName, pc->m_szDBLogin, pc->m_szDBPassword);
      bResult = DBQueryEx(hConn, szQuery);
   }

   if (bResult)
      bResult = DBQueryEx(hConn, _T("FLUSH PRIVILEGES"));
   return bResult;
}


//
// Create database in PostgreSQL
//

static BOOL CreateDBPostgreSQL(WIZARD_CFG_INFO *pc, DB_HANDLE hConn)
{
   TCHAR szQuery[256];
   BOOL bResult;

   _sntprintf(szQuery, 256, _T("CREATE DATABASE %s"), pc->m_szDBName);
   bResult = DBQueryEx(hConn, szQuery);

   if (bResult)
   {
      _sntprintf(szQuery, 256, _T("CREATE USER %s WITH PASSWORD '%s'"),
                pc->m_szDBLogin, pc->m_szDBPassword);
      bResult = DBQueryEx(hConn, szQuery);
   }

   if (bResult)
   {
      _sntprintf(szQuery, 256, _T("GRANT ALL PRIVILEGES ON DATABASE %s TO %s"),
                pc->m_szDBName, pc->m_szDBLogin);
      bResult = DBQueryEx(hConn, szQuery);
   }

   return bResult;
}


//
// Create database in Microsoft SQL
//

static BOOL CreateDBMSSQL(WIZARD_CFG_INFO *pc, DB_HANDLE hConn)
{
   TCHAR szQuery[512], *pszLogin;
   BOOL bResult;

   bResult = DBQueryEx(hConn, _T("USE master"));
   if (bResult)
   {
      _sntprintf(szQuery, 512, _T("CREATE DATABASE %s"), pc->m_szDBName);
      bResult = DBQueryEx(hConn, szQuery);
   }

   if (bResult)
   {
      _sntprintf(szQuery, 512, _T("USE %s"), pc->m_szDBName);
      bResult = DBQueryEx(hConn, szQuery);
   }

   if (bResult)
   {
      if (!_tcscmp(pc->m_szDBLogin, _T("*")))
      {
         // Use Windows authentication
         pszLogin = pc->m_szServiceLogin;
      }
      else
      {
         _sntprintf(szQuery, 512, _T("sp_addlogin @loginame = '%s', @passwd = '%s', @defdb = '%s'"),
                    pc->m_szDBLogin, pc->m_szDBPassword, pc->m_szDBName);
         bResult = DBQueryEx(hConn, szQuery);
         pszLogin = pc->m_szDBLogin;
      }
   }

   if (bResult)
   {
      _sntprintf(szQuery, 512, _T("sp_grantdbaccess @loginame = '%s'"), pszLogin);
      bResult = DBQueryEx(hConn, szQuery);
   }

   if (bResult)
   {
      _sntprintf(szQuery, 512, _T("GRANT ALL TO %s"), pszLogin);
      bResult = DBQueryEx(hConn, szQuery);
   }

   return bResult;
}


//
// Create SQLite embedded database
//

static BOOL CreateSQLiteDB(WIZARD_CFG_INFO *pc)
{
   TCHAR szBaseDir[MAX_PATH], dbErrorText[DBDRV_MAX_ERROR_TEXT];
   DB_HANDLE hConn;
   BOOL bResult = FALSE;

   _sntprintf(szBaseDir, MAX_PATH, _T("%s\\database"), pc->m_szInstallDir);
   SetCurrentDirectory(szBaseDir);
   DeleteFile(pc->m_szDBName);
	hConn = DBConnect(pc->m_dbDriver, NULL, pc->m_szDBName, NULL, NULL, NULL, dbErrorText);
   if (hConn != NULL)
   {
      DBDisconnect(hConn);
      bResult = TRUE;
   }
	else
	{
		_sntprintf(g_szWizardErrorText, MAX_ERROR_TEXT, _T("Unable to connect to database: %s"), dbErrorText);
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
	TCHAR dbErrorText[DBDRV_MAX_ERROR_TEXT];

   if (pc->m_iDBEngine == DB_ENGINE_SQLITE)
   {
      PostMessage(m_hStatusWnd, WM_START_STAGE, 0, (LPARAM)_T("Creating database"));
      bResult = CreateSQLiteDB(pc);
   }
   else
   {
      PostMessage(m_hStatusWnd, WM_START_STAGE, 0, (LPARAM)_T("Connecting to database server as DBA"));
		hConn = DBConnect(pc->m_dbDriver, pc->m_szDBServer, 
                        (pc->m_iDBEngine == DB_ENGINE_PGSQL) ? _T("template1") : NULL,
                        pc->m_szDBALogin, pc->m_szDBAPassword, NULL, dbErrorText);
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
            case DB_ENGINE_PGSQL:
               bResult = CreateDBPostgreSQL(pc, hConn);
               break;
            default:
               bResult = FALSE;
               _sntprintf(g_szWizardErrorText, MAX_ERROR_TEXT,
                          _T("Unsupported database engine code %d"), pc->m_iDBEngine);
               break;
         }

         DBDisconnect(hConn);
      }
      else
      {
			_sntprintf(g_szWizardErrorText, MAX_ERROR_TEXT, _T("Unable to connect to database: %s"), dbErrorText);
      }
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
   TCHAR szDataDir[MAX_PATH], szQuery[256], szGUID[64];
   uuid_t guid;
   BOOL bResult;

   bResult = CreateConfigFile(pc);

   // Load database driver
   if (bResult)
   {
      PostMessage(m_hStatusWnd, WM_START_STAGE, 0, (LPARAM)_T("Loading database driver"));
      bResult = DBInit(0, 0);
		if (bResult)
		{
			pc->m_dbDriver = DBLoadDriver(pc->m_szDBDriver, _T(""), false, NULL, NULL);
			if (pc->m_dbDriver == NULL)
				bResult = FALSE;
		}
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
		TCHAR dbErrorText[DBDRV_MAX_ERROR_TEXT];

      PostMessage(m_hStatusWnd, WM_START_STAGE, 0, (LPARAM)_T("Connecting to database"));
		hConn = DBConnect(pc->m_dbDriver, pc->m_szDBServer, pc->m_szDBName, pc->m_szDBLogin, pc->m_szDBPassword, NULL, dbErrorText);
      bResult = (hConn != NULL);
      if (!bResult)
			_sntprintf(g_szWizardErrorText, MAX_ERROR_TEXT, _T("Unable to connect to database: %s"), dbErrorText);
      PostMessage(m_hStatusWnd, WM_STAGE_COMPLETED, bResult, 0);
   }

   // Initialize database if requested
   if ((pc->m_bCreateDB || pc->m_bInitDB) && bResult)
   {
      TCHAR szInitFile[MAX_PATH];
      static TCHAR *szEngineName[] = { _T("mysql"), _T("pgsql"), _T("mssql"), _T("oracle"), _T("sqlite") };

      PostMessage(m_hStatusWnd, WM_START_STAGE, 0, (LPARAM)_T("Initializing database"));

      _sntprintf(szInitFile, MAX_PATH, _T("%s\\lib\\sql\\dbinit_%s.sql"),
                 pc->m_szInstallDir, szEngineName[pc->m_iDBEngine]);
      bResult = ExecSQLBatch(hConn, szInitFile);

      // Generate GUID for user "admin"
      if (bResult)
      {
         uuid_generate(guid);
         _sntprintf(szQuery, 256, _T("UPDATE users SET guid='%s' WHERE id=0"),
                    uuid_to_string(guid, szGUID));
         bResult = DBQueryEx(hConn, szQuery);
      }

      // Generate GUID for "everyone" group
      if (bResult)
      {
         uuid_generate(guid);
         _sntprintf(szQuery, 256, _T("UPDATE user_groups SET guid='%s' WHERE id=%d"),
                    uuid_to_string(guid, szGUID), GROUP_EVERYONE);
         bResult = DBQueryEx(hConn, szQuery);
      }

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

      _sntprintf(szDataDir, MAX_PATH, _T("%s\\var"), pc->m_szInstallDir);
      __CHK(WriteConfigStr(hConn, _T("DataDirectory"), szDataDir, TRUE, TRUE));

      __CHK(WriteConfigStr(hConn, _T("SMSDriver"), pc->m_szSMSDriver, TRUE, TRUE));
      __CHK(WriteConfigStr(hConn, _T("SMSDrvConfig"), pc->m_szSMSDrvParam, TRUE, TRUE));

#undef __CHK

      PostMessage(m_hStatusWnd, WM_STAGE_COMPLETED, bResult, 0);
   }

   // Cleanup
   if (hConn != NULL)
      DBDisconnect(hConn);

   // Install service
   if (bResult)
   {
      PostMessage(m_hStatusWnd, WM_START_STAGE, 0, (LPARAM)_T("Installing service"));
      bResult = InstallService(pc);
      PostMessage(m_hStatusWnd, WM_STAGE_COMPLETED, bResult, 0);
   }

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
